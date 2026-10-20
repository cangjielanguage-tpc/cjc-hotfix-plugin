// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

extend Int64 {
    @patchable
    static func test() {
        7
    }
}

@patchable
func exercise() {
    var x = Int64.test
    return x() - 7
}

main(): Int64 {
    let result = exercise()
    if (result != 0) { return 1 }
    println("extension_static_value passed")
    0
}
