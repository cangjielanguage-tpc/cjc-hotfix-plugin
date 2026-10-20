// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
enum E {
    A | B(S, Bool)
}

@patchable
struct S {
    var x: E = E.A
}

@patchable
func foo(e: E) {
    match (e) {
        case A => println(100)
        case B(s, x) => println(200)
    }
    return 0
}

@patchable
func exercise() {
    foo(E.B(S(), true))
    return 0
}

main(): Int64 {
    let result = exercise()
    if (result != 0) { return 1 }
    println("box_recursive_enum passed")
    0
}
