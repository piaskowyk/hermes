/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#define DEBUG_TYPE "functionanalysis"

#include "hermes/IR/IR.h"
#include "hermes/Optimizer/PassManager/Pass.h"
#include "hermes/Optimizer/Scalar/Utils.h"
#include "hermes/Support/Statistic.h"

#include "llvh/ADT/SmallPtrSet.h"
#include "llvh/Support/Debug.h"

namespace hermes {

namespace {
/// Registers the call by setting the target/env operands if possible,
/// if they haven't been set yet.
/// \param call the Call instruction being analyzed.
/// \param callee the expected callee of the call instruction.
/// \param scope the scope instruction that should be populated on the call, or
///        null if the scope is not available.
void registerCallsite(
    BaseCallInst *call,
    BaseCreateCallableInst *callee,
    Instruction *scope) {
  // Set the target/env operands if possible.
  if (llvh::isa<EmptySentinel>(call->getTarget())) {
    call->setTarget(callee->getFunctionCode());
  }

  // Check if the function uses its parent scope, and populate it if possible.
  if (scope && llvh::isa<EmptySentinel>(call->getEnvironment()) &&
      callee->getFunctionCode()->getParentScopeParam()->hasUsers())
    call->setEnvironment(scope);
}

/// Check if the call \p CI which uses the closure \p C may leak the closure
/// through its arguments.
/// \param C the closure being analyzed.
/// \param F the function associated with \p C.
/// \param CI the call instruction that uses \p C.
/// \return true if the closure may leak through the call, false otherwise.
bool canEscapeThroughCall(Instruction *C, Function *F, BaseCallInst *CI) {
  // The call does not actually invoke C, so we must assume it is leaked.
  if (CI->getCallee() != C)
    return true;

  // Check if the closure is used as any of the arguments. If it is, and the
  // argument is actually used by F, assume that it escapes.
  // TODO: If we know that F does not indirectly access arguments, we can refine
  // this by looking at if/how this argument is used.
  for (int i = 0, e = CI->getNumArguments(); i < e; i++)
    if (C == CI->getArgument(i))
      return true;

  // Check if the closure is passed as the new.target argument, and the function
  // actually uses it.
  // TODO: Allow certain instructions to use new.target.
  if (C == CI->getNewTarget() && F->getNewTargetParam()->hasUsers())
    return true;

  return false;
}

/// Find all callsites that could call a function via the closure created
/// by the \p create instruction and register them.
/// Looks at calls that use \p create as an operand themselves as well as
/// calls that load \p create via a variable which is stored to once.
void analyzeCreateCallable(BaseCreateCallableInst *create) {
  Function *F = create->getFunctionCode();
  Module *M = F->getParent();

  /// Define an element in the worklist below.
  struct UserAndScope {
    /// An instruction that is known to have the value of the closure at
    /// runtime.
    Instruction *closure;

    /// An instruction that is known to produce the scope of the closure at the
    /// point where the closure is used.
    Instruction *scope;
  };

  // List of instructions whose result we know is the same closure created by
  // \p create, and the associated scope at the point of the instruction.
  // Initially populated with \p create itself, it also can contain
  // LoadFrameInst, casts, etc.
  // The users of the elements of this list can then be iterated to find calls,
  // ways for the closure to escape, and anything else we want to analyze.
  // When the list is empty, we're done analyzing \p create.
  llvh::SmallVector<UserAndScope, 2> worklist{};

  // Use a set to avoid revisiting the same Instruction.
  // For example, if the same function is stored to two vars we need
  // to avoid going back and forth between the corresponding loads.
  llvh::SmallPtrSet<Instruction *, 2> visited{};

  worklist.push_back({create, create->getScope()});
  while (!worklist.empty()) {
    // Instruction whose result is known to be the closure.
    Instruction *closureInst = worklist.back().closure;

    // Instruction whose result is known to be the scope of the closure.
    auto *knownScope = worklist.pop_back_val().scope;

    if (!visited.insert(closureInst).second) {
      // Already visited.
      continue;
    }

    for (Instruction *closureUser : closureInst->getUsers()) {
      // Closure is used as the callee operand.
      if (auto *call = llvh::dyn_cast<BaseCallInst>(closureUser)) {
        if (canEscapeThroughCall(closureInst, F, call)) {
          // F potentially escapes.
          F->getAttributesRef(M)._allCallsitesKnownInStrictMode = false;
        }
        if (call->getCallee() == closureInst) {
          registerCallsite(call, create, knownScope);
        }
        continue;
      }

      // Construction setup instructions can't leak the closure on their own,
      // but don't contribute to the call graph.
      if (isConstructionSetup(closureUser, closureInst)) {
        continue;
      }

      if (llvh::isa<GetClosureScopeInst>(closureUser)) {
        // If the scope is available, replace this instruction with it. It will
        // now be unused, but we avoid deleting any instructions in
        // FunctionAnalysis since we are iterating over the IR, so it will be
        // deleted by DCE.
        if (knownScope)
          closureUser->replaceAllUsesWith(knownScope);

        // Getting the closure scope does not leak the closure.
        continue;
      }

      // UnionNarrowTrustedInst is a cast, the result is the same as its input.
      // That means we can add it to the worklist to follow it.
      if (llvh::isa<UnionNarrowTrustedInst>(closureUser)) {
        assert(
            llvh::cast<UnionNarrowTrustedInst>(closureUser)
                ->getSingleOperand()
                ->getType()
                .canBeObject() &&
            "closure type is not object");
        assert(
            llvh::cast<UnionNarrowTrustedInst>(closureUser)
                ->getType()
                .canBeObject() &&
            "The result UnionNarrowTrusted of closure is not object");
        worklist.push_back({closureUser, knownScope});
        continue;
      }

      // CheckedTypeCast's result is the same as its input, as long as the
      // output allows the closure type.
      // That means that if the conditions are met, we can add it to the
      // worklist to follow it.
      if (auto *CC = llvh::dyn_cast<CheckedTypeCastInst>(closureUser)) {
        assert(
            CC->getCheckedValue()->getType().canBeObject() &&
            "closure type is not object");
        if (CC->getType().canBeObject()) {
          worklist.push_back({closureUser, knownScope});
          continue;
        }
      }

      // Closure is stored to a variable, look at corresponding loads
      // to find callsites.
      if (auto *store = llvh::dyn_cast<StoreFrameInst>(closureUser)) {
        Variable *var = store->getVariable();
        if (!isStoreOnceVariable(var)) {
          // Multiple stores to the variable, give up.
          F->getAttributesRef(M)._allCallsitesKnownInStrictMode = false;
          continue;
        }

        // If the scope is the same as the scope we are storing into, we know
        // that the scope of the closure will always just be a pointer back to
        // the scope. We can therefore can propagate it by simply using the
        // scope at the point it is loaded.
        bool propagateScope = store->getScope() == knownScope;

        for (Instruction *varUser : var->getUsers()) {
          auto *load = llvh::dyn_cast<LoadFrameInst>(varUser);
          if (!load) {
            // Skip all stores, because they'll all be storing the same
            // closure.
            assert(
                llvh::isa<StoreFrameInst>(varUser) &&
                "only Store and Load can use variables");
            continue;
          }
          worklist.push_back(
              {load, propagateScope ? load->getScope() : nullptr});
        }
        continue;
      }

      // Unknown user, F could escape somewhere.
      LLVM_DEBUG(
          llvh::dbgs() << "Unknown user of function '"
                       << F->getInternalNameStr()
                       << "': " << closureUser->getKindStr() << '\n');
      F->getAttributesRef(M)._allCallsitesKnownInStrictMode = false;
    }
  }
}

/// Find and register any callsites that can be found which call \p F.
void analyzeFunctionCallsites(Function *F) {
  Module *M = F->getParent();

  // Attempt to start from a position of knowing all callsites.
  F->getAttributesRef(M)._allCallsitesKnownInStrictMode = true;

  if (F->isGlobalScope()) {
    // global function is called by the runtime, so its callsites aren't known.
    F->getAttributesRef(M)._allCallsitesKnownInStrictMode = false;
  }

  // Users can be added as the loop iterates.
  for (size_t i = 0; i < F->getNumUsers(); ++i) {
    Instruction *user = F->getUsers()[i];
    if (auto *create = llvh::dyn_cast<BaseCreateCallableInst>(user)) {
      assert(
          create->getFunctionCode() == F &&
          "Function can only be used as the FunctionCode operand");
      analyzeCreateCallable(create);
      continue;
    }

    if (auto *call = llvh::dyn_cast<BaseCallInst>(user)) {
      // Ignore uses as call target.
      (void)call;
      assert(
          call->getTarget() == F &&
          "invalid use of Function as operand of call");
      continue;
    }

    // Unknown user of Function.
    LLVM_DEBUG(
        llvh::dbgs() << "Unknown function user: " << user->getKindStr()
                     << '\n');
    F->getAttributesRef(M)._allCallsitesKnownInStrictMode = false;
  }

  // If all callsites are known, and none of the users are calls, then the
  // function is unreachable.
  if (F->getAttributesRef(M)._allCallsitesKnownInStrictMode) {
    F->getAttributesRef(M).unreachable =
        !llvh::any_of(F->getUsers(), llvh::isa<BaseCallInst, Instruction *>);
  }
}

} // namespace

Pass *createFunctionAnalysis() {
  /// Analyze Function call graphs to update `target` operands and attributes.
  class FunctionAnalysis : public ModulePass {
   public:
    explicit FunctionAnalysis() : hermes::ModulePass("FunctionAnalysis") {}
    ~FunctionAnalysis() override = default;

    /// Create the call graph for \p mod by analyzing all potential callsites
    /// for all functions and populating the maps.
    /// If a callee is definitely known, populate the target/env operands on the
    /// \c BaseCallInst.
    bool runOnModule(Module *M) override {
      for (Function &F : *M) {
        analyzeFunctionCallsites(&F);
      }
      return true;
    }
  };

  return new FunctionAnalysis();
}

} // namespace hermes
#undef DEBUG_TYPE
