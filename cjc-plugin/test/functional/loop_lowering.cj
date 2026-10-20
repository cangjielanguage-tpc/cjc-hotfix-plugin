// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
func sum(values: Array<Int64>): Int64 {
    var total = 0
    for (i in 0..values.size) { total += values[i] }
    for (i in 1..=3) { total += i }
    for (value in values) { total += value }
    var i = 0
    while (i < 3) { i++; if (i == 2) { continue }; total += i }
    do { total--; break } while (true)
    total
}
@patchable
func doTest(): Int64 {
    if (sum([1, 2]) != 15) { return 1 }
    0
}

main(): Int64 {
    let result = doTest()
    if (result != 0) { return 1 }
    println("loop_lowering passed")
    0
}
