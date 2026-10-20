// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
interface Value { func get(): Int64 }
@patchable
class Item<T> { let value: T; init(value: T) { this.value = value } }
extend<T> Item<T> <: Value where T <: ToString {
    @patchable
    public func get(): Int64 { value.toString().size }
}
@patchable
func read(x: Value): Int64 { x.get() }
@patchable
func doTest(): Int64 {
    if (read(Item<Int64>(123)) != 3 || read(Item<String>("abcd")) != 4) { return 1 }
    0
}

main(): Int64 {
    let result = doTest()
    if (result != 0) { return 1 }
    println("extension_constrained passed")
    0
}
