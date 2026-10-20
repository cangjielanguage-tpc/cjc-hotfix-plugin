// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@OverflowThrowing
@patchable
func arithmetic(a: Int64, b: Int64): Int64 {
    var n = 0
    try { n += -a } catch (_: ArithmeticException) { n += 1 }
    try { n += a + b } catch (_: ArithmeticException) { n += 1 }
    try { n += a - b } catch (_: ArithmeticException) { n += 1 }
    try { n += a * b } catch (_: ArithmeticException) { n += 1 }
    try { n += a / b } catch (_: ArithmeticException) { n += 1 }
    try { n += a % b } catch (_: ArithmeticException) { n += 1 }
    try { n += a ** UInt64(b) } catch (_: ArithmeticException) { n += 1 }
    try { n += a << b } catch (_: ArithmeticException) { n += 1 }
    try { n += a >> b } catch (_: ArithmeticException) { n += 1 }
    n
}
@OverflowThrowing
@patchable
func overflowCount(a: Int64): Int64 {
    var caught: Int64 = 0
    try { a + 1 } catch (_: OverflowException) { caught++ }
    try { a * 2 } catch (_: OverflowException) { caught++ }
    try { a ** UInt64(2) } catch (_: OverflowException) { caught++ }
    caught
}
@patchable
func doTest(): Int64 {
    if (overflowCount(Int64.Max) != 3) { return 2 }
    if (arithmetic(2, 1) != 13) { return 1 }
    0
}

main(): Int64 {
    let result = doTest()
    if (result != 0) { return 1 }
    println("try_arithmetic passed")
    0
}
