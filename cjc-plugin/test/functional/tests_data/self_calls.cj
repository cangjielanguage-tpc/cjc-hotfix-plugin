import hotfix.patchable

main() {
  let base = Base()
  let derived = Derived()
  base.printMember()
  derived.printMember()
  base.printExtension()
  derived.printExtension()
}

open class Base {
  @patchable
  protected open func printVirtual(): Unit {
    println("base")
  }

  @patchable
  protected static func printStatic(): Unit {
    println("static")
  }

  @patchable
  public func printMember(): Unit {
    println("member")
    printVirtual()
    printStatic()
  }
}

extend Base {
  @patchable
  public func printExtension(): Unit {
    println("extension")
    printVirtual()
    printStatic()
  }
}

class Derived <: Base {
  @patchable
  protected override func printVirtual(): Unit {
    println("derived")
  }
}
