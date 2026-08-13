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

        let packageInitGuardClass = genPackageInitGuardClass()
        let packageInitGuardClassType = packageInitGuardClass.classType
        let packageInitGuardVar = genGuardVar(packageInitGuardClassType, true)
        // C++ uses GlobalVar::SetInitFunc(packageInitFunc). Public stdx.chir
        // exposes the same storage through setInitializer(Value), and Function
        // is a Value.
        packageInitGuardVar.setInitializer(pkg.packageInitFunc.getOrThrow())
        let packageInitGuardVarFlag = genGuardVarFlag(PatchableName(pkg).genGuardVarFlagName(true))
        let guardClassMethods = packageInitGuardClass.methods
        if (guardClassMethods.size != 3) {
            throw Exception("expected package init guard class to have 3 methods")
        }
        let guardClassCtor = guardClassMethods[2]

        genPackageInitGuardChecks(pkg.packageInitFunc.getOrThrow(), packageInitGuardClassType, packageInitGuardVar,
            packageInitGuardVarFlag, guardClassMethods[0], guardClassCtor)
        genPackageInitGuardChecks(findPackageLiteralInitFunc(pkg), packageInitGuardClassType,
            packageInitGuardVar, packageInitGuardVarFlag, guardClassMethods[1], guardClassCtor)

        let packageInitFlag = findGlobalVar(pkg, "$has_applied_pkg_init_func")
        let packageInitLiteralFlag = findGlobalVar(pkg, "has_invoked_pkg_init_literal")
        genPackageInitAccessors(packageInitFlag, AccPackageInit)
        genPackageInitAccessors(packageInitLiteralFlag, AccPackageLiteralInit)
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

    /*
     * Generates:
     *   open/internal class $PackageInitPatch$GC <: Object {
     *     protected open func packageInit...() { shouldNotReachHere }
     *     protected open func packageLiteralInit...() { shouldNotReachHere }
     *     public init() { Exit }
     *   }
     */
    func genPackageInitGuardClass(): ClassLikeDef {
        debug { =>
            println("Create guard class for package inits")
        }
        let packagePatchableName = PatchableName(pkg)
        let clName = packagePatchableName.genGuardClassName(true)
        let cl = pkg.addClass(clName, clName, true)
        cl.superClass = pluginCtx.objectType
        cl.setOpen(true)
        cl.setInternal()
        // C++ also marks this class COMPILER_ADD and external. Public stdx.chir
        // does not expose those setters.

        let paramTypes = Array<Type>(1, {_ => RefType.get(cl.classType)})

        let genPatchableMethod = {name: String =>
            let m = createEmptyFunctionWithUnitRetVal(name, paramTypes)
            m.setOpen(true)
            m.setProtected()
            // C++ also sets NO_INLINE.
            let block = m.body.getOrThrow().entryBlock
            genShouldNotReachHere(block)
            cl.appendMethod(m)
            debug { =>
                println("method:")
                println(m.toString())
            }
        }

        genPatchableMethod(packagePatchableName.genGuardMethodName(PackageInit))
        genPatchableMethod(packagePatchableName.genGuardMethodName(PackageLiteralInit))

        let ctorName = packagePatchableName.genGuardMethodName(PackageInitCtor)
        let ctor = createEmptyFunctionWithUnitRetVal(ctorName, paramTypes)
        ctor.setPublic()
        // C++ sets FuncKind to CLASS_CONSTRUCTOR. Public stdx.chir does not
        // expose a func-kind setter for plugin code.
        CHIRBuilder(InsertPosition.AtEnd(ctor.body.getOrThrow().entryBlock)).createExit()
        cl.appendMethod(ctor)

        debug { =>
            println("ctor:")
            println(ctor.toString())
            println("result:")
            println(cl.toString())
        }
        return cl
    }

    /*
     * Rewrites a package init entry so that, when the package-init guard flag
     * is enabled, it obtains or creates a $PackageInitPatch$GC instance and
     * invokes the corresponding virtual guard method instead of running the
     * original package init body.
     */
    func genPackageInitGuardChecks(patchable: Function, guardClassType: ClassLikeType, packageInitGuardVar: GlobalVar,
        packageInitGuardVarFlag: GlobalVar, guardMethod: Function, guardClassCtor: Function): Unit {
        debug { =>
            println("Gen package init guard check")
        }
        let bg = patchable.body.getOrThrow()
        let entryBlock = bg.entryBlock
        let oldEntrySuccessors = lastExpr(entryBlock).successors
        if (oldEntrySuccessors.size != 2) {
            throw Exception("entry block of package inits is expected to have two successors")
        }
        if (entryBlock.predecessors.size != 0) {
            throw Exception("entry block of package inits is expected to have zero predecessors")
        }

        // C++ creates a new outer entry block before the old package-init entry:
        //
        //   newEntryBlock -> oldEntryBlock
        //
        // and then replaces that goto with the packageInitGuardVarFlag check.
        // Public stdx.chir has only BlockGroup.appendBlock(), so it cannot
        // insert a block before the current entry in BlockGroup.blocks. To keep
        // the flag check in the outer/first block, use the existing entry block
        // as C++'s newEntryBlock and move the original entry contents into an
        // appended continuation block.
        let originalEntryBodyBlock = bg.appendBlock()
        moveBlockExpressions(entryBlock, originalEntryBodyBlock)

        let guardVarCheckBlock = bg.appendBlock()
        let eb = CHIRBuilder(InsertPosition.AtEnd(entryBlock))
        let loadGuardVarFlag = eb.createLoad(packageInitGuardVarFlag)
        let guardVarFlagCond = eb.createUnaryNot(loadGuardVarFlag.result)
        eb.createBranch(guardVarFlagCond.result, originalEntryBodyBlock, guardVarCheckBlock)

        let guardVarBaseType = refBaseType(packageInitGuardVar.ty) // Option<$PackageInitPatch$GC&>

        let cb = CHIRBuilder(InsertPosition.AtEnd(guardVarCheckBlock))
        let guardVarAlloc = cb.createAllocate(guardVarBaseType)
        let loadGuardVar = cb.createLoad(packageInitGuardVar)
        let field = cb.createField(loadGuardVar.result, pathOf(0))
        let falseConst = cb.createConstant(BoolLiteral.get(false))
        let guardVarCheckCond = cb.createBinaryEqual(field.result, falseConst.result)

        let guardVarInitializedBlock = bg.appendBlock()
        let guardVarNotInitializedBlock = bg.appendBlock()
        let callPatchBlock = bg.appendBlock()
        cb.createBranch(guardVarCheckCond.result, guardVarInitializedBlock, guardVarNotInitializedBlock)

        let ib = CHIRBuilder(InsertPosition.AtEnd(guardVarInitializedBlock))
        ib.createStore(loadGuardVar.result, guardVarAlloc.result)
        ib.createGoTo(callPatchBlock)

        let guardClassRefType = RefType.get(guardClassType)
        let nb = CHIRBuilder(InsertPosition.AtEnd(guardVarNotInitializedBlock))
        let patchAlloc = nb.createAllocate(guardClassType)
        nb.createApply(guardClassCtor, FuncCallContext([patchAlloc.result], Array<Type>(), guardClassRefType))
        let newFalseConst = nb.createConstant(BoolLiteral.get(false))
        let tuple = nb.createTuple([newFalseConst.result, patchAlloc.result])
        tuple.result.ty = guardVarBaseType
        nb.createStore(tuple.result, guardVarAlloc.result)
        nb.createStore(loadGuardVar.result, packageInitGuardVar)
        nb.createGoTo(callPatchBlock)

        let pb = CHIRBuilder(InsertPosition.AtEnd(callPatchBlock))
        let loadedGuardVar = pb.createLoad(guardVarAlloc.result)
        let optionType = TupleType.get([BoolType.get(), guardClassRefType])
        // C++ creates a CHIR TypeCast here. Public stdx.chir currently exposes
        // createNumericCast for plugin code, and this is the available wrapper
        // used elsewhere in this port for the same Option-enum to Tuple cast.
        let typeCast = pb.createNumericCast(loadedGuardVar.result, optionType, OverflowStrategy.Throwing)
        let caller = pb.createField(typeCast.result, pathOf(1))
        let funcType = guardMethod.funcType
        let funcCallCtx = FuncCallContext(Array<Value>(), Array<Type>(), guardClassRefType)
        let virMethodCtx = FuncSigInfo(guardMethod.srcCodeName, funcType, guardMethod.genericTypeParams)
        let callContext = InvokeCallContext(caller.result, virMethodCtx, funcCallCtx)
        pb.createInvoke(funcType.returnType, callContext)
        pb.createExit()

        debug { =>
            println(patchable.toString())
        }
    }

    /*
     * Generates external package init flag getter/setter functions.
     */
    func genPackageInitAccessors(flag: GlobalVar, kind: PackageInitAccessorKind): Unit {
        let packagePatchableName = PatchableName(pkg)

        let getterName = packagePatchableName.genPackageInitFlagAccessor(kind, true)
        let getterType = FuncType.get(Array<Type>(), BoolType.get())
        let getter = pkg.addFunction(getterType, getterName, getterName, pkg.name)
        getter.setInternal()
        getter.initBody()
        let getterEntry = getter.body.getOrThrow().entryBlock
        let gb = CHIRBuilder(InsertPosition.AtEnd(getterEntry))
        let getterRetVal = gb.createAllocate(BoolType.get())
        getter.returnValue = getterRetVal.result
        let load = gb.createLoad(flag)
        gb.createStore(load.result, getterRetVal.result)
        gb.createExit()

        let setterName = packagePatchableName.genPackageInitFlagAccessor(kind, false)
        let setter = createEmptyFunctionWithUnitRetVal(setterName, [BoolType.get()])
        let setterEntry = setter.body.getOrThrow().entryBlock
        let sb = CHIRBuilder(InsertPosition.AtEnd(setterEntry))
        sb.createStore(setter.parameters[0], flag)
        let unitConst = sb.createConstant(UnitLiteral.get())
        sb.createStore(unitConst.result, setter.returnValue.getOrThrow())
        sb.createExit()

        debug { =>
            println("getter for package init:")
            println(getter.toString())
            println("setter for package init:")
            println(setter.toString())
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
        debug { =>
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
        debug { =>
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
        debug { =>
            println("guard class with new guard method:")
            println(gc.toString())
        }
        return method
    }

    /* $patchVar: Option<$Patch&>&  (initialized to None by $guardVarsInit) */
    func genGuardVar(guardClassType: ClassLikeType, forPackageInit: Bool): GlobalVar {
        let gvName = PatchableName(pkg).genGuardVarName(forPackageInit)
        debug { =>
            println("Gen guard var " + gvName + " for class " + guardClassType.toString())
        }
        let refToClass = RefType.get(guardClassType)
        let guardVarType = EnumType.get(pluginCtx.optionDef, [refToClass])
        let gv = pkg.addGlobalVar(RefType.get(guardVarType), gvName, gvName, pkg.name)
        // C++ marks guard vars external. Public stdx.chir does not expose
        // linkage setters.

        if (!forPackageInit) {
            debug { =>
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
            debug { =>
                println("guard vars initializer body:")
                println(bg.toString())
            }
        }
        debug { =>
            println(gv.toString())
        }
        return gv
    }

    /* @$..PatchVarFlag: Bool& = false */
    func genGuardVarFlag(name: String): GlobalVar {
        debug { =>
            println("Gen guard var flag " + name)
        }
        let gvf = pkg.addGlobalVar(RefType.get(BoolType.get()), name, name, pkg.name)
        gvf.setInitializer(BoolLiteral.get(false))
        // C++ marks guard flag vars external. Public stdx.chir does not expose
        // linkage setters.
        debug { =>
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
        debug { =>
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

        let modifiedPatchableBuf = modifiedPatchable
        debug { =>
            if (isConstructorFn(patchable)) {
                println("new ctor:")
                println(modifiedPatchableBuf.toString())
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

/** Move every expression from one block to another, preserving order. */
func moveBlockExpressions(from: Block, to: Block): Unit {
    let anchor = CHIRBuilder(InsertPosition.AtEnd(to)).createConstant(UnitLiteral.get())
    for (expr in from.exprs) {
        if (expr.isTerminator()) {
            expr.moveAfter(anchor)
        } else {
            expr.moveBefore(anchor)
        }
    }
    anchor.destroySelf()
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

func findGlobalVar(pkg: Package, name: String): GlobalVar {
    for (gv in pkg.globalVars) {
        if (gv.srcCodeName == name) {
            return gv
        }
    }
    throw Exception("unable to find global var " + name)
}

func findPackageLiteralInitFunc(pkg: Package): Function {
    for (fn in pkg.functions) {
        if (fn.packageName == pkg.name && fn.srcCodeName.startsWith("_CGP") && fn.srcCodeName.endsWith("ilHv")) {
            return fn
        }
    }
    throw Exception("unable to find package literal init function")
}
