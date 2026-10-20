// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
class Item { let value: Int64; init(value: Int64) { this.value = value } }
@patchable
func allocate(size: Int64): Int64 {
    try {
        let item = Item(3)
        let array = Array<Int64>(size, repeat: item.value)
        array[0]
    } catch (_: Exception) { -1 }
}
@patchable
func doTest(): Int64 {
    if (allocate(2) != 3 || allocate(0) != -1) { return 1 }
    0
}

main(): Int64 {
    let result = doTest()
    if (result != 0) { return 1 }
    println("try_allocate_intrinsic passed")
    0
}
