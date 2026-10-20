// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
class C0 {
    var t0: Int64 = 0
    init() {}
}

@patchable
class C1 {
    var t1: C0 = C0()
    init() {}
}

@patchable
class C2 {
    var t2: C1 = C1()
    init() {}
}

@patchable
func doTest(): Unit {
    let c2 = C2()
    c2.t2.t1.t0 += 10
    println(c2.t2.t1.t0.toString())
}

main(): Int64 {
    doTest()
    println("nested_class_store passed")
    0
}
