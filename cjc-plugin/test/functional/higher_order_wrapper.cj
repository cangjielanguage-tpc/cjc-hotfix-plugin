// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
func highOrderF(fVal: (Int64, Int64) -> (Int64, Nothing) -> Int64) {
    return 2
}

@patchable
func bar(a: Int64, b: Int64): Int64 {
    return 1
}

@patchable
func foo(x: Int64, b: Int64): (Int64, Int64) -> Int64 {
    return bar
}

@patchable
func exercise() {
    let lambdaVal = {
        x: Int64, y: Int64 => bar
    }
    highOrderF(foo)
    highOrderF(lambdaVal)
    0
}

main(): Int64 {
    let result = exercise()
    if (result != 0) { return 1 }
    println("higher_order_wrapper passed")
    0
}
