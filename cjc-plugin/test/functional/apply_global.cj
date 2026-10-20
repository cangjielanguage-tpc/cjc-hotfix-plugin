// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
func foo(): Unit {
  println("foo")
}

@patchable
func bar(i: Int32): Unit {
  println("bar ${i}")
}

@patchable
func baz(f: Float32): Unit {
  println("baz ${f}");
}

@patchable
func exercise() {
foo()
  bar(42)
  baz(3.14)
}

main(): Int64 {
    exercise()
    println("apply_global passed")
    0
}
