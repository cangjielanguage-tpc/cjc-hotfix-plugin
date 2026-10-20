// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
interface I {
    func foo() {}
}

@patchable
struct S <: I {
    var a: Int64
    init(x: Int64) {
        a = x
    }
}

@patchable
class Other <: I {}

@patchable
func foo(x: I): Int64 {
    match (x) {
        case v: S => v.a 
        case _ => S(1).a
    }
}

@patchable
func doTest(): Int64 {
    if (foo(S(7)) != 7 || foo(Other()) != 1) { return 1 }
    0
}

main(): Int64 {
    let result = doTest()
    if (result != 0) { return 1 }
    println("box_struct_interface passed")
    0
}
