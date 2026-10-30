import hotfix.patchable

main() {
  printValue<Int64>(42)
  printValue<String>("global")
  let foo = Foo()
  foo.printValue<Int64>(43)
  foo.printValue<String>("method")
  Foo.printPair<Int64, String>(44, "static")
  Foo.printPair<String, Int64>("static", 45)
}

@patchable
func printValue<T>(value: T): Unit where T <: ToString {
  println(value.toString())
}

class Foo {
  @patchable
  func printValue<T>(value: T): Unit where T <: ToString {
    println(value.toString())
  }

  @patchable
  static func printPair<T, U>(first: T, second: U): Unit where T <: ToString, U <: ToString {
    println("${first.toString()} ${second.toString()}")
  }
}
