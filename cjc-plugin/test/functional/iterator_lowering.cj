// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
class Values <: Iterable<Int64> {
    public func iterator(): Iterator<Int64> { ValuesIterator() }
}
@patchable
class ValuesIterator <: Iterator<Int64> {
    var n: Int64 = 0
    public func next(): ?Int64 {
        n++
        if (n <= 3) { Some(n) } else { None }
    }
}
@patchable
func run(values: Iterable<Int64>): Int64 {
    var n = 0
    for (value in values) { n += value }
    n
}
main(): Int64 {
    if (run(Values()) != 6) { return 1 }
    println("iterator_lowering passed")
    0
}
