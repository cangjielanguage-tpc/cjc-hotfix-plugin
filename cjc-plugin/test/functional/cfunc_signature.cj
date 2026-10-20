// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@C
@patchable
struct Pair { var a: Int64 = 2; var b: Int64 = 3 }
@C
@patchable
func add(p: Pair): Int64 { p.a + p.b }
@patchable
func doTest(): Int64 {
    let call: CFunc<(Pair) -> Int64> = {p => p.a + p.b}
    if (unsafe { add(Pair()) } != 5 || unsafe { call(Pair()) } != 5) { return 1 }
    0
}

main(): Int64 {
    let result = doTest()
    if (result != 0) { return 1 }
    println("cfunc_signature passed")
    0
}
