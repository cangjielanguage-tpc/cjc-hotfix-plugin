// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
class C {
    var t0: Int64

    init(t0_: Int64) {
        t0 = t0_
    }
}

@patchable
func foo(x: C) {
    x.t0 = 5
}

@patchable
func doTest() {
    let c0 = C(1)
    print(c0.t0.toString())
    c0.t0 = 2
    print(c0.t0.toString())
    var c1 = C(3)
    c1 = c0
    print(c1.t0.toString())
    c0.t0 = 4
    print(c1.t0.toString())
    foo(c1)
    print(c0.t0.toString())
    0
}

main(): Int64 {
    let result = doTest()
    if (result != 0) { return 1 }
    println("mutable_class passed")
    0
}
