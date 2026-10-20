// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
open class Base {
    var a = 1
    func foo() {
        a++
    }
}

@patchable
class A <: Base {}
@patchable
class B <: Base {}

@patchable
func foo<T>(a: T) where T <: Base {
    var b = a
    b.foo()
    return b.a
}
 
@patchable
func exercise() {
    var a: A = A()
    var b: B = B()
    return foo<Base>(a) - foo<B>(b) + 8
}

main(): Int64 {
    let result = exercise()
    if (result != 8) { return 1 }
    println("apply_constrained passed")
    0
}
