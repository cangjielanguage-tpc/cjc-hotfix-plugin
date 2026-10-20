// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
enum Choice { A | B(Int64) | C }
@patchable
func classify(n: Int64): Int64 {
    match (n) {
        case 0 => 10
        case 1 => 20
        case 2 => 30
        case _ => 40
    }
}
@patchable
func payload(c: Choice): Int64 {
    match (c) { case A => 1; case B(n) => n; case C => 3 }
}
@patchable
func classifyUnion(n: Int64): Int64 {
    match (n) { case 0 | 1 | 2 => 7; case _ => 9 }
}

main(): Int64 {
    if (classifyUnion(0) != 7 || classifyUnion(2) != 7 || classifyUnion(4) != 9) { return 2 }
    if (classify(0) + classify(1) + classify(2) + classify(9) != 100 ||
        payload(A) + payload(B(7)) + payload(C) != 11) { return 1 }
    println("multibranch passed")
    0
}
