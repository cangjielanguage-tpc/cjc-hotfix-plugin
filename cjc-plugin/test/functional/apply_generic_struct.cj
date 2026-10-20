// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
struct C {
    func foo<T>(): Int64 {
    0
    }
}

@patchable
class X <: ToString {
  let str: String

  init(str: String) {
    this.str = str
  }

  public override func toString(): String {
    str
  }
}

@patchable
struct Foo {
  var a: Int64
  var b: ?Int32
  var c: X

  init(a: Int64, b: ?Int32, c: X) {
    this.a = a
    this.b = b
    this.c = c
  }

  func foo(): Unit {
    println("foo ${a} ${b} ${c}")
  }

  func bar(i: Int32, o: ?Int32) {
    let res = match(o) {
    case Some(x) => x == i
    case None    => false
    }
    println(res)
  }
}

@patchable
func exercise() {
    var x = C()
    if (x.foo<Int64>() != 0) { return 1 }
    let foo = Foo(42, Some(43), X("44"))
    println(foo.a)
    println(foo.b)
    println(foo.c)
    foo.foo()
    foo.bar(1, Some(10))
    foo.bar(42, Some(42))
    foo.bar(2, None)
    0
}

main(): Int64 {
    let result = exercise()
    if (result != 0) { return 1 }
    println("apply_generic_struct passed")
    0
}
