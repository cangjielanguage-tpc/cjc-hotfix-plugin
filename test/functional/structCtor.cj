import hotfix.patchable

main() {
  let foo = Foo(42, Some(43), X("44"))
  println(foo.a)
  println(foo.b)
  println(foo.c)
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

  @patchable
  init(a: Int64, b: ?Int32, c: X) {
    this.a = a
    this.b = b
    this.c = c
  }

  // non-patchable, using in stub functions
  init() {
    this.a = 45
    this.b = 46
    this.c = X("47")
  }
}

// stub functions
class Patch {

   // patch for Foo.init(a: Int64, b: ?Int32, c: X)
   func newFoo(a: Int64, b: ?Int32, c: X): Foo {
     var x = Foo()
     x.a += a
     x.b = Some(Int32(x.b??0) + Int32(b??0))
     x.c = X(c.str + "0")
     x
   }
}
