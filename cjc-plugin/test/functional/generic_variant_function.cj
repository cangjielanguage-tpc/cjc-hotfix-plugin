// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
abstract class A<T> {
    public func bar<S>(a: S): S where S <: ToString
}

@patchable
class B<T> <: A<T> {
    public func bar<M>(a: M): M where M <: ToString {
        a
    }
}

@patchable
class F <: ToString {
    var i = 0
    public func toString() {
        "F(${i})"
    }
}

@patchable
func exercise() {
    let r: (F) -> ToString = B<Int32>().bar<F>
    let f = F()
    f.i = 3
    println(r(f))
    0
}

main(): Int64 {
    let result = exercise()
    if (result != 0) { return 1 }
    println("generic_variant_function passed")
    0
}
