/*
 * Cangjie port of src/PatchableFinder.cpp.
 *
 * Scans a package for functions/methods that should be made patchable — either
 * because they carry the `@patchable` annotation or because their qualified
 * name matches the regex filter derived from the cjpm.toml directive — and
 * returns them as a deterministically ordered, deduplicated list of Patchable.
 *
 * Note on the shipped (non-TEST) semantics: `matchesRegexpFilter` is permissive
 * when no filter is present (everything matches), matching the original build
 * without the TEST define.
 */
package hotfixplugin

import stdx.chir.*
import std.collection.*
import std.regex.*

const MAIN_INVOKE = "$mainInvoke"

private func isPatchableAnno(annos: Array<CustomAnnoInstance>): Bool {
    for (a in annos) {
        if (a.classSrcCodeName == "patchable") {
            return true
        }
    }
    return false
}

private func matchesRegexpFilter(regexpFilter: ?Regex, name: String): Bool {
    if (envSet("HOTFIX_TEST_MODE")) {
        match (regexpFilter) {
            case Some(r) => return r.matches(name)
            case None => return false
        }
    }
    // Non-TEST behaviour: matches if there is no filter, or the filter matches.
    match (regexpFilter) {
        case Some(r) => return r.matches(name)
        case None => return true
    }
}

private func matches(annos: Array<CustomAnnoInstance>, regexpFilter: ?Regex, name: String): Bool {
    return isPatchableAnno(annos) || matchesRegexpFilter(regexpFilter, name)
}

func findPatchables(pkg: Package, regexpFilter: ?Regex): ArrayList<Patchable> {
    let packagePatchableName = PatchableName(pkg)
    let guardVarInitializerName = packagePatchableName.genGuardVarInitializedName()
    let packageInitGuardClassName = packagePatchableName.genGuardClassName(true)
    let packageInitFlagGetter = packagePatchableName.genPackageInitFlagAccessor(AccPackageInit, false)
    let packageInitFlagSetter = packagePatchableName.genPackageInitFlagAccessor(AccPackageInit, true)
    let packageLiteralInitFlagGetter = packagePatchableName.genPackageInitFlagAccessor(AccPackageLiteralInit, false)
    let packageLiteralInitFlagSetter = packagePatchableName.genPackageInitFlagAccessor(AccPackageLiteralInit, true)

    let collected = ArrayList<Patchable>()
    let seen = HashSet<String>()

    for (fn in pkg.functions) {
        // Only functions with a body defined in the current package.
        if (fn.body.isNone()) {
            continue
        }
        if (fn.isImported() || fn.packageName != pkg.name) {
            continue
        }

        // No need to generate guards for initializers and anno factories (they
        // are reachable from package inits), constructors, or mut methods.
        //
        // The C++ implementation supports class constructors by marking a
        // synthetic constructor with the internal SKIP_ANALYSIS attribute. That
        // attribute is not exposed through the public stdx.chir API used by this
        // Cangjie port, so class constructors are excluded here as well.
        if (hasInitializerAttr(fn) || isAnnoFactoryFn(fn) || (isConstructorFn(fn) && !isStubSafeConstructor(fn)) ||
            isStructConstructorFn(fn) || hasMutAttr(fn)) {
            continue
        }

        // No need to generate guards for plugin-generated methods.
        let funcIdentifier = fn.srcCodeName
        if (funcIdentifier == MAIN_INVOKE || funcIdentifier == guardVarInitializerName ||
            funcIdentifier == packageInitFlagGetter || funcIdentifier == packageInitFlagSetter ||
            funcIdentifier == packageLiteralInitFlagGetter || funcIdentifier == packageLiteralInitFlagSetter) {
            continue
        }

        // No need to generate guards for plugin-generated package inits' guard methods.
        let declType = fn.declaredParent
        if (let Some(dt) <- declType) {
            if (dt.srcCodeName == packageInitGuardClassName) {
                continue
            }
        }

        let funcName = PatchableName(fn)
        var isMatch = matches(fn.customAnnoInstances, regexpFilter, funcName.getQualifiedName())
        if (!isMatch) {
            if (let Some(dt) <- declType) {
                isMatch = matches(dt.customAnnoInstances, regexpFilter, PatchableName(dt).getQualifiedName())
            }
        }

        if (isMatch) {
            let patchable = Patchable(funcName, fn)
            if (seen.add(patchable.sortKey())) {
                collected.add(patchable)
            }
        }
    }

    return sortPatchables(collected)
}

private func isStubSafeConstructor(fn: Function): Bool {
    if (!envSet("HOTFIX_STUB_TEST")) {
        return false
    }
    if (let Some(parent) <- fn.declaredParent) {
        return parent.instanceVars.isEmpty()
    }
    return false
}

/** Insertion sort using the C++ operator< total order (small lists). */
private func sortPatchables(items: ArrayList<Patchable>): ArrayList<Patchable> {
    let arr = items
    for (i in 1..arr.size) {
        let cur = arr[i]
        var j = i - 1
        while (j >= 0 && cur.lessThan(arr[j])) {
            arr[j + 1] = arr[j]
            j -= 1
        }
        arr[j + 1] = cur
    }
    return arr
}
