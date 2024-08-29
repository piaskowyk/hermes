/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include "asmjit/a64.h"

#include "hermes/ADT/DenseUInt64.h"
#include "hermes/ADT/SimpleLRU.h"
#include "hermes/Support/OptValue.h"
#include "hermes/VM/static_h.h"

#include "llvh/ADT/DenseMap.h"

#include <deque>

#include "hermes/VM/CodeBlock.h"

namespace hermes::vm::arm64 {

namespace a64 = asmjit::a64;

/// A HermesVM frame register
class FR {
  uint32_t index_;

 public:
  static constexpr uint32_t kInvalid = UINT32_MAX;

  FR() : index_(kInvalid) {}
  constexpr explicit FR(uint32_t index) : index_(index) {}

  constexpr bool isValid() const {
    return index_ != kInvalid;
  }

  constexpr uint32_t index() const {
    return index_;
  }
  bool operator==(const FR &fr) const {
    return fr.index_ == index_;
  }
  bool operator!=(const FR &fr) const {
    return fr.index_ != index_;
  }
};

enum class FRType : uint8_t {
  Union = 1,
  Number = 2,
  Bool = 4,
  Pointer = 8,
  Unknown = 16,
  UnknownPtr = Union | Unknown | Pointer,
};

class HWReg {
  // 0..31: GpX. 32..63: VecD. 128: invalid.
  uint8_t index_;

  explicit constexpr HWReg(uint8_t index) : index_(index) {}

 public:
  struct GpX {};
  struct VecD {};

  constexpr HWReg() : index_(0xFF) {}
  explicit constexpr HWReg(uint8_t index, GpX) : index_(index) {}
  explicit constexpr HWReg(uint8_t index, VecD) : index_(index + 32) {}
  explicit constexpr HWReg(const a64::GpX &gpx) : HWReg(gpx.id(), GpX{}) {}
  explicit constexpr HWReg(const a64::VecD &vecd) : HWReg(vecd.id(), VecD{}) {}

  static constexpr HWReg gpX(uint8_t index) {
    assert(index < 31 && "invalid GpX");
    return HWReg(index, GpX{});
  }
  static constexpr HWReg vecD(uint8_t index) {
    assert(index < 32 && "invalid VecD");
    return HWReg(index, VecD{});
  }

  operator bool() const {
    return isValid();
  }
  bool isValid() const {
    return index_ != 0xFF;
  }
  bool isValidGpX() const {
    return index_ < 32;
  }
  bool isValidVecD() const {
    return index_ >= 32 && index_ < 64;
  }
  bool isGpX() const {
    assert(isValid());
    return index_ < 32;
  }
  bool isVecD() const {
    assert(isValid());
    return index_ >= 32 && index_ < 64;
  }

  a64::GpX a64GpX() const {
    assert(isGpX());
    return a64::GpX(indexInClass());
  }
  a64::VecD a64VecD() const {
    assert(isVecD());
    return a64::VecD(indexInClass());
  }

  uint8_t combinedIndex() const {
    assert(isValid());
    return index_ & 63;
  }
  uint8_t indexInClass() const {
    assert(isValid());
    return index_ & 31;
  }

  bool operator==(const HWReg &other) const {
    return index_ == other.index_;
  }
  bool operator!=(const HWReg &other) const {
    return index_ != other.index_;
  }
};

/// A frame register can reside simultaneously in one or more of the following
/// locations:
/// - The stack frame
/// - A global callee-save register (which can be either GpX or VecD)
/// - A local GpX register
/// - A local VecD register.
/// A frame register always has an allocated slot in the frame, even if it never
/// uses it.
/// Additionally, it may have an associated global reg, and two local regs.
/// Having them associated with the frame reg does not necessarily mean that the
/// hardware registers contain the most up-to-date value. The following
/// invariants apply:
/// - If there are local registers, they always contain the latest value.
/// - If there is more than one local register, they all contain the same bit
/// pattern.
/// - if there is a global register, it contains the latest value, unless
/// globalRegUpToDate is not set, in which case the latest value *must* be in
/// local registers. The state where there is a global reg, but the latest value
/// is only in the frame is not valid, as it is not useful.
/// - if frameUpToDate is set, then the frame contains the latest value.
struct FRState {
  /// Type that applies for the entire function.
  FRType globalType = FRType::UnknownPtr;
  /// Type in the current basic block, could be narrower. This applies, until
  /// it is reset, to the up-to-date value, local or not.
  FRType localType = FRType::UnknownPtr;

  /// Pre-allocated global register.
  HWReg globalReg{};
  /// Register in the current basic block.
  HWReg localGpX{};
  HWReg localVecD{};

  /// Whether the latest value has been written to the frame.
  bool frameUpToDate = false;
  /// Whether the global register exists and contains an up-to-date value. If
  /// false, either there is no globalReg, or there must be a local register
  /// allocated.
  bool globalRegUpToDate = false;
};

struct HWRegState {
  FR contains{};
};

// x19 is runtime
static constexpr auto xRuntime = a64::x19;
// x20 is frame
static constexpr auto xFrame = a64::x20;
// x0 < xDoubleLim means that it is a double.
//    cmp   x0, xDoubleLim
//    b.hs  slowPath
static constexpr auto xDoubleLim = a64::x21;

/// GP arg registers (inclusive).
// static constexpr std::pair<uint8_t, uint8_t> kGPArgs(0, 7);
/// Temporary GP registers (inclusive).
static constexpr std::pair<uint8_t, uint8_t> kGPTemp(0, 15);
/// Callee-saved GP registers (inclusive).
static constexpr std::pair<uint8_t, uint8_t> kGPSaved(22, 28);

/// Vec arg registers (inclusive).
// static constexpr std::pair<uint8_t, uint8_t> kVecArgs(0, 7);
/// Temporary vec registers (inclusive).
static constexpr std::pair<uint8_t, uint8_t> kVecTemp(16, 31);
/// Callee-saved vec registers (inclusive).
static constexpr std::pair<uint8_t, uint8_t> kVecSaved(8, 15);

static constexpr uint32_t bitMask32(unsigned first, unsigned last) {
  return ((1u << (last - first + 1)) - 1u) << first;
}
template <typename T>
static constexpr uint32_t bitMask32(std::pair<T, T> range) {
  return bitMask32(range.first, range.second);
}

class TempRegAlloc {
  unsigned first_;
  SimpleLRU<unsigned> lru_{};
  std::vector<unsigned *> map_{};
  uint32_t availBits_;

 public:
  TempRegAlloc(std::pair<uint8_t, uint8_t> range)
      : first_(range.first), lru_(range.second - range.first + 1) {
    map_.resize(range.second - range.first + 1);
    availBits_ = bitMask32(range);
  }

  llvh::Optional<unsigned> alloc(
      llvh::Optional<unsigned> preferred = llvh::None) {
    if (availBits_ == 0)
      return llvh::None;

    unsigned index;
    if (preferred && (availBits_ & (1u << *preferred)))
      index = *preferred;
    else
      index = llvh::findFirstSet(availBits_);
    availBits_ &= ~(1u << index);
    assert(index >= first_ && "Invalid tmpreg index");
    assert(!map_[index - first_] && "map shows the index as occupied");
    map_[index - first_] = lru_.add(index);

    return index;
  }

  void use(unsigned index) {
    assert(index >= first_ && "Invalid tmpreg index");
    if (!(availBits_ & (1u << index)))
      lru_.use(map_[index - first_]);
  }

  void free(unsigned index) {
    assert(index >= first_ && "Invalid tmpreg index");
    assert(map_[index - first_] && "map shows the tmpreg is freed");
    assert(!(availBits_ & (1u << index)) && "bitmask shows tmpreg is freed");

    availBits_ |= (1u << index);
    lru_.remove(map_[index - first_]);
    map_[index - first_] = nullptr;
  }

  unsigned leastRecentlyUsed() {
    return *lru_.leastRecent();
  }

 private:
};

class Emitter {
 public:
  std::unique_ptr<asmjit::Logger> logger_{};
  std::unique_ptr<asmjit::ErrorHandler> errorHandler_;
  asmjit::Error expectedError_ = asmjit::kErrorOk;

  std::vector<FRState> frameRegs_;
  std::array<HWRegState, 64> hwRegs_;

  /// GP temp registers.
  TempRegAlloc gpTemp_{kGPTemp};
  /// VecD temp registers.
  TempRegAlloc vecTemp_{kVecTemp};

  /// Keep enough information to generate a slow path at the end of the
  /// function.
  struct SlowPath {
    /// Label of the slow path.
    asmjit::Label slowPathLab;
    /// Label to jump to after the slow path.
    asmjit::Label contLab;
    /// Target if this is a branch.
    asmjit::Label target;

    /// Name of the slow path.
    const char *name;
    /// Frame register indexes;
    FR frRes, frInput1, frInput2;
    /// Optional hardware register for the result.
    HWReg hwRes;
    /// Whether to invert a condition.
    bool invert;

    /// Pointer to the slow path function that must be called.
    void *slowCall;
    /// The name of the slow path function.
    const char *slowCallName;

    /// Callback to actually emit.
    void (*emit)(Emitter &em, SlowPath &sl);
  };
  /// Queue of slow paths.
  std::deque<SlowPath> slowPaths_{};

  /// Descriptor for a single RO data entry.
  struct DataDesc {
    /// Size in bytes.
    int32_t size;
    asmjit::TypeId typeId;
    int32_t itemCount;
    /// Optional comment.
    const char *comment;
  };
  /// Used for pretty printing when logging data.
  std::vector<DataDesc> roDataDesc_{};
  std::vector<uint8_t> roData_{};
  asmjit::Label roDataLabel_{};

  /// Each thunk contains the offset of the function pointer in roData.
  std::vector<std::pair<asmjit::Label, int32_t>> thunks_{};
  llvh::DenseMap<void *, size_t> thunkMap_{};

  /// Map from the bit pattern of a double value to offset in constant pool.
  llvh::DenseMap<hermes::DenseUInt64, int32_t> fp64ConstMap_{};

  /// Label to branch to when returning from a function. Return value will be
  /// in x22.
  asmjit::Label returnLabel_{};

  /// Offset in RODATA of the pointer to the start of the read property
  /// cache.
  int32_t roOfsReadPropertyCachePtr_;
  /// Offset in RODATA of the pointer to the start of the read property
  /// cache.
  int32_t roOfsWritePropertyCachePtr_;

  unsigned gpSaveCount_ = 0;
  unsigned vecSaveCount_ = 0;

 public:
  asmjit::CodeHolder code{};
  a64::Assembler a{};

  explicit Emitter(
      asmjit::JitRuntime &jitRT,
      bool dumpJitCode,
      PropertyCacheEntry *readPropertyCache,
      PropertyCacheEntry *writePropertyCache,
      uint32_t numFrameRegs,
      uint32_t numCount,
      uint32_t npCount);

  /// Add the jitted function to the JIT runtime and return a pointer to it.
  JITCompiledFunctionPtr addToRuntime(asmjit::JitRuntime &jr);

  /// Log a comment.
  /// Annotated with printf-style format.
  void comment(const char *fmt, ...) __attribute__((format(printf, 2, 3)));

  void leave();
  void newBasicBlock(const asmjit::Label &label);
  void call(void *fn, const char *name);

  /// Call a JS function.
  /// TODO: Distinguish this from the other \c call.
  void call(FR frRes, FR frCallee, uint32_t argc);
  void callN(FR frRes, FR frCallee, llvh::ArrayRef<FR> args);
  void callBuiltin(FR frRes, uint32_t builtinIndex, uint32_t argc);
  void callWithNewTarget(FR frRes, FR frCallee, FR frNewTarget, uint32_t argc);

  /// Get a builtin closure.
  void getBuiltinClosure(FR frRes, uint32_t builtinIndex);

  /// Save the return value in x22.
  void ret(FR frValue);
  void mov(FR frRes, FR frInput, bool logComment = true);
  void loadParam(FR frRes, uint32_t paramIndex);
  void loadConstDouble(FR frRes, double val, const char *name);
  void loadConstBits64(FR frRes, uint64_t val, FRType type, const char *name);
  void
  loadConstString(FR frRes, RuntimeModule *runtimeModule, uint32_t stringID);
  void toNumber(FR frRes, FR frInput);

#define DECL_BINOP(methodName, forceNum, commentStr, slowCall, a64body) \
  void methodName(FR rRes, FR rLeft, FR rRight) {                       \
    arithBinOp(                                                         \
        forceNum,                                                       \
        rRes,                                                           \
        rLeft,                                                          \
        rRight,                                                         \
        commentStr,                                                     \
        [](a64::Assembler & as,                                         \
           const a64::VecD &res,                                        \
           const a64::VecD &dl,                                         \
           const a64::VecD &dr) a64body,                                \
        (void *)slowCall,                                               \
        #slowCall);                                                     \
  }

  DECL_BINOP(mul, false, "mul", _sh_ljs_mul_rjs, { as.fmul(res, dl, dr); })
  DECL_BINOP(add, false, "add", _sh_ljs_add_rjs, { as.fadd(res, dl, dr); })
  DECL_BINOP(sub, false, "sub", _sh_ljs_sub_rjs, { as.fsub(res, dl, dr); })
  DECL_BINOP(mulN, true, "mulN", _sh_ljs_mul_rjs, { as.fmul(res, dl, dr); })
  DECL_BINOP(addN, true, "addN", _sh_ljs_add_rjs, { as.fadd(res, dl, dr); })
  DECL_BINOP(subN, true, "subN", _sh_ljs_sub_rjs, { as.fsub(res, dl, dr); })
#undef DECL_BINOP

#define DECL_UNOP(methodName, forceNum, commentStr, slowCall, a64body) \
  void methodName(FR rRes, FR rInput) {                                \
    arithUnop(                                                         \
        forceNum,                                                      \
        rRes,                                                          \
        rInput,                                                        \
        commentStr,                                                    \
        [](a64::Assembler & as,                                        \
           const a64::VecD &d,                                         \
           const a64::VecD &s,                                         \
           const a64::VecD &tmp) a64body,                              \
        (void *)slowCall,                                              \
        #slowCall);                                                    \
  }

  DECL_UNOP(dec, false, "dec", _sh_ljs_dec_rjs, {
    as.fmov(tmp, -1.0);
    as.fadd(d, s, tmp);
  })
  DECL_UNOP(inc, false, "inc", _sh_ljs_inc_rjs, {
    as.fmov(tmp, -1.0);
    as.fadd(d, s, tmp);
  })
#undef DECL_UNOP

#define DECL_JCOND(methodName, forceNum, commentStr, slowCall, a64inst) \
  void methodName(                                                      \
      bool invert, const asmjit::Label &target, FR rLeft, FR rRight) {  \
    jCond(                                                              \
        forceNum,                                                       \
        invert,                                                         \
        target,                                                         \
        rLeft,                                                          \
        rRight,                                                         \
        commentStr,                                                     \
        [](a64::Assembler &as, const asmjit::Label &target) {           \
          as.a64inst(target);                                           \
        },                                                              \
        (void *)slowCall,                                               \
        #slowCall);                                                     \
  }
  DECL_JCOND(jGreater, false, "greater", _sh_ljs_greater_rjs, b_gt)
  DECL_JCOND(
      jGreaterEqual,
      false,
      "greater_equal",
      _sh_ljs_greater_equal_rjs,
      b_ge)
  DECL_JCOND(jGreaterN, true, "greater_n", _sh_ljs_greater_rjs, b_gt)
  DECL_JCOND(
      jGreaterEqualN,
      true,
      "greater_equal_n",
      _sh_ljs_greater_equal_rjs,
      b_ge)
#undef DECL_JCOND

  void getByVal(FR frRes, FR frSource, FR frKey);
  void getByIndex(FR frRes, FR frSource, uint8_t key);

#define DECL_PUT_BY_VAL(methodName, commentStr, shFn)                \
  void methodName(FR frTarget, FR frKey, FR frValue) {               \
    putByValImpl(frTarget, frKey, frValue, commentStr, shFn, #shFn); \
  }

  DECL_PUT_BY_VAL(putByValLoose, "putByValLoose", _sh_ljs_put_by_val_loose_rjs);
  DECL_PUT_BY_VAL(
      putByValStrict,
      "putByValStrict",
      _sh_ljs_put_by_val_strict_rjs);

#define DECL_GET_BY_ID(methodName, commentStr, shFn)                           \
  void methodName(FR frRes, SHSymbolID symID, FR frSource, uint8_t cacheIdx) { \
    getByIdImpl(frRes, symID, frSource, cacheIdx, commentStr, shFn, #shFn);    \
  }

  DECL_GET_BY_ID(getById, "getById", _sh_ljs_get_by_id_rjs)
  DECL_GET_BY_ID(tryGetById, "tryGetById", _sh_ljs_try_get_by_id_rjs)

#define DECL_PUT_BY_ID(methodName, commentStr, shFn)                          \
  void methodName(                                                            \
      FR frTarget, SHSymbolID symID, FR frValue, uint8_t cacheIdx) {          \
    putByIdImpl(frTarget, symID, frValue, cacheIdx, commentStr, shFn, #shFn); \
  }

  DECL_PUT_BY_ID(putByIdLoose, "putByIdLoose", _sh_ljs_put_by_id_loose_rjs);
  DECL_PUT_BY_ID(putByIdStrict, "putByIdStrict", _sh_ljs_put_by_id_strict_rjs);
  DECL_PUT_BY_ID(
      tryPutByIdLoose,
      "tryPutByIdLoose",
      _sh_ljs_try_put_by_id_loose_rjs);
  DECL_PUT_BY_ID(
      tryPutByIdStrict,
      "tryPutByIdStrict",
      _sh_ljs_try_put_by_id_strict_rjs);

  void isIn(FR frRes, FR frLeft, FR frRight);

  asmjit::Label newPrefLabel(const char *pref, size_t index);

  void getGlobalObject(FR frRes);
  void declareGlobalVar(SHSymbolID symID);
  void createTopLevelEnvironment(FR frRes, uint32_t size);
  void getParentEnvironment(FR frRes, uint32_t level);
  void loadFromEnvironment(FR frRes, FR frEnv, uint32_t slot);
  void storeToEnvironment(bool np, FR frEnv, uint32_t slot, FR frValue);
  void createClosure(
      FR frRes,
      FR frEnv,
      RuntimeModule *runtimeModule,
      uint32_t functionID);

 private:
  /// Create an a64::Mem to a specifc frame register.
  static constexpr inline a64::Mem frA64Mem(FR fr) {
    // FIXME: check if the offset fits
    return a64::Mem(xFrame, fr.index() * sizeof(SHLegacyValue));
  }

  /// Load an arbitrary bit pattern into a Gp.
  template <typename REG>
  void loadBits64InGp(const REG &dest, uint64_t bits, const char *constName);

  template <typename R>
  void loadFrame(R dest, FR rFrom) {
    // FIXME: check if the offset fits
    a.ldr(dest, frA64Mem(rFrom));
  }
  template <typename R>
  void storeFrame(R src, FR rFrom) {
    // FIXME: check if the offset fits
    a.str(src, frA64Mem(rFrom));
  }

  bool isTempGpX(HWReg hwReg) const {
    assert(hwReg.isGpX());
    unsigned index = hwReg.indexInClass();
    return index >= kGPTemp.first && index <= kGPTemp.second;
  }

  bool isTempVecD(HWReg hwReg) const {
    assert(hwReg.isVecD());
    unsigned index = hwReg.indexInClass();
    return index >= kVecTemp.first && index <= kVecTemp.second;
  }

  bool isTemp(HWReg hwReg) const {
    return hwReg.isGpX() ? isTempGpX(hwReg) : isTempVecD(hwReg);
  }

  void loadFrameAddr(a64::GpX dst, FR frameReg);
  template <bool use>
  void movHWReg(HWReg dst, HWReg src);
  void _storeHWRegToFrame(FR fr, HWReg src);
  void movHWFromFR(HWReg hwRes, FR src);
  void movHWFromMem(HWReg hwRes, a64::Mem src);

  /// Move a value from a hardware register \p src to the frame register \p
  /// frDest.
  void movFRFromHW(FR frDest, HWReg src, OptValue<FRType> type = llvh::None);

  template <class TAG>
  HWReg _allocTemp(TempRegAlloc &ra, llvh::Optional<HWReg> preferred);
  HWReg allocTempGpX(llvh::Optional<HWReg> preferred = llvh::None) {
    assert((!preferred || preferred->isGpX()) && "invalid preferred register");
    return _allocTemp<HWReg::GpX>(gpTemp_, preferred);
  }
  HWReg allocTempVecD(llvh::Optional<HWReg> preferred = llvh::None) {
    assert((!preferred || preferred->isVecD()) && "invalid preferred register");
    return _allocTemp<HWReg::VecD>(vecTemp_, preferred);
  }
  HWReg allocAndLogTempGpX() {
    HWReg res = allocTempGpX();
    comment("    ; alloc: x%u (temp)", res.indexInClass());
    return res;
  }
  void freeReg(HWReg hwReg);
  void syncAndFreeTempReg(HWReg hwReg);
  HWReg useReg(HWReg hwReg);

  void spillTempReg(HWReg toSpill);
  void syncToMem(FR fr);
  void syncAllTempExcept(FR exceptFR);
  void freeAllTempExcept(FR exceptFR);

  /// Free any temporary register associated with \p FR.
  void freeFRTemp(FR fr);

  void assignAllocatedLocalHWReg(FR fr, HWReg hwReg);

  /// \return a valid register if the FR is in a hw register, otherwise invalid.
  HWReg isFRInRegister(FR fr);
  HWReg getOrAllocFRInVecD(FR fr, bool load);
  HWReg getOrAllocFRInGpX(FR fr, bool load);
  HWReg getOrAllocFRInAnyReg(
      FR fr,
      bool load,
      llvh::Optional<HWReg> preferred = llvh::None);

  void frUpdatedWithHWReg(
      FR fr,
      HWReg hwReg,
      hermes::OptValue<FRType> localType = llvh::None);
  void frUpdateType(FR fr, FRType type);

  /// \return true if the FR is currently known to contain a number.
  bool isFRKnownNumber(FR fr) const {
    auto &frState = frameRegs_[fr.index()];
    return frState.globalType == FRType::Number ||
        frState.localType == FRType::Number;
  }

 private:
  void frameSetup(
      unsigned numFrameRegs,
      unsigned gpSaveCount,
      unsigned vecSaveCount);

  asmjit::Label newSlowPathLabel() {
    return newPrefLabel("SLOW_", slowPaths_.size());
  }
  asmjit::Label newContLabel() {
    return newPrefLabel("CONT_", slowPaths_.size());
  }

  int32_t reserveData(
      int32_t dsize,
      size_t align,
      asmjit::TypeId typeId,
      int32_t itemCount,
      const char *comment = nullptr);
  /// Register a 64-bit constant in RO DATA and return its offset.
  int32_t uint64Const(uint64_t bits, const char *comment);
  asmjit::Label registerCall(void *fn, const char *name = nullptr);

  void emitSlowPaths();
  void emitThunks();
  void emitROData();

 private:
  void arithUnop(
      bool forceNumber,
      FR frRes,
      FR frInput,
      const char *name,
      void (*fast)(
          a64::Assembler &a,
          const a64::VecD &dst,
          const a64::VecD &src,
          const a64::VecD &tmp),
      void *slowCall,
      const char *slowCallName);

  void arithBinOp(
      bool forceNumber,
      FR frRes,
      FR frLeft,
      FR frRight,
      const char *name,
      void (*fast)(
          a64::Assembler &a,
          const a64::VecD &res,
          const a64::VecD &dl,
          const a64::VecD &dr),
      void *slowCall,
      const char *slowCallName);

  void jCond(
      bool forceNumber,
      bool invert,
      const asmjit::Label &target,
      FR frLeft,
      FR frRight,
      const char *name,
      void(fast)(a64::Assembler &a, const asmjit::Label &target),
      void *slowCall,
      const char *slowCallName);

  void putByValImpl(
      FR frTarget,
      FR frKey,
      FR frValue,
      const char *name,
      void (*shImpl)(
          SHRuntime *shr,
          SHLegacyValue *target,
          SHLegacyValue *key,
          SHLegacyValue *value),
      const char *shImplName);

  void getByIdImpl(
      FR frRes,
      SHSymbolID symID,
      FR frSource,
      uint8_t cacheIdx,
      const char *name,
      SHLegacyValue (*shImpl)(
          SHRuntime *shr,
          const SHLegacyValue *source,
          SHSymbolID symID,
          SHPropertyCacheEntry *propCacheEntry),
      const char *shImplName);

  void putByIdImpl(
      FR frTarget,
      SHSymbolID symID,
      FR frValue,
      uint8_t cacheIdx,
      const char *name,
      void (*shImpl)(
          SHRuntime *shr,
          SHLegacyValue *target,
          SHSymbolID symID,
          SHLegacyValue *value,
          SHPropertyCacheEntry *propCacheEntry),
      const char *shImplName);
}; // class Emitter

} // namespace hermes::vm::arm64
