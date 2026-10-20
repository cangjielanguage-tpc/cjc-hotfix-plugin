// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
class CA {}

@patchable
struct SA {
    var x = 2
}

@patchable
open class C<T> {
    let x: T
    init(a: T) {
        x = a
    }
    public open func foo(param1: SA, param2: SA): T {
        x
    }
}

@patchable
class D <: C<SA> {
    init() {
        super(SA())
    }
    public func foo(param1: SA, param2: SA): SA {
        SA()
    }
}

@patchable
func exercise(): Int64 {
    let value: C<SA> = D()
    if (value.foo(SA(), SA()).x != 2 || C<Int64>(3).foo(SA(), SA()) != 3) { return 1 }
    0
}

main(): Int64 {
    let result = exercise()
    if (result != 0) { return 1 }
    println("vtable_value_return_wrapper passed")
    0
}
