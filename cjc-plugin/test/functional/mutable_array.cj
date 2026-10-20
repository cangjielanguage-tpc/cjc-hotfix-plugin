// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
func foo(x: Array<Int64>) {
    x[0] = 5
}

@patchable
func doTest() {
    let a0 = Array<Int64>(1, repeat: 1)
    print(a0[0].toString()) 
    a0[0] = 2
    print(a0[0].toString()) 
    var a1 = Array<Int64>(1, repeat: 3)
    a1 = a0
    print(a1[0].toString()) 
    a0[0] = 4
    print(a1[0].toString()) 
    foo(a1)
    print(a0[0].toString()) 
    0
}

main(): Int64 {
    let result = doTest()
    if (result != 0) { return 1 }
    println("mutable_array passed")
    0
}
