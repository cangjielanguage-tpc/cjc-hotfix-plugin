import hotfix.patchable

main() {
  printFoo( Foo() )
  printFoo( Foo(Int64(2)) )
  printFoo( Foo(Int32(3)) )
  printFoo( Foo(4, 5) )
  printFoo( Foo("str") )

  printBar( Bar() )
  printBar( Bar(6, 7) )
}

func printFoo(foo: Foo) {
  println(foo.i)
  println(foo.j)
  println(foo.str)
}

func printBar(bar: Bar) {
  println(bar.i)
  println(bar.j)
  println(bar.str)
}

@patchable
open class Super1 {
  let str: String

  init() {
    this("0")
  }

  init(str: String) {
    this.str = str
  }
}

@patchable
class Foo <: Super1 {
  var i: Int64
  var j: Int32

  init() {
    this(44, 45)
  }

  init(i: Int64) {
    this(i, 42)
  }

  init(j: Int32) {
    this(43, j)
  }

  init(i: Int64, j: Int32) {
    super("${i + Int64(j)}")
    this.i = i
    this.j = j
  }

  init(s: String) {
    super(s)
    this.i = 43
    this.j = 42
  }
}

open class Super2 {
  let str: String

  @patchable
  init() {
    this("0")
  }

  init(str: String) {
    this.str = str
  }
}

class Bar <: Super2 {
  let i: Int64
  let j: Int32

  init() {
    super()
    this.i = 100
    this.j = 101
  }

  init(i: Int64, j: Int32) {
    super("${i + Int64(j)}")
    this.i = i
    this.j = j
  }
}

// stub functions
class Patch {

  // patch for Foo.init(i: Int64, j: Int32))
  func newFoo(f: Foo, i: Int64, j: Int32): Unit {
    f.i = 10 * i
    f.j = Int32(20 * j)
  }
}
