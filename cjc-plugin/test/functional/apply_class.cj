// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
class Foo {
    func foo(): Unit {
        println("foo")
    }

    func bar(i: Int32, o: ?Int32) {
        let res = match(o) {
            case Some(x) => x == i
            case None    => false
        }
        println(res)
    }

    static func baz(f: Float64) {
        println("baz ${f}")
    }
}

@patchable
func doTest(): Int64 {
    let foo = Foo()
    foo.foo()
    foo.bar(1, Some(10))
    foo.bar(42, Some(42))
    foo.bar(2, None)
    Foo.baz(3.14)
    0
}

main(): Int64 {
    let result = doTest()
    if (result != 0) { return result }
    println("apply_class passed")
    0
}
