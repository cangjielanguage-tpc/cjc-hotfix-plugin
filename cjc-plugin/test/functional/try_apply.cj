// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
func mayThrow(flag: Bool): Int64 {
    if (flag) { throw Exception("expected") }
    7
}
@patchable
func doTest(): Int64 {
    var result = 0
    try { result = mayThrow(false) } catch (_: Exception) { return 1 }
    try { mayThrow(true); return 2 } catch (e: Exception) { result += 3 } finally { result += 1 }
    if (result != 11) { return 3 }
    0
}

main(): Int64 {
    let result = doTest()
    if (result != 0) { return 1 }
    println("try_apply passed")
    0
}
