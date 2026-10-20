// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
func highOrderF(fVal: (Int64, Nothing) -> Int64) {
    return 2
}

@patchable
func highOrderF1(fVal: (Nothing, Nothing) -> Int64) {
    return 2
}

@patchable
func doTest() {
    let lambdaVal = {
        x: Int64, y: Int64 => x + y
    }
    highOrderF(lambdaVal)
    highOrderF(lambdaVal)
    highOrderF1(lambdaVal)
    highOrderF1(lambdaVal)

    0
}

main(): Int64 {
    let result = doTest()
    if (result != 0) { return 1 }
    println("lambda_nothing_wrapper passed")
    0
}
