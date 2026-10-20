// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
class B {
    func g(): Int64 {
        100
    }
}

@patchable
interface I {
    func f(): Int64
}

extend B <: I {
    @patchable
    public func f() {
        1
    }
}

@patchable
func doTest(): Int64 {
    var m: E = C(B())
    let res: Int64 = match (m) {
        case C(x: B) => x.g()
        case _ => 0
    }

    if (res == 100) {
        return 0
    } else {
        return 1
    }
}

@patchable
enum E {
    C(I)
}

main(): Int64 {
    let result = doTest()
    if (result != 0) { return result }
    println("pattern_enum_interface passed")
    0
}
