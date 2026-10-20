// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
struct B {
    var bb = 1
}

@patchable
struct A {
    var b = B()
}

@patchable
func exercise() {
    var x = A()
    x.b.bb += 3
    return x.b.bb - 4
}

main(): Int64 {
    let result = exercise()
    if (result != 0) { return 1 }
    println("nested_struct_store passed")
    0
}
