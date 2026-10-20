// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
enum HHH<T, G> {
    A(T) | B(G, Int8)
}

@patchable
class CA<T> {
    public let data: T
    init(_x: T) {
        data = _x
    }
}

@patchable
func foo<T, G>(param: HHH<CA<T>, G>) {
    match (param) {
        
        case A(x) => var a = x.data
        
        case B(x, y) => var b = y
    }
}

@patchable
func exercise(): Int64 {
    foo<Int64, Int8>(HHH<CA<Int64>, Int8>.A(CA<Int64>(5)))
    foo<Int64, Int8>(HHH<CA<Int64>, Int8>.B(10, 11))
    0
}

main(): Int64 {
    let result = exercise()
    if (result != 0) { return 1 }
    println("generic_nested_enum passed")
    0
}
