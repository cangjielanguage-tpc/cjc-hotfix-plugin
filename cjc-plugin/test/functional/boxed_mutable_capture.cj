// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
func foo(): Unit {
    var counter = 0
    func f1(): Unit {
        counter++
        if (counter < 10) {
            f1()
        }
    }
    f1()
    println(counter)
}

@patchable
func exercise() {
    foo()
}

main(): Int64 {
    exercise()
    println("boxed_mutable_capture passed")
    0
}
