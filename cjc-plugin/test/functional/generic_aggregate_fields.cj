// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
class Ref<T> {
    var value: T
    init(value: T) { this.value = value }
}
@patchable
struct Pair<T, U> {
    var first: T
    var second: U
    init(first: T, second: U) { this.first = first; this.second = second }
}
@patchable
func swap<T, U>(p: Pair<T, U>): Pair<U, T> { Pair<U, T>(p.second, p.first) }
@patchable
func doTest(): Int64 {
    let ref = Ref<Int64>(7)
    let result = swap<Int64, Ref<Int64>>(Pair<Int64, Ref<Int64>>(3, ref))
    if (result.first.value != 7 || result.second != 3) { return 1 }
    0
}

main(): Int64 {
    let result = doTest()
    if (result != 0) { return 1 }
    println("generic_aggregate_fields passed")
    0
}
