// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
func foo<T>(x: T): Int32 where T <: A {
    return x.n + x.m
}

@patchable
interface A {
    public prop m: Int32
    public prop n: Int32
}

extend Int64 <: A {
    public prop m: Int32 {
        get() {
            return 2
        }
    }
    public prop n: Int32 {
        get() {
            return 1
        }
    }
}

main(): Int64 {
    if (foo(1) != 3) {
        return 1
    }
    println("generic_property passed")
    0
}
