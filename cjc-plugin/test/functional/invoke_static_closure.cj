// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
interface H {
    static func foo(): Unit
}

@patchable
class R <: H {
    public static func foo() {
        println("R::foo")
    }
}

@patchable
class C<T> where T <: H {
    init() {
        let b = {=> T.foo()}
        b()
    }
}

main(): Int64 {
    C<R>()
    println("invoke_static_closure passed")
    0
}
