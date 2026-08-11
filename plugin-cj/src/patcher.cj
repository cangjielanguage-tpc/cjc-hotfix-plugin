/*
 * Cangjie port of src/Patcher.cpp.
 *
 * Rewrites the CHIR of each patchable so that, when its guard flag is set, the
 * call is routed to a virtual "guard" method (implemented by a generated patch
 * class) instead of executing the original body. Also generates the guard
 * class, the guard variable and its flag, the guard-vars initializer, and the
 * patch implementation class.
 *
 * Translation notes vs. the C++ original:
 *  - Expressions are built directly at an `InsertPosition` cursor instead of
 *    create-then-Move.
 *  - A terminator is "replaced" by capturing the original successor, destroying
 *    the old terminator, then appending the new control flow.
 *  - Enum (Option) values are created as tuples whose result type is overridden
 *    to the Option enum type, matching the C++ explicit-result-type Tuple.
 *  - Compiler-internal attributes/function metadata with no public setter
 *    (OVERRIDE/FINAL/COMPILER_ADD/NO_INLINE/NO_DEBUG_INFO/SKIP_ANALYSIS,
 *    external linkage, FuncKind) are still documented at the use sites below.
 *    Public equivalents (internal/protected/open/abstract/readOnly) are set.
 */
package hotfixplugin

import stdx.chir.*
import std.collection.*

class Patcher {
    let pkg: Package
    let pluginCtx: PluginContext

    // Lazily initialized on the first `patch` call.
    var guardVarsInitializer: ?Function = None
    var guardClass: ?ClassLikeDef = None
    var guardVar: ?GlobalVar = None

    init(pkg: Package, pluginCtx: PluginContext) {
        this.pkg = pkg
        this.pluginCtx = pluginCtx
        // C++ Patcher constructs package-init guards here. This CJ port still
        // lacks the complete package-init path because public stdx.chir does
        // not expose all operations used by Patcher::genPackageInitGuardChecks
        // and Patcher::genPackageInitAccessors. Keep this explicit because it
        // is a functional parity gap, not an intentional behavior change.
    }

    func patch(patchable: Patchable): Unit {
        if (guardVarsInitializer.isNone()) {
            let gvi = genGuardVarsInitializer()
            guardVarsInitializer = gvi
            let gc = genGuardClass()
            guardClass = gc
            guardVar = genGuardVar(gc.classType, false)
        }
        let guardVarFlag = genGuardVarFlag(patchable.funcName.genGuardVarFlagName(false))
        let guardMethod = genGuardMethod(patchable.funcName.genGuardMethodName(GuardMethodKind.Plain), patchable.fn)
        if (envSet("HOTFIX_SKIP_CHECKS")) {
            return
        }
        genGuardChecks(patchable.fn, guardVarFlag, guardMethod)
    }

    /*
     * Generates the patch template class:
     *   class $PatchImpl <: $Patch { override each guard method to throw }
     */
    func genPatchClass(): Unit {
        let gc = match (guardClass) {
            case Some(c) => c
            case None => return
        }
        let patchName = PatchableName(gc).genPatchClassName()
        let patch = pkg.addClass(patchName, patchName, true)
        patch.superClass = gc.classType
        patch.setInternal()
        // C++ also sets COMPILER_ADD and external linkage here. Public
        // stdx.chir does not expose those setters for plugin code.

        for (baseMethod in gc.methods) {
            let baseMethodType = baseMethod.funcType
            let paramTypes = baseMethodType.paramTypes
            paramTypes[0] = RefType.get(patch.classType)
            let retType = baseMethodType.returnType
            let patchMethodType = FuncType.get(paramTypes, retType)

            let baseName = baseMethod.srcCodeName
            let patchMethod = pkg.addFunction(patchMethodType, baseName, baseName, pkg.name)
            patchMethod.setProtected()
            // C++ also marks this method OVERRIDE, FINAL and external. Public
            // stdx.chir does not expose those setters.

            patchMethod.initBody()
            let body = patchMethod.body.getOrThrow().entryBlock
            let b = CHIRBuilder(InsertPosition.AtEnd(body))
            let retVal = b.createAllocate(retType)
            patchMethod.returnValue = retVal.result

            // TODO (as in the C++): figure out why an Exit here fails at runtime.
            genShouldNotReachHere(body)

            patch.appendMethod(patchMethod)
        }
    }

    /* [abstract] class $Patch <: Object {} */
    func genGuardClass(): ClassLikeDef {
        let name = PatchableName(pkg).genGuardClassName(false)
        let cl = pkg.addClass(name, name, true)
        cl.superClass = pluginCtx.objectType
        cl.setAbstract(true)
        cl.setInternal()
        // C++ marks the class COMPILER_ADD and external. Public stdx.chir does
        // not expose those setters.
        if (hotfixDebug()) {
            println("Create guard class")
            println("Guard class:")
            println(cl.toString())
        }
        return cl
    }

    /* func $guardVarsInit() : Unit { Allocate(Unit); Exit } and an Apply into package init. */
    func genGuardVarsInitializer(): Function {
        let gviName = PatchableName(pkg).genGuardVarInitializedName()
        let gvi = createEmptyFunctionWithUnitRetVal(gviName, Array<Type>())

        let entry = gvi.body.getOrThrow().entryBlock
        CHIRBuilder(InsertPosition.AtEnd(entry)).createExit()

        // Call the initializer from the package init function. The C++ inserts
        // the Apply into the block carrying the INITIALIZER attribute; that
        // attribute is not exposed, so we insert before the terminator of the
        // package-init entry block.
        if (let Some(pkgInit) <- pkg.packageInitFunc) {
            if (let Some(bg) <- pkgInit.body) {
                let block = bg.entryBlock
                let term = lastExpr(block)
                let b = CHIRBuilder(InsertPosition.Before(term))
                b.createApply(gvi, FuncCallContext(Array<Value>(), Array<Type>(), None))
            }
        }
        return gvi
    }

    /*
     * abstract func $..Patch: ($Patch&, <orig params>) -> <ret>
     */
    func genGuardMethod(name: String, patchable: Function): Function {
        if (hotfixDebug()) {
            println("gen guard method for patchable func:")
            println(patchable.toString())
        }
        let gc = guardClass.getOrThrow()
        let patchableMethodType = patchable.funcType

        let origParamTypes = patchableMethodType.paramTypes
        let methodParamTypes = ArrayList<Type>()
        methodParamTypes.add(RefType.get(gc.classType))
        // C++ removes the `this` parameter for struct constructors and returns
        // the struct by value. Struct constructors are skipped by
        // PatchableFinder in both implementations, so the class-constructor and
        // normal-function path keeps all original parameters.
        for (t in origParamTypes) {
            methodParamTypes.add(t)
        }

        let methodType = FuncType.get(methodParamTypes.toArray(), patchableMethodType.returnType)
        let method = pkg.addFunction(methodType, name, name, pkg.name)
        method.setAbstract(true)
        method.setProtected()
        // C++ also sets NO_DEBUG_INFO, COMPILER_ADD and external linkage here.
        // Public stdx.chir does not expose those setters.

        gc.appendMethod(method)
        if (hotfixDebug()) {
            println("guard class with new guard method:")
            println(gc.toString())
        }
        return method
    }

    /* $patchVar: Option<$Patch&>&  (initialized to None by $guardVarsInit) */
    func genGuardVar(guardClassType: ClassLikeType, forPackageInit: Bool): GlobalVar {
        let gvName = PatchableName(pkg).genGuardVarName(forPackageInit)
        if (hotfixDebug()) {
            println("Gen guard var " + gvName + " for class " + guardClassType.toString())
        }
        let refToClass = RefType.get(guardClassType)
        let guardVarType = EnumType.get(pluginCtx.optionDef, [refToClass])
        let gv = pkg.addGlobalVar(RefType.get(guardVarType), gvName, gvName, pkg.name)
        // C++ marks guard vars external. Public stdx.chir does not expose
        // linkage setters.

        if (!forPackageInit) {
            if (hotfixDebug()) {
                println("Instantiate guard var")
            }
            gv.setInitializer(guardVarsInitializer.getOrThrow())
            let bg = guardVarsInitializer.getOrThrow().body.getOrThrow()
            let body = bg.entryBlock
            let term = lastExpr(body)
            let b = CHIRBuilder(InsertPosition.Before(term))
            let trueExpr = b.createConstant(BoolLiteral.get(true))
            let tuple = b.createTuple([trueExpr.result])
            tuple.result.ty = guardVarType // override inferred tuple type to the Option enum type
            b.createStore(tuple.result, gv)
            if (hotfixDebug()) {
                println("guard vars initializer body:")
                println(bg.toString())
            }
        }
        if (hotfixDebug()) {
            println(gv.toString())
        }
        return gv
    }

    /* @$..PatchVarFlag: Bool& = false */
    func genGuardVarFlag(name: String): GlobalVar {
        if (hotfixDebug()) {
            println("Gen guard var flag " + name)
        }
        let gvf = pkg.addGlobalVar(RefType.get(BoolType.get()), name, name, pkg.name)
        gvf.setInitializer(BoolLiteral.get(false))
        // C++ marks guard flag vars external. Public stdx.chir does not expose
        // linkage setters.
        if (hotfixDebug()) {
            println(gvf.toString())
        }
        return gvf
    }

    /*
     * Rewrites the patchable's entry so that when its flag is set control flows
     * to the guard method; otherwise to the original body. (Non-constructor
     * path; the constructor path is added in the package-init/ctor phase.)
     */
    func genGuardChecks(patchable: Function, guardVarFlag: GlobalVar, guardMethod: Function): Unit {
        if (hotfixDebug()) {
            println("Gen guard check")
        }
        let patchableBody = patchable.body.getOrThrow()
        let entryBlock = patchableBody.entryBlock

        // The single original successor of the entry block.
        let originalSuccessor = entryBlock.singleSuccessor()

        // Replace the entry terminator with: load flag; not; branch.
        let entryTerm = lastExpr(entryBlock)
        entryTerm.destroySelf()

        let guardVarCheckBlock = patchableBody.appendBlock()

        let eb = CHIRBuilder(InsertPosition.AtEnd(entryBlock))
        let loadGuardVarFlag = eb.createLoad(guardVarFlag)
        let guardVarFlagCond = eb.createUnaryNot(loadGuardVarFlag.result)
        eb.createBranch(guardVarFlagCond.result, originalSuccessor, guardVarCheckBlock)

        // guardVarCheckBlock: check whether the patch var is initialized.
        let gv = guardVar.getOrThrow()
        let guardVarBaseType = refBaseType(gv.ty) // Option<$Patch&>

        let cb = CHIRBuilder(InsertPosition.AtEnd(guardVarCheckBlock))
        let loadGuardVar = cb.createLoad(gv)
        let guardVarResult = loadGuardVar.result
        let field = cb.createField(guardVarResult, pathOf(0))
        let falseConst = cb.createConstant(BoolLiteral.get(false))
        let guardVarCheckCond = cb.createBinaryEqual(field.result, falseConst.result)

        var callPatchBlock = patchableBody.appendBlock()
        let shouldNotReachHereBlock = patchableBody.appendBlock()
        genShouldNotReachHere(shouldNotReachHereBlock)
        cb.createBranch(guardVarCheckCond.result, callPatchBlock, shouldNotReachHereBlock)

        var modifiedPatchable = patchable
        var modifiedPatchableBody = patchableBody
        var modifiedGuardVarResult: Value = guardVarResult

        if (isConstructorFn(patchable)) {
            // C++ creates a synthetic constructor with original parameters plus
            // the loaded guard var, invokes it from the original constructor's
            // patch block, then emits the guard-method Invoke inside that
            // synthetic constructor. We use the same CFG shape.
            let paramTypes = ArrayList<Type>()
            for (p in patchable.parameters) {
                paramTypes.add(p.ty)
            }
            paramTypes.add(guardVarResult.ty)

            let ctorName = PatchableName(patchable).genSyntheticCtorName()
            let ctor = createEmptyFunctionWithUnitRetVal(ctorName, paramTypes.toArray())
            ctor.setPrivate()
            // C++ sets FuncKind to the original constructor kind, COMPILER_ADD,
            // external linkage, and for class constructors SKIP_ANALYSIS. Those
            // setters are not exposed through public stdx.chir. This is the
            // remaining known reason class-constructor tests may fail although
            // the generated control-flow shape follows the C++ implementation.
            if (let Some(parent) <- patchable.declaredParent) {
                parent.appendMethod(ctor)
            }

            let pb = CHIRBuilder(InsertPosition.AtEnd(callPatchBlock))
            let originalArgs = toValues(patchable.parameters)
            let ctorArgs = ArrayList<Value>()
            for (a in originalArgs) {
                ctorArgs.add(a)
            }
            ctorArgs.add(guardVarResult)
            let thisTy: ?Type = if (patchable.parameters.isEmpty()) {
                None
            } else {
                Some(patchable.parameters[0].ty)
            }
            pb.createApply(ctor, FuncCallContext(ctorArgs.toArray(), Array<Type>(), thisTy))
            pb.createExit()

            modifiedPatchable = ctor
            modifiedPatchableBody = ctor.body.getOrThrow()
            callPatchBlock = modifiedPatchableBody.entryBlock
            let ctorParams = ctor.parameters
            modifiedGuardVarResult = ctorParams[ctorParams.size - 1]
        }

        // callPatchBlock: extract the patch instance and invoke the guard method.
        let classRefType = guardVarBaseType.typeArgs[0] // $Patch&
        let optionType = TupleType.get([BoolType.get(), classRefType])

        let pb = CHIRBuilder(InsertPosition.AtEnd(callPatchBlock))
        let typeCast = pb.createNumericCast(modifiedGuardVarResult, optionType, OverflowStrategy.Throwing)
        let caller = pb.createField(typeCast.result, pathOf(1))
        // C++ marks caller->GetResult() READONLY. The public stdx.chir API has
        // setReadOnly on GlobalValue, not on this LocalVar result.

        let funcType = guardMethod.funcType
        let args = if (isConstructorFn(patchable)) {
            let params = modifiedPatchable.parameters
            Array<Value>(params.size - 1, {i => params[i]})
        } else {
            toValues(patchable.parameters)
        }
        let funcCallCtx = FuncCallContext(args, Array<Type>(), classRefType)
        let virMethodCtx = FuncSigInfo(guardMethod.srcCodeName, funcType, guardMethod.genericTypeParams)
        let callContext = InvokeCallContext(caller.result, virMethodCtx, funcCallCtx)
        let callMethod = pb.createInvoke(funcType.returnType, callContext)

        let resultType = callMethod.result.ty
        if (resultType.isUnitType()) {
            let unitConst = pb.createConstant(UnitLiteral.get())
            pb.createStore(unitConst.result, modifiedPatchable.returnValue.getOrThrow())
        } else {
            pb.createStore(callMethod.result, modifiedPatchable.returnValue.getOrThrow())
        }
        pb.createExit()

        if (hotfixDebug()) {
            if (isConstructorFn(patchable)) {
                println("new ctor:")
                println(modifiedPatchable.toString())
            }
            println("updated patchable:")
            println(patchable.toString())
        }
    }

    /*
     * Block: raise Exception("should not reach here").
     */
    func genShouldNotReachHere(block: Block): Unit {
        // C++ emits:
        //   Constant("should not reach here")
        //   Allocate(Exception)
        //   Apply(Exception.<init>(String))
        //   RaiseException(exception)
        //
        // The same public stdx.chir construction compiles in plugin-cj but
        // makes `cjc --plugin ... --dump-chir` crash with SIGSEGV for every
        // integration fixture. Until stdx.chir exposes a safe equivalent for
        // this terminator sequence, keep the previous safe terminator and mark
        // this as an explicit behavior gap.
        CHIRBuilder(InsertPosition.AtEnd(block)).createExit()
    }

    func createEmptyFunctionWithUnitRetVal(name: String, paramTypes: Array<Type>): Function {
        let fType = FuncType.get(paramTypes, UnitType.get())
        let f = pkg.addFunction(fType, name, name, pkg.name)
        f.initBody()
        let entry = f.body.getOrThrow().entryBlock
        let b = CHIRBuilder(InsertPosition.AtEnd(entry))
        let retVal = b.createAllocate(UnitType.get())
        f.returnValue = retVal.result
        return f
    }
}

// --- small helpers -------------------------------------------------------

/** Last (terminator) expression of a block. */
func lastExpr(block: Block): Expression {
    let exprs = block.exprs
    return exprs[exprs.size - 1]
}

/** The single successor of the entry block (the original body block). */
extend Block {
    func singleSuccessor(): Block {
        let succ = lastExpr(this).successors
        if (succ.size != 1) {
            throw Exception("entry block is expected to have exactly one successor, found ${succ.size}")
        }
        return succ[0]
    }
}

/** Build a Field/GetElement path of a single index. */
func pathOf(i: UInt64): ArrayList<UInt64> {
    let p = ArrayList<UInt64>()
    p.add(i)
    return p
}

/** Upcast an array of Parameter to Array<Value>. */
func toValues(params: Array<Parameter>): Array<Value> {
    return Array<Value>(params.size, {i => params[i]})
}

/** Base type of a RefType, or the type itself if not a ref. */
func refBaseType(t: Type): Type {
    if (let Some(r) <- (t as RefType)) {
        return r.baseType
    }
    return t
}
