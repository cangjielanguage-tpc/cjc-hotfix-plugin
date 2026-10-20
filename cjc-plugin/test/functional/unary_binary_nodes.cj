// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
func calculate(a: Int64, b: Int64): Int64 {
    let bits = ((a | b) & 7) ^ b
    if (!(a < b) && a >= b && a != b && b <= a && a > 0) { return bits + (-a) }
    99
}
@patchable
func doTest(): Int64 {
    let complement = !Int64(0)
    if (calculate(6, 3) != -2 || complement != -1 || (false || true) != true) { return 1 }
    0
}

main(): Int64 {
    let result = doTest()
    if (result != 0) { return 1 }
    println("unary_binary_nodes passed")
    0
}
