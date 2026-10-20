// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
func exercise(): Int64 {
    let literal = [1, 2, 3]
    let repeat = Array<Int64>(3, repeat: 4)
    let built = Array<Int64>(3, {i => i + 1})
    if (literal[2] != 3 || repeat[2] != 4 || built[2] != 3) { return 1 }
    0
}

main(): Int64 {
    let result = exercise()
    if (result != 0) { return 1 }
    println("array_construction passed")
    0
}
