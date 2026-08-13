package package_init

import hotfix.patchable

main() {
  foo()
}

@patchable
func foo() {
  println("Hello")
}
