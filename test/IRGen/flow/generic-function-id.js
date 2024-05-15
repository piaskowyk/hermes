/**
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

// RUN: %shermes -O0 -typed -dump-ir %s | %FileCheckOrRegen %s --match-full-lines

// Make sure two specializations are generated for number and string.
// TODO: Original "any" version still exists during IRGen,
// should probably just be deleted.
function id<T>(x: T): T {
  return x;
}

id<number>(1);
id<string>('a');

// Auto-generated content below. Please do not modify manually.

// CHECK:scope %VS0 []

// CHECK:function global(): any
// CHECK-NEXT:%BB0:
// CHECK-NEXT:  %0 = CreateScopeInst (:environment) %VS0: any, empty: any
// CHECK-NEXT:  %1 = AllocStackInst (:any) $?anon_0_ret: any
// CHECK-NEXT:       StoreStackInst undefined: undefined, %1: any
// CHECK-NEXT:  %3 = CreateFunctionInst (:object) %0: environment, %""(): functionCode
// CHECK-NEXT:  %4 = AllocObjectLiteralInst (:object) empty: any
// CHECK-NEXT:  %5 = CallInst [njsf] (:any) %3: object, empty: any, empty: any, undefined: undefined, undefined: undefined, %4: object
// CHECK-NEXT:       StoreStackInst %5: any, %1: any
// CHECK-NEXT:  %7 = LoadStackInst (:any) %1: any
// CHECK-NEXT:       ReturnInst %7: any
// CHECK-NEXT:function_end

// CHECK:scope %VS1 [exports: any, id: any, id#1: any, id#2: any]

// CHECK:function ""(exports: any): any
// CHECK-NEXT:%BB0:
// CHECK-NEXT:  %0 = GetParentScopeInst (:environment) %VS0: any, %parentScope: environment
// CHECK-NEXT:  %1 = CreateScopeInst (:environment) %VS1: any, %0: environment
// CHECK-NEXT:  %2 = LoadParamInst (:any) %exports: any
// CHECK-NEXT:       StoreFrameInst %1: environment, %2: any, [%VS1.exports]: any
// CHECK-NEXT:  %4 = CreateFunctionInst (:object) %1: environment, %id(): functionCode
// CHECK-NEXT:       StoreFrameInst %1: environment, %4: object, [%VS1.id#1]: any
// CHECK-NEXT:  %6 = CreateFunctionInst (:object) %1: environment, %"id 1#"(): functionCode
// CHECK-NEXT:       StoreFrameInst %1: environment, %6: object, [%VS1.id#2]: any
// CHECK-NEXT:  %8 = LoadFrameInst (:any) %1: environment, [%VS1.id#1]: any
// CHECK-NEXT:  %9 = CheckedTypeCastInst (:object) %8: any, type(object)
// CHECK-NEXT:  %10 = CallInst [njsf] (:any) %9: object, empty: any, empty: any, undefined: undefined, undefined: undefined, 1: number
// CHECK-NEXT:  %11 = CheckedTypeCastInst (:number) %10: any, type(number)
// CHECK-NEXT:  %12 = LoadFrameInst (:any) %1: environment, [%VS1.id#2]: any
// CHECK-NEXT:  %13 = CheckedTypeCastInst (:object) %12: any, type(object)
// CHECK-NEXT:  %14 = CallInst [njsf] (:any) %13: object, empty: any, empty: any, undefined: undefined, undefined: undefined, "a": string
// CHECK-NEXT:  %15 = CheckedTypeCastInst (:string) %14: any, type(string)
// CHECK-NEXT:        ReturnInst undefined: undefined
// CHECK-NEXT:function_end

// CHECK:scope %VS2 [x: any]

// CHECK:function id(x: number): any [typed]
// CHECK-NEXT:%BB0:
// CHECK-NEXT:  %0 = GetParentScopeInst (:environment) %VS1: any, %parentScope: environment
// CHECK-NEXT:  %1 = CreateScopeInst (:environment) %VS2: any, %0: environment
// CHECK-NEXT:  %2 = LoadParamInst (:number) %x: number
// CHECK-NEXT:       StoreFrameInst %1: environment, %2: number, [%VS2.x]: any
// CHECK-NEXT:  %4 = LoadFrameInst (:any) %1: environment, [%VS2.x]: any
// CHECK-NEXT:  %5 = CheckedTypeCastInst (:number) %4: any, type(number)
// CHECK-NEXT:       ReturnInst %5: number
// CHECK-NEXT:function_end

// CHECK:scope %VS3 [x: any]

// CHECK:function "id 1#"(x: string): any [typed]
// CHECK-NEXT:%BB0:
// CHECK-NEXT:  %0 = GetParentScopeInst (:environment) %VS1: any, %parentScope: environment
// CHECK-NEXT:  %1 = CreateScopeInst (:environment) %VS3: any, %0: environment
// CHECK-NEXT:  %2 = LoadParamInst (:string) %x: string
// CHECK-NEXT:       StoreFrameInst %1: environment, %2: string, [%VS3.x]: any
// CHECK-NEXT:  %4 = LoadFrameInst (:any) %1: environment, [%VS3.x]: any
// CHECK-NEXT:  %5 = CheckedTypeCastInst (:string) %4: any, type(string)
// CHECK-NEXT:       ReturnInst %5: string
// CHECK-NEXT:function_end
