// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
class AAA<XXX, YYY> {
    public let val: Int64 = 1
}

@patchable
func foo1<TTT>(x: TTT) {
    var y = AAA<TTT, TTT>()
    var lambda = {=> y}()
    lambda
}

@patchable
func exercise() {
    foo1<Int64>(1).val
}

main(): Int64 {
    let result = exercise()
    if (result != 1) { return 1 }
    println("generic_nested_closure passed")
    0
}
