// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
func run(a: Int64, b: Int64): Int64 {
    a * b + a / b + a % b + a ** UInt64(b) + (a << b) + (a >> b)
}
main(): Int64 {
    if (run(6, 2) != 76) { return 1 }
    println("ordinary_arithmetic passed")
    0
}
