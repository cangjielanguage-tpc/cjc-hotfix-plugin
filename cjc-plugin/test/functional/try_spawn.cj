// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
func run(fail: Bool): Int64 {
    try {
        let future = spawn {
            if (fail) { throw Exception("expected") }
            7
        }
        future.get()
    } catch (_: Exception) { -1 }
}
@patchable
func exercise(): Int64 {
    if (run(false) != 7 || run(true) != -1) { return 1 }
    0
}

main(): Int64 {
    let result = exercise()
    if (result != 0) { return 1 }
    println("try_spawn passed")
    0
}
