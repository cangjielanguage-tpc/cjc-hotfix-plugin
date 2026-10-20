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

    func foo() {
        t0 = 10
    }
}

@patchable
struct R {
    var t0: Int64
    let t1: C

    init(t0_: Int64, t1_: C) {
        t0 = t0_
        t1 = t1_
    }

    mut func foo() {
        t0 = 100
        t1.t0 = 1000
    }
}

@patchable
func exercise(): Int64 {
    var c0 = C(1)
    var r0 = R(2, c0)
    print(r0.t0.toString())
    print(r0.t1.t0.toString())
    c0.foo()
    print(c0.t0.toString())
    print(r0.t1.t0.toString())
    r0.foo()
    print(c0.t0.toString())
    print(r0.t0.toString())
    print(r0.t1.t0.toString())
    0
}

main(): Int64 {
    let result = exercise()
    if (result != 0) { return 1 }
    println("mut_receiver passed")
    0
}
