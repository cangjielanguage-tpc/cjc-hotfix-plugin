// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
class CA {
    var a: Int64
    init(x: Int64) {
        a = x
    }
}

@patchable
func foo<T>(x: T): Int64 {
    let res = match (x) {
        case val: CA => val.a
        case _ => CA(-1).a
    }
    return res
}

@patchable
func doTest(): Int64 {
    if (foo<CA>(CA(7)) != 7) { return 1 }
    if (foo<Int64>(0) != -1) { return 2 }
    0
}

main(): Int64 {
    let result = doTest()
    if (result != 0) { return 1 }
    println("box_generic_class passed")
    0
}
