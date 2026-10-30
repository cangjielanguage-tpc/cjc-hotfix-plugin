package hotfixplugin

import stdx.chir.*
import std.collection.*

// Guard methods are owned by a nongeneric class. Pass the original enclosing
// context explicitly, before the function's own type arguments.
func patchTypeParams(fn: Function): Array<GenericType> {
    let params = ArrayList<GenericType>()
    if (let Some(parent) <- fn.declaredParent) {
        for (ty in parent.genericTypeParams) {
            params.add(ty)
        }
    }
    for (ty in fn.genericTypeParams) {
        params.add(ty)
    }
    return params.toArray()
}

func replacePatchType(ty: Type, replacements: HashMap<GenericType, Type>): Type {
    if (let Some(generic) <- (ty as GenericType)) {
        return replacements.get(generic) ?? ty
    }
    let args = ty.typeArgs
    let replaced = Array<Type>(args.size, {i => replacePatchType(args[i], replacements)})
    var changed = false
    for (i in 0..args.size) {
        changed = changed || args[i] != replaced[i]
    }
    if (!changed) {
        return ty
    }
    if (let Some(t) <- (ty as ClassLikeType)) {
        return ClassLikeType.get(t.classLikeDef, replaced)
    }
    if (let Some(t) <- (ty as StructType)) {
        return StructType.get(t.structDef, replaced)
    }
    if (let Some(t) <- (ty as EnumType)) {
        return EnumType.get(t.enumDef, replaced)
    }
    if (let Some(t) <- (ty as FuncType)) {
        return FuncType.get(Array<Type>(t.paramTypes.size, {i => replaced[i]}),
            replaced[replaced.size - 1], varArg: t.hasVarArg, cFunc: t.isCFunc)
    }
    if (ty is TupleType) {
        return TupleType.get(replaced)
    }
    if (ty is RefType) {
        return RefType.get(replaced[0])
    }
    if (ty is BoxType) {
        return BoxType.get(replaced[0])
    }
    if (ty is CPointerType) {
        return CPointerType.get(replaced[0])
    }
    if (let Some(t) <- (ty as RawArrayType)) {
        return RawArrayType.get(replaced[0], t.dims)
    }
    if (let Some(t) <- (ty as VArrayType)) {
        return VArrayType.get(replaced[0], t.size)
    }
    throw Exception("Unsupported generic patch type: " + ty.toString())
}

func guardGenericContext(fn: Function, name: String): (Array<GenericType>, HashMap<GenericType, Type>) {
    let original = patchTypeParams(fn)
    let replacements = HashMap<GenericType, Type>()
    if (original.size == fn.genericTypeParams.size) {
        return (original, replacements)
    }
    // Fresh identities avoid treating the original class variable as both a
    // class variable and a function variable in CBC's type-variable cache.
    let lifted = Array<GenericType>(original.size,
        {i => GenericType.get(name + "$T" + i.toString(), original[i].srcCodeName)})
    for (i in 0..original.size) {
        replacements.add(original[i], lifted[i])
    }
    // Populate constraints after the entire mapping exists: bounds can refer
    // to another enclosing or method parameter.
    for (i in 0..original.size) {
        let bounds = original[i].upperBounds
        lifted[i].upperBounds = Array<Type>(bounds.size, {j => replacePatchType(bounds[j], replacements)})
    }
    return (lifted, replacements)
}
