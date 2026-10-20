// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

let x99abc: Int64 = 100
var y: Int64 = 100

@patchable
func exercise() {
    x99abc - y
}

main(): Int64 {
    let result = exercise()
    if (result != 0) { return 1 }
    println("global_constant passed")
    0
}
