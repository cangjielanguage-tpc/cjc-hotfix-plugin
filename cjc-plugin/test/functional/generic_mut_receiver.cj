// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
interface Increment { mut func increment(): Int64 }
@patchable
struct Counter <: Increment {
    var value: Int64 = 0
    public mut func increment(): Int64 { value++; value }
}
@patchable
func advance<T>(initial: T): Int64 where T <: Increment {
    var local = initial
    local.increment()
    local.increment()
}
@patchable
func doTest(): Int64 {
    if (advance<Counter>(Counter()) != 2) { return 1 }
    0
}

main(): Int64 {
    let result = doTest()
    if (result != 0) { return 1 }
    println("generic_mut_receiver passed")
    0
}
