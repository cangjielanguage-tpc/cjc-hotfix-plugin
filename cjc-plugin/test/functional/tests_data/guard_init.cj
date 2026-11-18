import hotfix.patchable

let initialized = initialize()

@patchable
func initialize(): Unit {
  println("global initializer")
}

main() {
  initialized
  println("main")
}
