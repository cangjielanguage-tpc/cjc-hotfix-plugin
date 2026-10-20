// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
struct SA {
    var x = 2
}

@patchable
open class C<T> {
    public open func foo(param1: T, param2: SA): SA {
        SA()
    }
}

@patchable
class D <: C<SA> {
    public func foo(param1: SA, param2: SA): SA {
        SA()
    }
}

@patchable
func doTest(): Int64 {
    let value: C<SA> = D()
    if (value.foo(SA(), SA()).x != 2 || C<Unit>().foo((), SA()).x != 2) { return 1 }
    0
}

main(): Int64 {
    let result = doTest()
    if (result != 0) { return 1 }
    println("vtable_value_parameter_wrapper passed")
    0
}
