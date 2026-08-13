package hotfixplugin

import std.unittest.*

func assertMatches(name: String, actual: Bool, expected: Bool): Unit {
    assertEqual("expected " + name, "actual " + name, expected, actual)
}

func typeFilterSimpleTest(): Unit {
    assertMatches("empty filter does not match", matchesFilter("pkg1", []), false)

    assertMatches("exact package", matchesFilter("pkg1", ["pkg1"]), true)

    assertMatches("exact class", matchesFilter("pkg1.Class1", ["pkg1.Class1"]), true)

    assertMatches("single-star package segment 1", matchesFilter("pkg2.pkg3.Func1", ["pkg2.*.Func1"]), true)
    assertMatches("single-star package segment 2", matchesFilter("pkg2.pkg4.Func1", ["pkg2.*.Func1"]), true)
    assertMatches("single-star rejects suffix", matchesFilter("pkg2.pkg4.Func11", ["pkg2.*.Func1"]), false)
    assertMatches("single-star rejects prefix", matchesFilter("ppkg2.pkg4.Func1", ["pkg2.*.Func1"]), false)
    assertMatches("single-star rejects two package segments", matchesFilter("pkg2.pkg5.pkg6.Func1", ["pkg2.*.Func1"]), false)
    assertMatches("single-star requires one package segment", matchesFilter("pkg2.Func1", ["pkg2.*.Func1"]), false)

    assertMatches("double-star nested class 1", matchesFilter("pkg3.pkg4.Class3", ["pkg3.**.Class3"]), true)
    assertMatches("double-star nested class 2", matchesFilter("pkg3.pkg4.pkg5.Class3", ["pkg3.**.Class3"]), true)
    assertMatches("double-star requires nested segment", matchesFilter("pkg3.Class3", ["pkg3.**.Class3"]), false)
}

main(): Int64 {
    let tests = TestPackage("hotfixplugin")
    tests.registerCase({ => UnitTestCase.create("matches.simple", body: typeFilterSimpleTest) })
    return entryMain(tests)
}
