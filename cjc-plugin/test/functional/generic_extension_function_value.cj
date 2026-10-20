// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
interface I<T1> {
    static func foo<T2>() {}
    static func goo(x: T1) {
        0
    }
}

extend Bool <: I<Bool> {}

extend Int32 <: I<Int32> {
    @patchable
    public static func goo(x: Int32) {
        1
    }
}

extend CString <: I<Int32> & I<Bool> {
    @patchable
    public static func foo<U>() {}
}

@patchable
func doTest() {
    var a = Bool.foo<Bool>
    var b = Int32.goo
    var c: (Bool) -> Int64 = CString.goo
    a()
    return b(1) - 1 + c(true)
}

main(): Int64 {
    let result = doTest()
    if (result != 0) { return 1 }
    println("generic_extension_function_value passed")
    0
}
