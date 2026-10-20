// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
open class A {
    public static func f() {
        return 11
    }

    func bar() {
        f()
    }

    static func baz() {
        f()
    }
}

@patchable
class C2 <: A {
    public static redef func f() {
        return 12
    }
}

@patchable
func exercise() {
    let a: A = C2()
    println(a.bar())

    println(A.f())
    println(C2.baz())
}

main(): Int64 {
    exercise()
    println("invoke_static_instance_context passed")
    0
}
