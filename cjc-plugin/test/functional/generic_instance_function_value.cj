// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
public interface I<T> {
    func add(a: T): Int64
}

@patchable
class A <: I<A> {
    public func add(a: A): Int64 {
        return 1
    }
}

@patchable
class B<T> where T <: I<T> {
    func foo(a: T) {
        
        return a.add
    }
}

@patchable
func exercise(): Int64 {
    let a = A()
    if (B<A>().foo(a)(a) != 1) { return 1 }
    0
}

main(): Int64 {
    let result = exercise()
    if (result != 0) { return 1 }
    println("generic_instance_function_value passed")
    0
}
