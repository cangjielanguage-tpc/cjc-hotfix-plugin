// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
struct S1 {
    var z: Int64
    init() {
        z = 12
    }

    public static func staticFoo1() {
        12
    }
    public func Foo2() {
        z
    }
}

@patchable
func test3() {
    var a = S1()
    S1.staticFoo1() - a.Foo2()
}

main(): Int64 {
    let result = test3()
    if (result != 0) { return 1 }
    println("apply_struct passed")
    0
}
