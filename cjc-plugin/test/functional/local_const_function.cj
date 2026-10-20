// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
class K {
    public static let z = { =>
        const func f():Int64 {
            return 2
        }
        f()
    }
}

@patchable
func doTest() {
    return K.z() - 2
}

main(): Int64 {
    let result = doTest()
    if (result != 0) { return 1 }
    println("local_const_function passed")
    0
}
