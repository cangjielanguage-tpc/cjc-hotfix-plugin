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
func exercise() {
    var a: VArray<Int64, $2> = [1, 2]
    a[foo1()] = a[foo2()]
    if (a[1] != 1) {
        return 1
    }
    0
}

main(): Int64 {
    let result = exercise()
    if (result != 0) { return 1 }
    println("varray_writeback passed")
    0
}
