// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
func run(value: Int64): Int64 {
    try { let values = [value, value + 1]; values[0] } catch (_: Exception) { -1 }
}
main(): Int64 {
    if (run(7) != 7) { return 1 }
    println("exceptional_array_literal passed")
    0
}
