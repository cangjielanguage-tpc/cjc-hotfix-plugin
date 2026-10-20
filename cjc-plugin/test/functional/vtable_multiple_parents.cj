// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
open class AAA {
    public func foo1(): Int64 {
        1
    }
}

@patchable
interface III {
    func foo1(): Int64
}

@patchable
class BBB <: AAA & III {}

@patchable
func exercise(): Int64 {
    let value: III = BBB()
    if (value.foo1() != 1) { return 1 }
    0
}

main(): Int64 {
    let result = exercise()
    if (result != 0) { return 1 }
    println("vtable_multiple_parents passed")
    0
}
