// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
func build(seed: Int64): Int64 {
    let values = VArray<Int64, $3> {i => seed + i}
    values[0] + values[1] + values[2]
}

main(): Int64 {
    if (build(2) != 9) { return 1 }
    println("varray_builder passed")
    0
}
