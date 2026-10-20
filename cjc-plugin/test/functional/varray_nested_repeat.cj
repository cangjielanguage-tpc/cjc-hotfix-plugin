// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

var glb = 0

@patchable
func foo1() {
    glb += 3
    print(glb)
    return 1
}

@patchable
func foo2() {
    glb += 2
    print(glb)
    return 0
}

@patchable
func doTest() {
    let a: VArray<Int64, $2> = [1, 2]
    let arr = VArray<VArray<Int64, $2>, $5>(repeat: a)
    let c = arr[foo1()][foo2()]
    0
}

main(): Int64 {
    let result = doTest()
    if (result != 0) { return 1 }
    println("varray_nested_repeat passed")
    0
}
