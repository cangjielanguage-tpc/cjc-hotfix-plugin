// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
func foo<T>(): Int64 {
    1
}

@patchable
func goo1<U>(): Int64 {
    var x = foo<U>
    x()
    let y = foo<Int64>
    y()
}

@patchable
func goo2<U>(): Int64 {
    var y = foo<Int32>
    y()
    var x = foo<U>
    x()
}

@patchable
func exercise() {
    goo1<Int32>() + goo2<Int64>()
}

main(): Int64 {
    let result = exercise()
    if (result != 2) { return 1 }
    println("generic_global_function_value passed")
    0
}
