// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
interface interfaceA {
    func a(a: Int64) {
        a
    }
}

@patchable
class B {}

extend B <: interfaceA {}

@patchable
func exercise(): Int64 {
    var a: B = B()
    if (a.a(1) == 1) {      
        return 0
    } else {
        return 1
    }
}

main(): Int64 {
    let result = exercise()
    if (result != 0) { return 1 }
    println("extension_default passed")
    0
}
