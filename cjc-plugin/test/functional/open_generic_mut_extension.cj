// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
interface I<T> {
    mut func foo<S>() {
        println("I<T>::foo<S>")
    }
    mut func bar<S>(a: S): S
}

@patchable
struct FOO<T> {}

extend<R> FOO<R> <: I<R> {
    @patchable
    public mut func foo<S>() {
        println("FOO<T>::foo<S>")
    }

    @patchable
    public mut func bar<S>(a: S): S {
        println("FOO<T>::bar<S>")
        a
    }
}

@patchable
func exercise() {
    let b: I<Bool> = FOO<Bool>()
    b.foo<Int64>()
    b.bar<Int32>(7)

    0
}

main(): Int64 {
    let result = exercise()
    if (result != 0) { return 1 }
    println("open_generic_mut_extension passed")
    0
}
