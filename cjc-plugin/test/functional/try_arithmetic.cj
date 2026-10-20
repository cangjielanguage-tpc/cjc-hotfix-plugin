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
@patchable
func exercise(): Int64 {
    if (arithmetic(2, 1) != 13) { return 1 }
    0
}

main(): Int64 {
    let result = exercise()
    if (result != 0) { return 1 }
    println("try_arithmetic passed")
    0
}
