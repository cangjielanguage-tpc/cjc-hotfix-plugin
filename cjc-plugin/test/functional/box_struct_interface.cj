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
func foo(x: I) {
    match (x) {
        case v: S => v.a 
        case _ => S(1).a
    }
    return 0
}

@patchable
func exercise(): Int64 {
    foo(S(1))
    0
}

main(): Int64 {
    let result = exercise()
    if (result != 0) { return 1 }
    println("box_struct_interface passed")
    0
}
