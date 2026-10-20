// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
open class Base {}
@patchable
class Derived <: Base {}
@patchable
struct Value { let n = 7 }
@patchable
func isValue<T>(value: T): Bool { value is Value }
@patchable
func doTest(): Int64 {
    let value: Any = Derived()
    if (!(value is Base) || (value as Derived).isNone() || (value as String).isSome()) { return 1 }
    if (!isValue<Value>(Value()) || isValue<Int64>(3)) { return 2 }
    0
}

main(): Int64 {
    let result = doTest()
    if (result != 0) { return 1 }
    println("casts_rtti passed")
    0
}
