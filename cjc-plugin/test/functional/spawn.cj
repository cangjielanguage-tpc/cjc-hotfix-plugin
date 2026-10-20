// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
func run(seed: Int64): Int64 {
    let future = spawn { seed + 1 }
    future.get()
}

main(): Int64 {
    if (run(3) != 4) { return 1 }
    println("spawn passed")
    0
}
