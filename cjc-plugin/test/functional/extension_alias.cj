// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
interface I {
    func f(): Int64
}

@patchable
class C {}

type c = C

extend c <: I {
    @patchable
    public func f() {
        2
    }
}

@patchable
func exercise() {
    var a = c()
    return a.f() - 2  
}

main(): Int64 {
    let result = exercise()
    if (result != 0) { return 1 }
    println("extension_alias passed")
    0
}
