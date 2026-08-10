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
 *  - Compiler-internal attributes with no public setter (VIRTUAL/FINAL/OVERRIDE/
 *    COMPILER_ADD/NO_INLINE/SKIP_ANALYSIS/READONLY) are omitted; access levels
 *    and abstract/open are set via the public API.
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
        // Package-init guard setup is performed by setupPackageInitGuards()
        // (ported in the package-init phase); the plain-function path below is
        // independent of it.
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

        for (baseMethod in gc.methods) {
            let baseMethodType = baseMethod.funcType
            let paramTypes = baseMethodType.paramTypes
            paramTypes[0] = RefType.get(patch.classType)
            let retType = baseMethodType.returnType
            let patchMethodType = FuncType.get(paramTypes, retType)

            let baseName = baseMethod.srcCodeName
            let patchMethod = pkg.addFunction(patchMethodType, baseName, baseName, pkg.name)
            patchMethod.setProtected()

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
        let gc = guardClass.getOrThrow()
        let patchableMethodType = patchable.funcType

        let origParamTypes = patchableMethodType.paramTypes
        let methodParamTypes = ArrayList<Type>()
        methodParamTypes.add(RefType.get(gc.classType))
        for (t in origParamTypes) {
            methodParamTypes.add(t)
        }

        let methodType = FuncType.get(methodParamTypes.toArray(), patchableMethodType.returnType)
        let method = pkg.addFunction(methodType, name, name, pkg.name)
        method.setAbstract(true)
        method.setProtected()

        gc.appendMethod(method)
        return method
    }

    /* $patchVar: Option<$Patch&>&  (initialized to None by $guardVarsInit) */
    func genGuardVar(guardClassType: ClassLikeType, forPackageInit: Bool): GlobalVar {
        let gvName = PatchableName(pkg).genGuardVarName(forPackageInit)
        let refToClass = RefType.get(guardClassType)
        let guardVarType = EnumType.get(pluginCtx.optionDef, [refToClass])
        let gv = pkg.addGlobalVar(RefType.get(guardVarType), gvName, gvName, pkg.name)

        if (!forPackageInit) {
            let bg = guardVarsInitializer.getOrThrow().body.getOrThrow()
            let body = bg.entryBlock
            let term = lastExpr(body)
            let b = CHIRBuilder(InsertPosition.Before(term))
            let trueExpr = b.createConstant(BoolLiteral.get(true))
            let tuple = b.createTuple([trueExpr.result])
            tuple.result.ty = guardVarType // override inferred tuple type to the Option enum type
            b.createStore(tuple.result, gv)
        }
        return gv
    }

    /* @$..PatchVarFlag: Bool& = false */
    func genGuardVarFlag(name: String): GlobalVar {
        let gvf = pkg.addGlobalVar(RefType.get(BoolType.get()), name, name, pkg.name)
        gvf.setInitializer(BoolLiteral.get(false))
        return gvf
    }

    /*
     * Rewrites the patchable's entry so that when its flag is set control flows
     * to the guard method; otherwise to the original body. (Non-constructor
     * path; the constructor path is added in the package-init/ctor phase.)
     */
    func genGuardChecks(patchable: Function, guardVarFlag: GlobalVar, guardMethod: Function): Unit {
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

        let callPatchBlock = patchableBody.appendBlock()
        let shouldNotReachHereBlock = patchableBody.appendBlock()
        genShouldNotReachHere(shouldNotReachHereBlock)
        cb.createBranch(guardVarCheckCond.result, callPatchBlock, shouldNotReachHereBlock)

        // callPatchBlock: extract the patch instance and invoke the guard method.
        let classRefType = guardVarBaseType.typeArgs[0] // $Patch&
        let optionType = TupleType.get([BoolType.get(), classRefType])

        let pb = CHIRBuilder(InsertPosition.AtEnd(callPatchBlock))
        let typeCast = pb.createCastToConcrete(guardVarResult, optionType)
        let caller = pb.createField(typeCast.result, pathOf(1))

        let funcType = guardMethod.funcType
        let args = toValues(patchable.parameters)
        let funcCallCtx = FuncCallContext(args, Array<Type>(), classRefType)
        let virMethodCtx = FuncSigInfo(guardMethod.srcCodeName, funcType, guardMethod.genericTypeParams)
        let callContext = InvokeCallContext(caller.result, virMethodCtx, funcCallCtx)
        let callMethod = pb.createInvoke(funcType.returnType, callContext)

        let resultType = callMethod.result.ty
        if (resultType.isUnitType()) {
            let unitConst = pb.createConstant(UnitLiteral.get())
            pb.createStore(unitConst.result, patchable.returnValue.getOrThrow())
        } else {
            pb.createStore(callMethod.result, patchable.returnValue.getOrThrow())
        }
        pb.createExit()
    }

    /*
     * Block: raise Exception("should not reach here").
     */
    func genShouldNotReachHere(block: Block): Unit {
        let b = CHIRBuilder(InsertPosition.AtEnd(block))
        let errMsg = b.createConstant(StringLiteral.get(pluginCtx.stringType, "should not reach here"))

        let exceptionType = pluginCtx.exceptionDef.classType
        let exceptionRefType = RefType.get(exceptionType)
        let allocException = b.createAllocate(exceptionType)

        let ctx = FuncCallContext([allocException.result, errMsg.result], Array<Type>(), exceptionRefType)
        b.createApply(pluginCtx.exceptionInitDef, ctx)

        b.createRaiseException(allocException.result)
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
