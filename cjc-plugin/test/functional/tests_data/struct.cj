import hotfix.patchable

main() {
  let foo = Foo(42, Some(43), X("44"))
  println(foo.a)
  println(foo.b)
  println(foo.c)
  foo.foo()
  foo.bar(1, Some(10))
  foo.bar(42, Some(42))
  foo.bar(2, None)
}

class X <: ToString {
  let str: String

  init(str: String) {
    this.str = str
  }

  public override func toString(): String {
    str
  }
}

struct Foo {
  var a: Int64
  var b: ?Int32
  var c: X

  init(a: Int64, b: ?Int32, c: X) {
    this.a = a
    this.b = b
    this.c = c
  }

  @patchable
  func foo(): Unit {
    println("foo ${a} ${b} ${c}")
  }

  @patchable
  func bar(i: Int32, o: ?Int32) {
    let res = match(o) {
      case Some(x) => x == i
      case None    => false
    }
    println(res)
  }
}
