package package_init

import hotfix.patchable

let initialized = initializeGlobal()

func initializeGlobal() {
  println("Global initializer ran")
}

main() {
  foo()
}

@patchable
func foo() {
  println("Hello")
}
