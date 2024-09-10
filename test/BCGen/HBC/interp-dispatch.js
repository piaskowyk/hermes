/**
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

// RUN: %hermesc -fno-inline -O -dump-bytecode %s | %FileCheckOrRegen --match-full-lines %s

"use strict";

function bench (lc, fc) {
    // Ensure we still emit compare/branch instructions.
    lc = +lc;
    fc = +fc;
    var n, fact;
    var res = 0;
    while (--lc >= 0) {
        n = fc;
        fact = n;
        while (--n > 1)
            fact *= n;
        res += fact;
    }
    return res;
}

print(bench(4e6, 100))

// Auto-generated content below. Please do not modify manually.

// CHECK:Bytecode File Information:
// CHECK-NEXT:  Bytecode version number: {{.*}}
// CHECK-NEXT:  Source hash: {{.*}}
// CHECK-NEXT:  Function count: 2
// CHECK-NEXT:  String count: 3
// CHECK-NEXT:  BigInt count: 0
// CHECK-NEXT:  String Kind Entry count: 2
// CHECK-NEXT:  RegExp count: 0
// CHECK-NEXT:  Segment ID: 0
// CHECK-NEXT:  CommonJS module count: 0
// CHECK-NEXT:  CommonJS module count (static): 0
// CHECK-NEXT:  Function source count: 0
// CHECK-NEXT:  Bytecode options:
// CHECK-NEXT:    staticBuiltins: 0
// CHECK-NEXT:    cjsModulesStaticallyResolved: 0

// CHECK:Global String Table:
// CHECK-NEXT:s0[ASCII, 0..5]: global
// CHECK-NEXT:i1[ASCII, 6..10] #20300996: bench
// CHECK-NEXT:i2[ASCII, 11..15] #A689F65B: print

// CHECK:Function<global>(1 params, 15 registers, 1 numbers, 0 non-pointers):
// CHECK-NEXT:Offset in debug table: source 0x0000, lexical 0x0000
// CHECK-NEXT:    DeclareGlobalVar  "bench"
// CHECK-NEXT:    CreateTopLevelEnvironment r4, 0
// CHECK-NEXT:    CreateClosure     r3, r4, Function<bench>
// CHECK-NEXT:    GetGlobalObject   r4
// CHECK-NEXT:    PutByIdStrict     r4, r3, 1, "bench"
// CHECK-NEXT:    TryGetById        r2, r4, 1, "print"
// CHECK-NEXT:    GetByIdShort      r1, r4, 2, "bench"
// CHECK-NEXT:    LoadConstUndefined r3
// CHECK-NEXT:    LoadConstInt      r0, 4000000
// CHECK-NEXT:    LoadConstUInt8    r4, 100
// CHECK-NEXT:    Call3             r4, r1, r3, r0, r4
// CHECK-NEXT:    Call2             r4, r2, r3, r4
// CHECK-NEXT:    Ret               r4

// CHECK:Function<bench>(3 params, 14 registers, 10 numbers, 0 non-pointers):
// CHECK-NEXT:Offset in debug table: source 0x0017, lexical 0x0000
// CHECK-NEXT:    LoadParam         r11, 1
// CHECK-NEXT:    ToNumber          r11, r11
// CHECK-NEXT:    LoadParam         r10, 2
// CHECK-NEXT:    ToNumber          r5, r10
// CHECK-NEXT:    LoadConstUInt8    r6, 1
// CHECK-NEXT:    SubN              r7, r11, r6
// CHECK-NEXT:    LoadConstZero     r8
// CHECK-NEXT:    SubN              r9, r5, r6
// CHECK-NEXT:    LoadConstZero     r10
// CHECK-NEXT:    LoadConstZero     r11
// CHECK-NEXT:    JNotGreaterEqualN L1, r7, r11
// CHECK-NEXT:L4:
// CHECK-NEXT:    Mov               r2, r10
// CHECK-NEXT:    Mov               r4, r7
// CHECK-NEXT:    Mov               r0, r5
// CHECK-NEXT:    Mov               r3, r0
// CHECK-NEXT:    Mov               r1, r9
// CHECK-NEXT:    JNotGreaterN      L2, r1, r6
// CHECK-NEXT:L3:
// CHECK-NEXT:    MulN              r0, r0, r1
// CHECK-NEXT:    SubN              r1, r1, r6
// CHECK-NEXT:    Mov               r3, r0
// CHECK-NEXT:    JGreaterN         L3, r1, r6
// CHECK-NEXT:L2:
// CHECK-NEXT:    AddN              r10, r2, r3
// CHECK-NEXT:    SubN              r7, r4, r6
// CHECK-NEXT:    Mov               r11, r10
// CHECK-NEXT:    JGreaterEqualN    L4, r7, r8
// CHECK-NEXT:L1:
// CHECK-NEXT:    Ret               r11

// CHECK:Debug filename table:
// CHECK-NEXT:  0: {{.*}}interp-dispatch.js

// CHECK:Debug file table:
// CHECK-NEXT:  source table offset 0x0000: filename id 0

// CHECK:Debug source table:
// CHECK-NEXT:  0x0000  function idx 0, starts at line 10 col 1
// CHECK-NEXT:    bc 0: line 10 col 1
// CHECK-NEXT:    bc 18: line 10 col 1
// CHECK-NEXT:    bc 24: line 28 col 1
// CHECK-NEXT:    bc 30: line 28 col 7
// CHECK-NEXT:    bc 46: line 28 col 12
// CHECK-NEXT:    bc 52: line 28 col 6
// CHECK-NEXT:  0x0017  function idx 1, starts at line 12 col 1
// CHECK-NEXT:    bc 3: line 14 col 10
// CHECK-NEXT:    bc 9: line 15 col 10
// CHECK-NEXT:  0x0021  end of debug source table
