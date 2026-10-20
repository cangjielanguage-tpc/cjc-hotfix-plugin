// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

var a = true

@patchable
class A {
    var x = 1
}

@patchable
func foo() {
    var obj = A()

    func goo(): Unit {
        if (a) {
            obj = A()
            obj.x = 10
        } else {
            println("fake recursion")
            goo()
        }
    }
    goo()

    println(obj.x)
    0
}

@patchable
func exercise() { 
    foo() 
}

main(): Int64 {
    let result = exercise()
    if (result != 0) { return 1 }
    println("boxed_reference_capture passed")
    0
}
