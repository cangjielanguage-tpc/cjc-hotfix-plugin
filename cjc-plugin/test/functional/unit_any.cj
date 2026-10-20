// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
func foo(a: ?Int): Any {
    match (a) {
        case Some(v) => println(v)
        case None => 3
    }
}

main(): Int64 {
    foo(Some(1))
    println("unit_any passed")
    0
}
