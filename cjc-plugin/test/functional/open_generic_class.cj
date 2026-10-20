// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
abstract class A<T> {
    public open func foo<S>() {
    }
    public func bar<S>(a: S): Unit where S <: ToString
}

@patchable
class B<T> <: A<T> {
    public func bar<M>(a: M): Unit where M <: ToString {
        println(a.toString())
    }

    public func foo<S>() {
        println(3)
    }
}

@patchable
func exercise() {
    B<Int32>().foo<Int64>()
    B<Int32>().bar<Int32>(3)

    let a: A<Int32> = B<Int32>()
    a.bar<Int64>(7)

    0
}

main(): Int64 {
    let result = exercise()
    if (result != 0) { return 1 }
    println("open_generic_class passed")
    0
}
