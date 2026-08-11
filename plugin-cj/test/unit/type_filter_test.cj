package hotfixplugin

func expectEquals(name: String, actual: Bool, expected: Bool): Unit {
    if (actual != expected) {
        throw Exception("failed: " + name + ", expected " + expected.toString() + ", actual " + actual.toString())
    }
}

func expectTrue(name: String, actual: Bool): Unit {
    expectEquals(name, actual, true)
}

func expectFalse(name: String, actual: Bool): Unit {
    expectEquals(name, actual, false)
}

main(): Int64 {
    expectFalse("empty filter does not match", matchesFilter("pkg1", []))

    expectTrue("exact package", matchesFilter("pkg1", ["pkg1"]))

    expectTrue("exact class", matchesFilter("pkg1.Class1", ["pkg1.Class1"]))

    expectTrue("single-star package segment 1", matchesFilter("pkg2.pkg3.Func1", ["pkg2.*.Func1"]))
    expectTrue("single-star package segment 2", matchesFilter("pkg2.pkg4.Func1", ["pkg2.*.Func1"]))
    expectFalse("single-star rejects suffix", matchesFilter("pkg2.pkg4.Func11", ["pkg2.*.Func1"]))
    expectFalse("single-star rejects prefix", matchesFilter("ppkg2.pkg4.Func1", ["pkg2.*.Func1"]))
    expectFalse("single-star rejects two package segments", matchesFilter("pkg2.pkg5.pkg6.Func1", ["pkg2.*.Func1"]))
    expectFalse("single-star requires one package segment", matchesFilter("pkg2.Func1", ["pkg2.*.Func1"]))

    expectTrue("double-star nested class 1", matchesFilter("pkg3.pkg4.Class3", ["pkg3.**.Class3"]))
    expectTrue("double-star nested class 2", matchesFilter("pkg3.pkg4.pkg5.Class3", ["pkg3.**.Class3"]))
    expectFalse("double-star requires nested segment", matchesFilter("pkg3.Class3", ["pkg3.**.Class3"]))

    println("type_filter_test: passed")
    return 0
}
