// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
func narrow(n: Int64): Int32 {
    try { Int32(n) } catch (_: OverflowException) { -1 }
}
@patchable
func exercise(): Int64 {
    if (narrow(12) != 12 || narrow(Int64.Max) != -1) { return 1 }
    0
}

main(): Int64 {
    let result = exercise()
    if (result != 0) { return 1 }
    println("try_numeric_cast passed")
    0
}
