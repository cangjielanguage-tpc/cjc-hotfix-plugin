import hotfix.patchable

main() {
  let foo = Foo()
  foo.foo()
  foo.bar(1, Some(10))
  foo.bar(42, Some(42))
  foo.bar(2, None)
  Foo.baz(3.14)
}

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
