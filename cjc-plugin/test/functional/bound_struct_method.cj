// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
func call(v1: () -> Unit): Unit {
    v1()
}

@patchable
interface DefaultImpl {
    func test(): Unit {
        println("Hello, World!")
    }
}

@patchable
struct A <: DefaultImpl {}

@patchable
func doTest(): Unit {
    call(A().test)
}

main(): Int64 {
    doTest()
    println("bound_struct_method passed")
    0
}
