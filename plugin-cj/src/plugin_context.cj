// TODO move to the separate module to be used both from plugin and chir-diff

/*
 * Cangjie port of the name-mangling and data structures from
 * src/PluginContext.h (the `PatchableName` and `Patchable` types).
 *
 * `PatchableName` represents a function/class qualified name and derives the
 * names of the corresponding guard class, guard var, guard flag, guard method,
 * patch class, package-init accessors, etc. It is a verbatim behavioural port
 * of the C++ string logic.
 *
 * Type parameter strings embedded in mangled names use `Type.qualifiedName`,
 * the closest analogue of the C++ `Type::ToSrcCodeString()`.
 */
package hotfixplugin

import stdx.chir.*

// --- name-mangling constants (mirror the const members of PatchableName) ---
let BASE_DELIMITER = "$"
let PARAM_START = "$$"
let AUX_DELIMITER = "$_"

let GUARD_CLASS_SUFFIX = BASE_DELIMITER + "GC"
let GUARD_VAR_SUFFIX = BASE_DELIMITER + "GV"
let GUARD_VAR_FLAG_SUFFIX = BASE_DELIMITER + "GVF"
let GUARD_FUNC_SUFFIX = BASE_DELIMITER + "GF"

let GUARD_VARS_INITIALIZER_NAME = BASE_DELIMITER + "GVI"

let GUARD_CLASS_NAME = BASE_DELIMITER + "Patch" + GUARD_CLASS_SUFFIX

let PACKAGE_INIT_GUARD_CLASS_NAME = BASE_DELIMITER + "PackageInitPatch" + GUARD_CLASS_SUFFIX
let PACKAGE_INIT_GUARD_VAR_NAME = BASE_DELIMITER + "packageInit" + GUARD_VAR_SUFFIX
let PACKAGE_INIT_GUARD_VAR_FLAG_NAME = BASE_DELIMITER + "packageInit" + GUARD_VAR_FLAG_SUFFIX
let PACKAGE_INIT_CTOR_NAME = PACKAGE_INIT_GUARD_CLASS_NAME + BASE_DELIMITER + "<init>" + GUARD_FUNC_SUFFIX
let PACKAGE_INIT_GUARD_METHOD_NAME = PACKAGE_INIT_GUARD_CLASS_NAME + BASE_DELIMITER + "packageInit" + GUARD_FUNC_SUFFIX
let PACKAGE_LITERAL_INIT_GUARD_METHOD_NAME = PACKAGE_INIT_GUARD_CLASS_NAME + BASE_DELIMITER + "packageLiteralInit" +
    GUARD_FUNC_SUFFIX
let PACKAGE_INIT_GETTER_NAME = "packageInitGetter" + GUARD_FUNC_SUFFIX
let PACKAGE_INIT_SETTER_NAME = "packageInitSetter" + GUARD_FUNC_SUFFIX
let PACKAGE_LITERAL_INIT_GETTER_NAME = "packageLiteralInitGetter" + GUARD_FUNC_SUFFIX
let PACKAGE_LITERAL_INIT_SETTER_NAME = "packageLiteralInitSetter" + GUARD_FUNC_SUFFIX


@When[debug_mode == "true"]
func debug(action: () -> Unit): Unit {
    action()
}

@When[debug_mode == "false"]
func debug(action: () -> Unit): Unit {
    // do nothing
}


enum GuardMethodKind {
    | Plain
    | PackageInitCtor
    | PackageInit
    | PackageLiteralInit
}

enum PackageInitAccessorKind {
    | AccPackageInit
    | AccPackageLiteralInit
}

/**
 * Represents a function/class qualified name and derives the associated
 * guard/patch names. Verbatim port of PluginContext.h `PatchableName`.
 */
class PatchableName {
    let packageName: String
    let className: ?String
    let funcName: ?String
    let funcParams: Array<Type>
    let qualifiedName: String

    private init(packageName: String, className: ?String, funcName: ?String, funcParams: Array<Type>) {
        this.packageName = packageName
        this.className = className
        this.funcName = funcName
        this.funcParams = funcParams

        var q = packageName
        if (let Some(c) <- className) {
            q += "." + c
        }
        if (let Some(f) <- funcName) {
            q += "." + f
        }
        this.qualifiedName = q
    }

    init(packageName: String) {
        this(packageName, None, None, Array<Type>())
    }

    init(pkg: Package) {
        this(pkg.name, None, None, Array<Type>())
    }

    init(typeDef: CustomTypeDef) {
        this(typeDef.packageName, Some(typeDef.srcCodeName), None, Array<Type>())
    }

    init(fn: Function) {
        this(fn.packageName, matchParentClassName(fn), Some(fn.srcCodeName), fn.funcType.paramTypes)
    }

    func getClassName(): String {
        return className.getOrThrow()
    }

    func getFuncName(): String {
        return funcName.getOrThrow()
    }

    func getQualifiedName(): String {
        return qualifiedName
    }

    /**
     * Name of the generated patch implementation class. When a guard class name
     * is present, "Impl" is inserted right before the trailing "$GC" suffix;
     * otherwise the package-level default "$PatchImpl$GC" is used.
     */
    func genPatchClassName(): String {
        if (let Some(name) <- className) {
            let pos = name.size - GUARD_CLASS_SUFFIX.size
            return name[..pos] + "Impl" + name[pos..]
        }
        return BASE_DELIMITER + "PatchImpl" + GUARD_CLASS_SUFFIX
    }

    func genSyntheticCtorName(): String {
        // Expects a class name to be present (as in the original assert).
        return genNameWith(BASE_DELIMITER, BASE_DELIMITER + "CTOR")
    }

    func genPackageInitFlagAccessor(kind: PackageInitAccessorKind, isGetter: Bool): String {
        var suffix = ""
        match (kind) {
            case AccPackageInit =>
                suffix = if (isGetter) {
                    PACKAGE_INIT_GETTER_NAME
                } else {
                    PACKAGE_INIT_SETTER_NAME
                }
            case AccPackageLiteralInit =>
                suffix = if (isGetter) {
                    PACKAGE_LITERAL_INIT_GETTER_NAME
                } else {
                    PACKAGE_LITERAL_INIT_SETTER_NAME
                }
        }
        return genNameWith("", suffix)
    }

    func genGuardMethodName(kind: GuardMethodKind): String {
        match (kind) {
            case Plain =>
                var guardMethodName = genName(false, None, GUARD_FUNC_SUFFIX)
                let pos = packageName.size + BASE_DELIMITER.size
                return guardMethodName[..pos] + GUARD_CLASS_NAME + guardMethodName[pos..]
            case PackageInitCtor => return genName(true, PACKAGE_INIT_CTOR_NAME, None)
            case PackageInit => return genName(true, PACKAGE_INIT_GUARD_METHOD_NAME, None)
            case PackageLiteralInit => return genName(true, PACKAGE_LITERAL_INIT_GUARD_METHOD_NAME, None)
        }
    }

    func genGuardClassName(forPackageInit: Bool): String {
        return genName(forPackageInit, PACKAGE_INIT_GUARD_CLASS_NAME, GUARD_CLASS_NAME)
    }

    func genGuardVarName(forPackageInit: Bool): String {
        return genName(forPackageInit, PACKAGE_INIT_GUARD_VAR_NAME, GUARD_VAR_SUFFIX)
    }

    func genGuardVarFlagName(forPackageInit: Bool): String {
        return genName(forPackageInit, PACKAGE_INIT_GUARD_VAR_FLAG_NAME, GUARD_VAR_FLAG_SUFFIX)
    }

    func genGuardVarInitializedName(): String {
        return genName(true, GUARD_VARS_INITIALIZER_NAME, None)
    }

    // --- private helpers (mirror the private genName overloads) ---

    private func genName(forPackageInit: Bool, packageInitPatchableSuffix: ?String, patchableSuffix: ?String): String {
        if (forPackageInit) {
            let copy = PatchableName(packageName)
            return copy.genNameWith(GUARD_CLASS_SUFFIX, packageInitPatchableSuffix.getOrThrow())
        }
        return genNameWith(GUARD_CLASS_SUFFIX, patchableSuffix.getOrThrow())
    }

    private func genNameWith(classSuffix: String, suffix: String): String {
        var name = BASE_DELIMITER + packageName

        if (let Some(c) <- className) {
            name += BASE_DELIMITER + c + classSuffix
        }

        if (let Some(f) <- funcName) {
            name += BASE_DELIMITER + f

            let nfuncParams = funcParams.size
            if (nfuncParams > 0) {
                name += PARAM_START
                for (i in 0..nfuncParams) {
                    name += funcParams[i].qualifiedName
                    if (i < nfuncParams - 1) {
                        name += AUX_DELIMITER
                    }
                }
            }
        }

        name += suffix
        return name
    }
}

private func matchParentClassName(fn: Function): ?String {
    if (let Some(parent) <- fn.declaredParent) {
        return Some(parent.srcCodeName)
    }
    return None
}

// --- func-kind / attribute inspection ------------------------------------
// The public stdx.chir API does not expose the internal FuncKind enum or the
// Attribute test API, but Function.toString() prints both in its header line
// (e.g. "funcKind: StructConstructor" and "[initializer][mut]"). We parse the
// header to faithfully reproduce the C++ `GetFuncKind()` / `TestAttr()` checks.

/** First (header) line of a function's textual representation. */
func funcHeaderLine(fn: Function): String {
    let s = fn.toString()
    match (s.indexOf("\n")) {
        case Some(i) => return s[..i]
        case None => return s
    }
}

/** The FuncKind name (e.g. "StructConstructor"), or "Default" if unmarked. */
func funcKindStr(fn: Function): String {
    let h = funcHeaderLine(fn)
    let key = "funcKind: "
    if (let Some(i) <- h.indexOf(key)) {
        let rest = h[i + key.size..]
        match (rest.indexOf(",")) {
            case Some(j) => return rest[..j]
            case None => return rest
        }
    }
    return "Default"
}

/** True if the function header carries the given attribute token, e.g. "mut". */
func hasAttrToken(fn: Function, token: String): Bool {
    return funcHeaderLine(fn).contains("[" + token + "]")
}

/** True if `fn` is any kind of constructor. */
func isConstructorFn(fn: Function): Bool {
    if (fn.srcCodeName == "init" && fn.declaredParent.isSome()) {
        return true
    }
    let k = funcKindStr(fn)
    return k == "ClassConstructor" || k == "PrimaryClassConstructor" || k == "StructConstructor" ||
        k == "PrimaryStructConstructor"
}

/** True if `fn` is a struct constructor (primary or not) — C++ STRUCT/PRIMAL. */
func isStructConstructorFn(fn: Function): Bool {
    if (fn.srcCodeName == "init") {
        if (let Some(parent) <- fn.declaredParent) {
            return parent.isStruct()
        }
    }
    let k = funcKindStr(fn)
    return k == "StructConstructor" || k == "PrimaryStructConstructor"
}

/** True if `fn` is an annotation-factory function (C++ ANNOFACTORY_FUNC). */
func isAnnoFactoryFn(fn: Function): Bool {
    return funcKindStr(fn) == "AnnoFactoryFunc"
}

/** True if `fn` carries the INITIALIZER attribute. */
func hasInitializerAttr(fn: Function): Bool {
    return hasAttrToken(fn, "initializer")
}

/** True if `fn` carries the MUT attribute. */
func hasMutAttr(fn: Function): Bool {
    return hasAttrToken(fn, "mut")
}

/**
 * Symbol lookups needed for CHIR generation. Port of PluginContext.h
 * `PluginContext` together with EntryPoint::createPluginContext.
 *
 * Replaces the C++ `builder.GetXxxTy()` / `GetImported*` helpers with public
 * lookups over the deserialized package.
 */
class PluginContext {
    let optionDef: EnumDef
    let exceptionDef: ClassLikeDef
    let exceptionInitDef: Function
    let stringType: Type
    let objectType: ClassLikeType

    init(optionDef: EnumDef, exceptionDef: ClassLikeDef, exceptionInitDef: Function, stringType: Type,
        objectType: ClassLikeType) {
        this.optionDef = optionDef
        this.exceptionDef = exceptionDef
        this.exceptionInitDef = exceptionInitDef
        this.stringType = stringType
        this.objectType = objectType
    }
}

func createPluginContext(pkg: Package): PluginContext {
    // Option enum (std.core.Option) — used to type the guard variable.
    var optionDef: ?EnumDef = None
    for (e in pkg.importedEnumDefs) {
        if (e.srcCodeName == "Option") {
            optionDef = e
            break
        }
    }
    let option = optionDef ?? throw Exception("unable to find Option definition")

    // The String struct type — used both to find Exception.<init>(String) and
    // to build the "should not reach here" message constant.
    var stringType: ?Type = None
    for (s in pkg.importedStructDefs) {
        if (s.srcCodeName == "String") {
            stringType = s.ty
            break
        }
    }
    let stringTy = stringType ?? throw Exception("unable to find String definition")

    // Object class type — used as the guard classes' super class.
    var objectType: ?ClassLikeType = None
    for (c in pkg.importedClassDefs) {
        if (c.srcCodeName == "Object") {
            objectType = c.classType
            break
        }
    }
    let objectTy = objectType ?? throw Exception("unable to find Object definition")

    // Exception class + its (String) constructor.
    var exceptionDef: ?ClassLikeDef = None
    var exceptionInitDef: ?Function = None
    for (c in pkg.importedClassDefs) {
        if (c.srcCodeName == "Exception") {
            exceptionDef = c
            for (m in c.methods) {
                // Imported declarations do not carry a funcKind in their header,
                // so detect the constructor by its source name ("init").
                if (m.srcCodeName == "init" && m.parameters.size == 2 && m.parameters[1].ty == stringTy) {
                    exceptionInitDef = m
                    break
                }
            }
            break
        }
    }
    let excDef = exceptionDef ?? throw Exception("unable to find Exception definition")
    let excInit = exceptionInitDef ?? throw Exception("unable to find Exception.<init>(String) definition")

    return PluginContext(option, excDef, excInit, stringTy, objectTy)
}

/**
 * Minimal unit for patch creation: a CHIR function to be patched plus its
 * PatchableName. Port of PluginContext.h `Patchable`.
 */
class Patchable {
    let funcName: PatchableName
    let fn: Function

    init(funcName: PatchableName, fn: Function) {
        this.funcName = funcName
        this.fn = fn
    }

    /**
     * Total order matching the C++ `operator<`: first by qualified name, then by
     * parameter count, then by parameter type source strings. Returns true if
     * `this` sorts strictly before `other`.
     */
    func lessThan(other: Patchable): Bool {
        if (funcName.qualifiedName != other.funcName.qualifiedName) {
            return funcName.qualifiedName < other.funcName.qualifiedName
        }

        let firstParams = fn.parameters
        let secondParams = other.fn.parameters
        if (firstParams.size != secondParams.size) {
            return firstParams.size < secondParams.size
        }

        for (i in 0..firstParams.size) {
            let firstType = firstParams[i].ty
            let secondType = secondParams[i].ty
            if (firstType != secondType) {
                return firstType.qualifiedName < secondType.qualifiedName
            }
        }
        return false
    }

    /** Dedup/order key equivalent to the C++ std::set identity. */
    func sortKey(): String {
        let sb = StringBuilder()
        sb.append(funcName.qualifiedName)
        sb.append("#")
        sb.append(fn.parameters.size.toString())
        for (p in fn.parameters) {
            sb.append("#")
            sb.append(p.ty.qualifiedName)
        }
        return sb.toString()
    }
}
