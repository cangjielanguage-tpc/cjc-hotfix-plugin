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
class Foo {
    func g() {}
}

extend Foo <: I {
    @patchable
    public func f() {
        11
    }
}

@patchable
func doTest() {
    var i = (10, Foo())
    var res = match (i) {
        case (10, d: I) => d.f()
        case _ => 100
    }

    if (res == 11) {
        return 0
    } else {
        return 1
    }
}

main(): Int64 {
    let result = doTest()
    if (result != 0) { return 1 }
    println("pattern_interface_tuple passed")
    0
}
