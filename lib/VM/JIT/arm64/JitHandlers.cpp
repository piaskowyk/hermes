/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "hermes/VM/JIT/Config.h"
#if HERMESVM_JIT
#include "JitHandlers.h"

#include "hermes/VM/Callable.h"
#include "hermes/VM/CodeBlock.h"
#include "hermes/VM/Interpreter.h"
#include "hermes/VM/RuntimeModule-inline.h"
#include "hermes/VM/RuntimeModule.h"
#include "hermes/VM/StackFrame-inline.h"
#include "hermes/VM/StackFrame.h"
#include "hermes/VM/StaticHUtils.h"

#define DEBUG_TYPE "jit"

namespace hermes::vm {

SHLegacyValue _sh_ljs_create_bytecode_closure(
    SHRuntime *shr,
    const SHLegacyValue *env,
    SHRuntimeModule *shRuntimeModule,
    uint32_t functionID) {
  Runtime &runtime = getRuntime(shr);
  auto *runtimeModule = (RuntimeModule *)shRuntimeModule;
  GCScopeMarkerRAII marker{runtime};
  return JSFunction::createWithInferredParent(
             runtime,
             runtimeModule->getDomain(runtime),
             env ? Handle<Environment>::vmcast(toPHV(env))
                 : Runtime::makeNullHandle<Environment>(),
             runtimeModule->getCodeBlockMayAllocate(functionID))
      .getHermesValue();
}

SHLegacyValue _sh_ljs_get_bytecode_string(
    SHRuntime *shr,
    SHRuntimeModule *runtimeModule,
    uint32_t stringID) {
  return HermesValue::encodeStringValue(
      ((RuntimeModule *)runtimeModule)
          ->getStringPrimFromStringIDMayAllocate(stringID));
}

SHLegacyValue _interpreter_create_object_from_buffer(
    SHRuntime *shr,
    SHCodeBlock *codeBlock,
    uint32_t shapeTableIndex,
    uint32_t valBufferOffset) {
  Runtime &runtime = getRuntime(shr);
  CallResult<PseudoHandle<>> res = Interpreter::createObjectFromBuffer(
      runtime, (CodeBlock *)codeBlock, shapeTableIndex, valBufferOffset);
  if (LLVM_UNLIKELY(res == ExecutionStatus::EXCEPTION)) {
    _sh_throw_current(shr);
  }
  return res->getHermesValue();
}

/// Wrapper around Interpreter::createArrayFromBuffer.
SHLegacyValue _interpreter_create_array_from_buffer(
    SHRuntime *shr,
    SHCodeBlock *codeBlock,
    unsigned numElements,
    unsigned numLiterals,
    unsigned bufferIndex) {
  Runtime &runtime = getRuntime(shr);
  CallResult<PseudoHandle<>> res = Interpreter::createArrayFromBuffer(
      runtime, (CodeBlock *)codeBlock, numElements, numLiterals, bufferIndex);
  if (LLVM_UNLIKELY(res == ExecutionStatus::EXCEPTION)) {
    _sh_throw_current(shr);
  }
  return res->getHermesValue();
}

/// Implementation of createFunctionEnvironment that takes the closure to get
/// the parentEnvironment from.
/// The native backend doesn't use createFunctionEnvironment.
SHLegacyValue _sh_ljs_create_function_environment(
    SHRuntime *shr,
    SHLegacyValue *frame,
    uint32_t size) {
  Runtime &runtime = getRuntime(shr);

  StackFramePtr framePtr{toPHV(frame)};
  struct : public Locals {
    PinnedValue<Environment> parent;
  } lv;
  LocalsRAII lraii{runtime, &lv};

  lv.parent = framePtr.getCalleeClosureUnsafe()->getEnvironment(runtime);
  return Environment::create(runtime, lv.parent, size);
}

void _sh_print_function_entry_exit(bool enter, const char *msg) {
  static unsigned level = 0;
  if (enter) {
    printf("%*s*** Enter FunctionID ", level * 4, "");
    ++level;
  } else {
    --level;
    printf("%*s*** Leave FunctionID ", level * 4, "");
  }
  printf("%s", msg);
  fflush(stdout);
}

} // namespace hermes::vm

#endif // HERMESVM_JIT
