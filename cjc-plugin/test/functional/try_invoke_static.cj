// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
interface Factory { static func make(fail: Bool): Int64 }
@patchable
class C <: Factory {
    public static func make(fail: Bool): Int64 {
        if (fail) { throw Exception("expected") }
        7
    }
}
@patchable
func checked<T>(): Int64 where T <: Factory {
    var n = 0
    try { n = T.make(false) } catch (_: Exception) { return 1 }
    try { T.make(true); return 2 } catch (_: Exception) { n += 1 }
    if (n != 8) { return 3 }
    0
}

main(): Int64 {
    let result = checked<C>()
    if (result != 0) { return 1 }
    println("try_invoke_static passed")
    0
}
