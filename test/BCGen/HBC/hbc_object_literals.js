/**
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

// RUN: %hermes --target=HBC -dump-lir -O %s | %FileCheckOrRegen %s --match-full-lines --check-prefix=IRGEN
// RUN: %hermes --target=HBC -dump-bytecode -O %s | %FileCheckOrRegen %s --match-full-lines --check-prefix=BCGEN

var obj1 = {'a': 'hello', 'b': 1, 'c': null, 'd': undefined, 'e': true, 'f': function() {}, 'g': 2};

// Test the case when even we will save bytecode size if we serialize the object
// into the buffer, we choose not to, because it will add too many placeholders.
var obj2 = {
  a : undefined,
  b : undefined,
  c : undefined,
  d : undefined,
  e : undefined,
  r : undefined,
  f : 1,
  g : 1,
  h : 1,
  i : 1,
  j : 1,
  k : 1,
  l : 1,
  m : 1,
  n : 1,
  o : 1,
  p : 1,
  q : 1,
};

// Cannot serialize undefined as placeholder when the property is a number.
var obj3 = {
  1 : undefined,
  f : 1,
  g : 1,
  h : 1,
  i : 1,
  j : 1,
  k : 1,
  l : 1,
  m : 1,
  n : 1,
  o : 1,
  p : 1,
  q : 1,
}

var obj4 = {
  '1' : undefined,
  f : 1,
  g : 1,
  h : 1,
  i : 1,
  j : 1,
  k : 1,
  l : 1,
  m : 1,
  n : 1,
  o : 1,
  p : 1,
  q : 1,
}

// Auto-generated content below. Please do not modify manually.

// IRGEN:scope %VS0 []

// IRGEN:function global(): undefined
// IRGEN-NEXT:%BB0:
// IRGEN-NEXT:       DeclareGlobalVarInst "obj1": string
// IRGEN-NEXT:       DeclareGlobalVarInst "obj2": string
// IRGEN-NEXT:       DeclareGlobalVarInst "obj3": string
// IRGEN-NEXT:       DeclareGlobalVarInst "obj4": string
// IRGEN-NEXT:  %4 = HBCAllocObjectFromBufferInst (:object) 7: number, "a": string, "hello": string, "b": string, 1: number, "c": string, null: null, "d": string, null: null, "e": string, true: boolean, "f": string, null: null, "g": string, 2: number
// IRGEN-NEXT:  %5 = HBCLoadConstInst (:undefined) undefined: undefined
// IRGEN-NEXT:       PrStoreInst %5: undefined, %4: object, 3: number, "d": string, true: boolean
// IRGEN-NEXT:  %7 = CreateScopeInst (:environment) %VS0: any, empty: any
// IRGEN-NEXT:  %8 = CreateFunctionInst (:object) %7: environment, %f(): functionCode
// IRGEN-NEXT:       PrStoreInst %8: object, %4: object, 5: number, "f": string, false: boolean
// IRGEN-NEXT:  %10 = HBCGetGlobalObjectInst (:object)
// IRGEN-NEXT:        StorePropertyLooseInst %4: object, %10: object, "obj1": string
// IRGEN-NEXT:  %12 = HBCAllocObjectFromBufferInst (:object) 18: number, "a": string, null: null, "b": string, null: null, "c": string, null: null, "d": string, null: null, "e": string, null: null, "r": string, null: null, "f": string, 1: number, "g": string, 1: number, "h": string, 1: number, "i": string, 1: number, "j": string, 1: number, "k": string, 1: number, "l": string, 1: number, "m": string, 1: number, "n": string, 1: number, "o": string, 1: number, "p": string, 1: number, "q": string, 1: number
// IRGEN-NEXT:        PrStoreInst %5: undefined, %12: object, 0: number, "a": string, true: boolean
// IRGEN-NEXT:        PrStoreInst %5: undefined, %12: object, 1: number, "b": string, true: boolean
// IRGEN-NEXT:        PrStoreInst %5: undefined, %12: object, 2: number, "c": string, true: boolean
// IRGEN-NEXT:        PrStoreInst %5: undefined, %12: object, 3: number, "d": string, true: boolean
// IRGEN-NEXT:        PrStoreInst %5: undefined, %12: object, 4: number, "e": string, true: boolean
// IRGEN-NEXT:        PrStoreInst %5: undefined, %12: object, 5: number, "r": string, true: boolean
// IRGEN-NEXT:        StorePropertyLooseInst %12: object, %10: object, "obj2": string
// IRGEN-NEXT:  %20 = HBCAllocObjectFromBufferInst (:object) 13: number, "f": string, 1: number, "g": string, 1: number, "h": string, 1: number, "i": string, 1: number, "j": string, 1: number, "k": string, 1: number, "l": string, 1: number, "m": string, 1: number, "n": string, 1: number, "o": string, 1: number, "p": string, 1: number, "q": string, 1: number
// IRGEN-NEXT:        StoreNewOwnPropertyInst %5: undefined, %20: object, 1: number, true: boolean
// IRGEN-NEXT:        StorePropertyLooseInst %20: object, %10: object, "obj3": string
// IRGEN-NEXT:  %23 = HBCAllocObjectFromBufferInst (:object) 13: number, "f": string, 1: number, "g": string, 1: number, "h": string, 1: number, "i": string, 1: number, "j": string, 1: number, "k": string, 1: number, "l": string, 1: number, "m": string, 1: number, "n": string, 1: number, "o": string, 1: number, "p": string, 1: number, "q": string, 1: number
// IRGEN-NEXT:        StoreNewOwnPropertyInst %5: undefined, %23: object, 1: number, true: boolean
// IRGEN-NEXT:        StorePropertyLooseInst %23: object, %10: object, "obj4": string
// IRGEN-NEXT:        ReturnInst %5: undefined
// IRGEN-NEXT:function_end

// IRGEN:function f(): undefined
// IRGEN-NEXT:%BB0:
// IRGEN-NEXT:  %0 = HBCLoadConstInst (:undefined) undefined: undefined
// IRGEN-NEXT:       ReturnInst %0: undefined
// IRGEN-NEXT:function_end

// BCGEN:Bytecode File Information:
// BCGEN-NEXT:  Bytecode version number: {{.*}}
// BCGEN-NEXT:  Source hash: {{.*}}
// BCGEN-NEXT:  Function count: 2
// BCGEN-NEXT:  String count: 24
// BCGEN-NEXT:  BigInt count: 0
// BCGEN-NEXT:  String Kind Entry count: 2
// BCGEN-NEXT:  RegExp count: 0
// BCGEN-NEXT:  Segment ID: 0
// BCGEN-NEXT:  CommonJS module count: 0
// BCGEN-NEXT:  CommonJS module count (static): 0
// BCGEN-NEXT:  Function source count: 0
// BCGEN-NEXT:  Bytecode options:
// BCGEN-NEXT:    staticBuiltins: 0
// BCGEN-NEXT:    cjsModulesStaticallyResolved: 0

// BCGEN:Global String Table:
// BCGEN-NEXT:s0[ASCII, 0..5]: global
// BCGEN-NEXT:s1[ASCII, 6..10]: hello
// BCGEN-NEXT:i2[ASCII, 0..0] #00019A16: g
// BCGEN-NEXT:i3[ASCII, 4..4] #00018270: a
// BCGEN-NEXT:i4[ASCII, 5..5] #0001B6AD: l
// BCGEN-NEXT:i5[ASCII, 6..6] #0001A6E9: h
// BCGEN-NEXT:i6[ASCII, 10..10] #0001BA9E: o
// BCGEN-NEXT:i7[ASCII, 10..13] #2B6CD370: obj1
// BCGEN-NEXT:i8[ASCII, 12..12] #0001AECB: j
// BCGEN-NEXT:i9[ASCII, 14..14] #00018E43: b
// BCGEN-NEXT:i10[ASCII, 15..15] #00018A52: c
// BCGEN-NEXT:i11[ASCII, 16..16] #00019625: d
// BCGEN-NEXT:i12[ASCII, 17..17] #00019234: e
// BCGEN-NEXT:i13[ASCII, 18..18] #00019E07: f
// BCGEN-NEXT:i14[ASCII, 19..19] #0001A2F8: i
// BCGEN-NEXT:i15[ASCII, 20..20] #0001AADA: k
// BCGEN-NEXT:i16[ASCII, 21..21] #0001B2BC: m
// BCGEN-NEXT:i17[ASCII, 22..22] #0001BE8F: n
// BCGEN-NEXT:i18[ASCII, 23..26] #2B6CEF81: obj2
// BCGEN-NEXT:i19[ASCII, 27..30] #2B6CEB92: obj3
// BCGEN-NEXT:i20[ASCII, 31..34] #2B6CE7A3: obj4
// BCGEN-NEXT:i21[ASCII, 35..35] #0001C771: p
// BCGEN-NEXT:i22[ASCII, 36..36] #0001C360: q
// BCGEN-NEXT:i23[ASCII, 37..37] #0001CF53: r

// BCGEN:Literal Value Buffer:
// BCGEN-NEXT:[String 1]
// BCGEN-NEXT:[int 1]
// BCGEN-NEXT:null
// BCGEN-NEXT:null
// BCGEN-NEXT:true
// BCGEN-NEXT:null
// BCGEN-NEXT:[int 2]
// BCGEN-NEXT:null
// BCGEN-NEXT:null
// BCGEN-NEXT:null
// BCGEN-NEXT:null
// BCGEN-NEXT:null
// BCGEN-NEXT:null
// BCGEN-NEXT:[int 1]
// BCGEN-NEXT:[int 1]
// BCGEN-NEXT:[int 1]
// BCGEN-NEXT:[int 1]
// BCGEN-NEXT:[int 1]
// BCGEN-NEXT:[int 1]
// BCGEN-NEXT:[int 1]
// BCGEN-NEXT:[int 1]
// BCGEN-NEXT:[int 1]
// BCGEN-NEXT:[int 1]
// BCGEN-NEXT:[int 1]
// BCGEN-NEXT:[int 1]
// BCGEN-NEXT:Object Key Buffer:
// BCGEN-NEXT:[String 3]
// BCGEN-NEXT:[String 9]
// BCGEN-NEXT:[String 10]
// BCGEN-NEXT:[String 11]
// BCGEN-NEXT:[String 12]
// BCGEN-NEXT:[String 13]
// BCGEN-NEXT:[String 2]
// BCGEN-NEXT:[String 3]
// BCGEN-NEXT:[String 9]
// BCGEN-NEXT:[String 10]
// BCGEN-NEXT:[String 11]
// BCGEN-NEXT:[String 12]
// BCGEN-NEXT:[String 23]
// BCGEN-NEXT:[String 13]
// BCGEN-NEXT:[String 2]
// BCGEN-NEXT:[String 5]
// BCGEN-NEXT:[String 14]
// BCGEN-NEXT:[String 8]
// BCGEN-NEXT:[String 15]
// BCGEN-NEXT:[String 4]
// BCGEN-NEXT:[String 16]
// BCGEN-NEXT:[String 17]
// BCGEN-NEXT:[String 6]
// BCGEN-NEXT:[String 21]
// BCGEN-NEXT:[String 22]
// BCGEN-NEXT:[String 13]
// BCGEN-NEXT:[String 2]
// BCGEN-NEXT:[String 5]
// BCGEN-NEXT:[String 14]
// BCGEN-NEXT:[String 8]
// BCGEN-NEXT:[String 15]
// BCGEN-NEXT:[String 4]
// BCGEN-NEXT:[String 16]
// BCGEN-NEXT:[String 17]
// BCGEN-NEXT:[String 6]
// BCGEN-NEXT:[String 21]
// BCGEN-NEXT:[String 22]
// BCGEN-NEXT:Object Shape Table:
// BCGEN-NEXT:0[0, 7]
// BCGEN-NEXT:1[8, 18]
// BCGEN-NEXT:2[28, 12]
// BCGEN-NEXT:Function<global>(1 params, 4 registers):
// BCGEN-NEXT:Offset in debug table: source 0x0000, lexical 0x0000
// BCGEN-NEXT:    DeclareGlobalVar  "obj1"
// BCGEN-NEXT:    DeclareGlobalVar  "obj2"
// BCGEN-NEXT:    DeclareGlobalVar  "obj3"
// BCGEN-NEXT:    DeclareGlobalVar  "obj4"
// BCGEN-NEXT:    NewObjectWithBuffer r1, 7, 0, 0
// BCGEN-NEXT:    LoadConstUndefined r0
// BCGEN-NEXT:    PutOwnBySlotIdx   r1, r0, 3
// BCGEN-NEXT:    CreateTopLevelEnvironment r2, 0
// BCGEN-NEXT:    CreateClosure     r2, r2, Function<f>
// BCGEN-NEXT:    PutOwnBySlotIdx   r1, r2, 5
// BCGEN-NEXT:    GetGlobalObject   r2
// BCGEN-NEXT:    PutByIdLoose      r2, r1, 1, "obj1"
// BCGEN-NEXT:    NewObjectWithBuffer r1, 18, 1, 15
// BCGEN-NEXT:    PutOwnBySlotIdx   r1, r0, 0
// BCGEN-NEXT:    PutOwnBySlotIdx   r1, r0, 1
// BCGEN-NEXT:    PutOwnBySlotIdx   r1, r0, 2
// BCGEN-NEXT:    PutOwnBySlotIdx   r1, r0, 3
// BCGEN-NEXT:    PutOwnBySlotIdx   r1, r0, 4
// BCGEN-NEXT:    PutOwnBySlotIdx   r1, r0, 5
// BCGEN-NEXT:    PutByIdLoose      r2, r1, 2, "obj2"
// BCGEN-NEXT:    NewObjectWithBuffer r1, 13, 2, 16
// BCGEN-NEXT:    PutOwnByIndex     r1, r0, 1
// BCGEN-NEXT:    PutByIdLoose      r2, r1, 3, "obj3"
// BCGEN-NEXT:    NewObjectWithBuffer r1, 13, 2, 16
// BCGEN-NEXT:    PutOwnByIndex     r1, r0, 1
// BCGEN-NEXT:    PutByIdLoose      r2, r1, 4, "obj4"
// BCGEN-NEXT:    Ret               r0

// BCGEN:Function<f>(1 params, 1 registers):
// BCGEN-NEXT:    LoadConstUndefined r0
// BCGEN-NEXT:    Ret               r0

// BCGEN:Debug filename table:
// BCGEN-NEXT:  0: {{.*}}hbc_object_literals.js

// BCGEN:Debug file table:
// BCGEN-NEXT:  source table offset 0x0000: filename id 0

// BCGEN:Debug source table:
// BCGEN-NEXT:  0x0000  function idx 0, starts at line 11 col 1
// BCGEN-NEXT:    bc 0: line 11 col 1
// BCGEN-NEXT:    bc 5: line 11 col 1
// BCGEN-NEXT:    bc 10: line 11 col 1
// BCGEN-NEXT:    bc 15: line 11 col 1
// BCGEN-NEXT:    bc 51: line 11 col 10
// BCGEN-NEXT:    bc 89: line 15 col 10
// BCGEN-NEXT:    bc 103: line 37 col 12
// BCGEN-NEXT:    bc 107: line 37 col 10
// BCGEN-NEXT:    bc 121: line 53 col 12
// BCGEN-NEXT:    bc 125: line 53 col 10
// BCGEN-NEXT:  0x0022  end of debug source table

// BCGEN:Debug lexical table:
// BCGEN-NEXT:  0x0000  lexical parent: none, variable count: 0
// BCGEN-NEXT:  0x0002  end of debug lexical table
