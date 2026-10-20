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
class Foo {}

extend Foo <: I {
    @patchable
    public func f() {
        return 1
    }
}

@patchable
func g(i: I) {
    i.f() 
}

@patchable
func doTest() {
    let a = Foo()
    g(a) - 1
}

main(): Int64 {
    let result = doTest()
    if (result != 0) { return 1 }
    println("extension_interface passed")
    0
}
