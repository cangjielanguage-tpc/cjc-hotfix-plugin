// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
struct C {
    var x: Int64 = 0
    mut prop p: Int64 {
        get() {
            x
        }
        set(v) {
            x = v
        }
    }
}

@patchable
func exercise() {
    var x = C()
    x.p = 10
    x.p - 10
}

main(): Int64 {
    let result = exercise()
    if (result != 0) { return 1 }
    println("mut_property passed")
    0
}
