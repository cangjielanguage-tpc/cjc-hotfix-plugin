import hotfix.patchable

main() {
  foo()
  bar(42)
  baz(3.14)
}

@patchable
func foo(): Unit {
  println("foo")
}

@patchable
func bar(i: Int32): Unit {
  println("bar ${i}")
}

func baz(f: Float32): Unit {
  println("baz ${f}");
}
