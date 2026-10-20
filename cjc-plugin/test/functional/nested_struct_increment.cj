// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
struct S0 {
    var t0: Int64 = 9
    init() {}
}

@patchable
struct S1 {
    var t1: S0 = S0()
    init() {}
}

@patchable
struct S2 {
    var t2: S1 = S1()
    init() {}
}

@patchable
func exercise(): Unit {
    var s2 = S2()
    s2.t2.t1.t0++
    println(s2.t2.t1.t0.toString())
}

main(): Int64 {
    exercise()
    println("nested_struct_increment passed")
    0
}
