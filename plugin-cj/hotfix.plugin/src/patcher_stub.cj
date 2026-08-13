/*
 * Test-only stub patcher for the Cangjie CHIR hotfix plugin.
 *
 * This mirrors the default branch of the C++ PatcherStub: generated patch
 * methods with Unit return type are replaced with:
 *
 *   println("Hello from stub")
 *
 * and every generated guard flag is enabled from the guard-vars initializer.
 */
package hotfixplugin

import stdx.chir.*
import std.collection.*
import std.env.*
import std.fs.*
import std.io.*

class PatcherStub {
    let pkg: Package
    let guardClass: ?ClassLikeDef
    let patchClass: ?ClassLikeDef
    let guardVarsInitializer: ?Function
    let printlnStringFunc: Function
    let stubsMap: HashMap<String, String>

    init(pkg: Package) {
        this.pkg = pkg
        this.stubsMap = readStubMaps()
        this.printlnStringFunc = findStubPrintlnStringFunc(pkg)
        let gcOpt = findOptionalStubClass(pkg, PatchableName(pkg).genGuardClassName(false))
        if (gcOpt.isNone()) {
            this.guardClass = None
            this.patchClass = None
            this.guardVarsInitializer = None
            return
        }
        let gc = gcOpt.getOrThrow()
        this.guardClass = Some(gc)
        this.patchClass = Some(findStubClass(pkg, PatchableName(gc).genPatchClassName()))
        this.guardVarsInitializer = Some(findStubFunction(pkg, PatchableName(pkg).genGuardVarInitializedName()))
        prepareGuardVarsInitializer()
        if (pkg.name == "package_init") {
            preparePackageInitStub()
        }
        debug { =>
            println("")
            println("")
            println("")
            if (this.stubsMap.isEmpty()) {
                println("stub map file is empty")
            } else {
                println("found patched stub map file")
            }
        }
    }

    func patch(patchable: Patchable): Unit {
        if (pkg.name == "package_init") {
            return
        }
        if (patchClass.isNone() || guardVarsInitializer.isNone()) {
            return
        }
        if (patchable.fn.srcCodeName == "main") {
            return
        }
        let patchMethodName = patchable.funcName.genGuardMethodName(GuardMethodKind.Plain)
        let patchMethod = findPatchMethod(patchMethodName)
        if (stubsMap.contains(patchMethodName)) {
            genMappedStubCodeForPatchMethod(patchMethod, patchMethodName)
        } else {
            genDefaultStubCodeForPatchMethod(patchMethod)
        }
        enableGuardFlag(patchable)
    }

    private func genMappedStubCodeForPatchMethod(patchMethod: Function, patchMethodName: String): Unit {
        let stubName = stubsMap[patchMethodName]
        debug { =>
            println("search stub code for " + patchMethodName + ": " + stubName)
        }
        let stubFn = findMappedStubFunction(pkg, stubName)
        if (stubFn.body.isNone()) {
            throw Exception("mapped stub has no body " + stubName)
        }
        if (!patchMethod.funcType.returnType.isUnitType()) {
            // C++ PatcherStub replaces the body, fixes parameter references,
            // removes the source stub function/method and resets the return
            // value. The public stdx.chir API only exposes moveBodyTo(), which
            // is safe for the Unit-returning mapped stubs currently reachable
            // by the tests. Keep this failure explicit for non-Unit stubs.
            throw Exception("mapped stub body transplant is not available through public stdx.chir API for " +
                patchMethodName)
        }
        debug { =>
            println("found:")
            println(stubFn.toString())
        }
        stubFn.moveBodyTo(patchMethod)
        debug { =>
            println("patch after replacement:")
            println(patchMethod.toString())
        }
    }

    private func findPatchMethod(name: String): Function {
        for (m in patchClass.getOrThrow().methods) {
            if (m.srcCodeName == name) {
                return m
            }
        }
        throw Exception("unable to find patch method " + name)
    }

    private func stringType(): Type {
        for (s in pkg.importedStructDefs) {
            if (s.srcCodeName == "String") {
                return s.ty
            }
        }
        throw Exception("unable to find String type")
    }

    private func prepareGuardVarsInitializer(): Unit {
        let guardVar = findGuardVar()
        let entry = guardVarsInitializer.getOrThrow().body.getOrThrow().entryBlock
        let term = lastExpr(entry)
        let b = CHIRBuilder(InsertPosition.Before(term))

        let patchAlloc = b.createAllocate(patchClass.getOrThrow().classType)
        let patchAsGuard = b.createNumericCast(patchAlloc.result, RefType.get(guardClass.getOrThrow().classType), OverflowStrategy.Throwing)
        let falseExpr = b.createConstant(BoolLiteral.get(false))

        let guardVarType = refBaseType(guardVar.ty)
        let tuple = b.createTuple([falseExpr.result, patchAsGuard.result])
        tuple.result.ty = guardVarType
        b.createStore(tuple.result, guardVar)
    }

    private func preparePackageInitStub(): Unit {
        if (let Some(pkgInit) <- pkg.packageInitFunc) {
            let entry = pkgInit.body.getOrThrow().entryBlock
            let term = lastExpr(entry)
            let b = CHIRBuilder(InsertPosition.Before(term))
            let hello = b.createConstant(StringLiteral.get(stringType(), "Hello from package init stub"))
            b.createApply(printlnStringFunc, FuncCallContext([hello.result], Array<Type>(), None))
        }
    }


    private func findGuardVar(): GlobalVar {
        let name = PatchableName(pkg).genGuardVarName(false)
        for (gv in pkg.globalVars) {
            if (gv.srcCodeName == name) {
                return gv
            }
        }
        throw Exception("unable to find guard var " + name)
    }

    private func enableGuardFlag(patchable: Patchable): Unit {
        let name = patchable.funcName.genGuardVarFlagName(false)
        var flag: ?GlobalVar = None
        for (gv in pkg.globalVars) {
            if (gv.srcCodeName == name) {
                flag = gv
                break
            }
        }
        let guardVarFlag = flag ?? throw Exception("unable to find guard var flag " + name)

        let entry = guardVarsInitializer.getOrThrow().body.getOrThrow().entryBlock
        let term = lastExpr(entry)
        let b = CHIRBuilder(InsertPosition.Before(term))
        let trueExpr = b.createConstant(BoolLiteral.get(true))
        b.createStore(trueExpr.result, guardVarFlag)
        debug { =>
            println("guardVarsInit after update:")
            println(entry.toString())
        }
    }

    private func genDefaultStubCodeForPatchMethod(patchMethod: Function): Unit {
        debug { =>
            println("stub method for " + patchMethod.srcCodeName + " was not found in stub map")
        }
        if (!patchMethod.funcType.returnType.isUnitType()) {
            throw Exception("default stub only supports Unit-returning patch methods")
        }

        let bg = patchMethod.body.getOrThrow()
        let entry = bg.entryBlock
        for (expr in entry.exprs) {
            expr.destroySelf()
        }

        let b = CHIRBuilder(InsertPosition.AtEnd(entry))
        let retVal = b.createAllocate(UnitType.get())
        patchMethod.returnValue = retVal.result
        let hello = b.createConstant(StringLiteral.get(stringType(), "Hello from stub"))
        b.createApply(printlnStringFunc, FuncCallContext([hello.result], Array<Type>(), None))
        b.createExit()
        debug { =>
            println("patch after replacement:")
            println(patchMethod.toString())
        }
    }
}

private func findStubClass(pkg: Package, name: String): ClassLikeDef {
    return findOptionalStubClass(pkg, name) ?? throw Exception("unable to find class " + name)
}

private func findOptionalStubClass(pkg: Package, name: String): ?ClassLikeDef {
    for (c in pkg.classDefs) {
        if (c.srcCodeName == name) {
            return Some(c)
        }
    }
    return None
}

private func findStubFunction(pkg: Package, name: String): Function {
    for (f in pkg.functions) {
        if (f.srcCodeName == name) {
            return f
        }
    }
    throw Exception("unable to find function " + name)
}

private func findMappedStubFunction(pkg: Package, name: String): Function {
    for (f in pkg.functions) {
        if (f.identifierWithoutPrefix == name) {
            return f
        }
    }
    throw Exception("unable to find mapped stub function " + name)
}

private func findStubPrintlnStringFunc(pkg: Package): Function {
    let stringTy = stubStringType(pkg)
    for (f in pkg.functions) {
        if (f.body.isNone() && f.srcCodeName == "println" && f.funcType.paramTypes.size == 1 &&
            f.funcType.paramTypes[0] == stringTy) {
            return f
        }
    }
    throw Exception("unable to find println(String)")
}

private func stubStringType(pkg: Package): Type {
    for (s in pkg.importedStructDefs) {
        if (s.srcCodeName == "String") {
            return s.ty
        }
    }
    throw Exception("unable to find String type")
}

private func readStubMaps(): HashMap<String, String> {
    let result = HashMap<String, String>()
    if (let Some(path) <- getVariable("HOTFIX_STUB_MAP_FILE")) {
        loadStubMapFile(path, result)
        return result
    }
    try {
        Directory.walk(Path(".")) {
            fileInfo =>
            let path = fileInfo.path.toString()
            if (path.endsWith(".cj.stub.map")) {
                loadStubMapFile(path, result)
            }
            true
        }
    } catch (_: Exception) {
        ()
    }
    return result
}

private func loadStubMapFile(path: String, stubsMap: HashMap<String, String>): Unit {
    if (!exists(path)) {
        return
    }
    try (f = File(Path(path), OpenMode.Read)) {
        let reader = StringReader(f)
        while (let Some(raw) <- reader.readln()) {
            let line = raw.trimAscii()
            if (line.isEmpty()) {
                continue
            }
            if (let Some(delimiterPos) <- line.indexOf("=")) {
                let key = line[..delimiterPos].trimAscii()
                let value = line[(delimiterPos + 1)..].trimAscii()
                if (!key.isEmpty() && !value.isEmpty()) {
                    stubsMap[key] = value
                }
            }
        }
    } catch (_: Exception) {
        ()
    }
}
