// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
struct R {
    var t0: Int64

    init(t0_: Int64) {
        t0 = t0_
    }
}

@patchable
func exercise() {
    let r0 = R(1)
    print(r0.t0.toString())
    var r1 = R(2)
    print(r1.t0.toString())
    r1 = r0
    print(r1.t0.toString())
    r1.t0 = 4
    print(r0.t0.toString())
    print(r1.t0.toString())
    0
}

main(): Int64 {
    let result = exercise()
    if (result != 0) { return 1 }
    println("mutable_struct passed")
    0
}
