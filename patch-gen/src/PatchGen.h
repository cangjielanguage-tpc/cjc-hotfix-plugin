#ifndef CHIR_DIFF_GENERATOR_PATCHGEN_H
#define CHIR_DIFF_GENERATOR_PATCHGEN_H
#include "Equality.h"

#include <exception>
#include <string>
#include <utility>

#include "cangjie/CHIR/IR/CHIRBuilder.h"

#include <iostream>
#include <regex>
#include <sstream>

using namespace Cangjie::CHIR;

namespace PatchGenerator {
class PatchableName;

class PatchGenException : public std::exception {
public:
    explicit PatchGenException(std::string msg)
        : message(std::move(msg))
    {
    }

    const char* what() const noexcept override
    {
        return message.c_str();
    }

private:
    std::string message;
};

static void error(const std::string& msg)
{
    throw PatchGenException(msg);
}

static void error(const bool condition, const std::string& msg)
{
    if (!condition) {
        throw PatchGenException(msg);
    }
}

class MarkContext {
public:
    void markTypeAsImported(const std::string& typeToImport)
    {
#if DEBUG
        std::cout << "mark to import type " + typeToImport << std::endl;
#endif
        typesToImport.insert(typeToImport);
    }

    void markFuncAsImported(Function* funcToImport)
    {
#if DEBUG
        std::cout << "mark to import function" + funcToImport->GetIdentifierWithoutPrefix() << std::endl;
#endif
        funcsToImport.insert(funcToImport);
    }

    void unmarkFuncAsImported(Function* func)
    {
#if DEBUG
        std::cout << "unmark to import function" + func->GetIdentifierWithoutPrefix() << std::endl;
#endif
        funcsToImport.erase(func);
    }

    void markFuncToMakeNonPatchable(Function* funcToMakeNonPatchable)
    {
#if DEBUG
        std::cout << "mark function"  << funcToMakeNonPatchable->GetIdentifierWithoutPrefix() << " as non-patchable" << std::endl;
#endif
        funcsToMakeNonPatchable.insert(funcToMakeNonPatchable);
    }

    void markFuncAsDeleted(const Function* funcToDelete)
    {
#if DEBUG
        std::cout << "mark to delete function" + funcToDelete->GetIdentifierWithoutPrefix() << std::endl;
#endif
        funcsToDelete.insert(funcToDelete->GetIdentifierWithoutPrefix());
    }

    void unmarkFuncAsDeleted(const Function* func)
    {
#if DEBUG
        std::cout << "unmark to delete function" + func->GetIdentifierWithoutPrefix() << std::endl;
#endif
        funcsToDelete.erase(func->GetIdentifierWithoutPrefix());
    }

    void markVarAsImported(GlobalVar* varToImport)
    {
#if DEBUG
        std::cout << "mark to import variable" + varToImport->GetIdentifierWithoutPrefix() << std::endl;
#endif
        varsToImport.insert(varToImport);
    }

    void markAsPatchable(const Function* patched)
    {
#if DEBUG
        std::cout << "mark as patchable function " + patched->GetIdentifierWithoutPrefix() << std::endl;
#endif
        patchables.emplace_back(patched);
    }

    const std::unordered_set<std::string>& getTypesToImport() const
    {
        return typesToImport;
    }

    const std::unordered_set<Function*>& getFuncsToImport() const
    {
        return funcsToImport;
    }

    const std::unordered_set<Function*>& getFuncsToMakeNonPatchable() const
    {
        return funcsToMakeNonPatchable;
    }

    const std::unordered_set<std::string>& getFuncsToDelete() const
    {
        return funcsToDelete;
    }

    const std::unordered_set<GlobalVar*>& getVarsToImport() const
    {
        return varsToImport;
    }

    const std::vector<const Function*>& getPatchables() const
    {
        return patchables;
    }

private:
    std::unordered_set<std::string> typesToImport;
    std::unordered_set<Function*> funcsToImport;
    std::unordered_set<Function*> funcsToMakeNonPatchable;
    std::unordered_set<std::string> funcsToDelete;
    std::unordered_set<GlobalVar*> varsToImport;
    std::vector<const Function*> patchables;
};

class PatchGen {

public:
    PatchGen(std::string fileName, Package* source, Package* patched, CHIRBuilder* builder);

    void createPatch();

private:
    std::string fileName;
    Package* const sourcePackage = nullptr;
    Package* const patchedPackage = nullptr;
    CHIRBuilder* const builder = nullptr;
    MarkContext markContext;

    Function* const guardVarsInitializer = nullptr;
    ClassDef* const guardClass = nullptr;
    GlobalVar* const guardVar = nullptr;

    ClassDef* exceptionDef = nullptr;
    Function* exceptionInitDef = nullptr;

    ClassDef* patchClass = nullptr;

    void genPatch();

    void markInternalFuncsAsNonImportedAreReachableFromPatchable();

    void markInternalFuncAsNonImportedAreReachableFromPatchable(const Function* func, std::unordered_set<const Function*>& visited);

    void mark();

    void markFunc(const Function* sourceFunc, Function* patchedFunc);

    void markFuncsAsNonPatchable(const std::vector<Function*>& patchedGlobalFuncs,
        const std::unordered_set<Function*, FunctionHasher, FunctionEquality>& sourceGlobalFuncs);

    void removeGuardChecks(const Function* patchable) const;

    std::unordered_set<Function*> genPatches();

    void instantiatePatchClass() const;

    void enableGuardVarFlag(const PatchableName& patchableName) const;

    void makeNewPatchableFuncsAsNonPatchable();

    void importFuncs(const std::unordered_set<Function*>& patches) const;

    void importVars() const;

    void importTypes() const;

    void removeRedundantFunctions() const;

    void updatePackageInits() const;

    static Function* findGuardVarsInitializer(const Package* package);

    static ClassDef* findGuardClass(const Package* package);

    static ClassDef* findPatchClass(const Package* package, const ClassDef* patchClass);

    static GlobalVar* findGuardVar(const Package* package);

    static bool isPatchable(const Function* func);

    static bool isPackageInit(const Function* func, const Package* package);
};

static std::string ToCjQualifiedName(const Type& type);

static std::string JoinCjQualifiedNames(const std::vector<Type*>& types, const std::string& delimiter)
{
    std::stringstream ss;
    for (size_t i = 0; i < types.size(); ++i) {
        ss << ToCjQualifiedName(*types[i]);
        if (i < types.size() - 1) {
            ss << delimiter;
        }
    }
    return ss.str();
}

static std::string ToCjQualifiedName(const Type& type)
{
    switch (type.GetTypeKind()) {
        case Type::TYPE_RAWARRAY: {
            const auto& rawArrayType = static_cast<const RawArrayType&>(type);
            return "RawArray<" + ToCjQualifiedName(*rawArrayType.GetElementType()) + ">";
        }
        case Type::TYPE_REFTYPE: {
            const auto& refType = static_cast<const RefType&>(type);
            return ToCjQualifiedName(*refType.GetBaseType());
        }
        case Type::TYPE_VARRAY: {
            const auto& varArrayType = static_cast<const VArrayType&>(type);
            return "VArray<" + ToCjQualifiedName(*varArrayType.GetElementType()) + "," +
                std::to_string(varArrayType.GetSize()) + ">";
        }
        case Type::TYPE_CPOINTER: {
            const auto& cPointerType = static_cast<const CPointerType&>(type);
            return "CPointer<" + ToCjQualifiedName(*cPointerType.GetElementType()) + ">";
        }
        case Type::TYPE_TUPLE: {
            const auto& tupleType = static_cast<const TupleType&>(type);
            return "Tuple<" + JoinCjQualifiedNames(tupleType.GetElementTypes(), ",") + ">";
        }
        case Type::TYPE_STRUCT:
        case Type::TYPE_ENUM:
        case Type::TYPE_CLASS: {
            const auto& customType = static_cast<const CustomType&>(type);
            const auto def = customType.GetCustomTypeDef();
            const auto defName = def->GetSrcCodeIdentifier();
            if (defName.empty()) {
                return def->GetPackageName() + "." + def->GetIdentifierWithoutPrefix();
            }
            return def->GetPackageName() + "." + defName;
        }
        case Type::TYPE_FUNC: {
            const auto& funcType = static_cast<const FuncType&>(type);
            return "(" + JoinCjQualifiedNames(funcType.GetParamTypes(), ",") + ")->" +
                ToCjQualifiedName(*funcType.GetReturnType());
        }
        case Type::TYPE_GENERIC: {
            const auto& genericType = static_cast<const GenericType&>(type);
            return genericType.GetSrcCodeIdentifier();
        }
        case Type::TYPE_BOXTYPE: {
            const auto& boxType = static_cast<const BoxType&>(type);
            return "Box<" + ToCjQualifiedName(*boxType.GetBaseType()) + ">";
        }
        default:
            return type.ToString();
    }
}

/**
 * Represents function/class qualified name.
 * <p>
 * Additionally provides the names of corresponding guard class name, guard var name, etc.
 */
class PatchableName final {
public:
    enum class GuardMethodKind { PLAIN, PACKAGE_INIT_CTOR, PACKAGE_INIT, PACKAGE_LITERAL_INIT };
    enum class PackageInitAccessorKind {PACKAGE_INIT, PACKAGE_LITERAL_INIT};

    explicit PatchableName(const Package* package)
        : PatchableName(std::move(package->GetName()), std::nullopt, std::nullopt, {})
    {
    }

    explicit PatchableName(const CustomTypeDef* typeDef)
        : PatchableName(
              std::move(typeDef->GetPackageName()), std::move(typeDef->GetSrcCodeIdentifier()), std::nullopt, {})
    {
    }

    explicit PatchableName(const Function* func)
        : PatchableName(func->GetPackageName(),
              func->GetParentCustomTypeDef()
                  ? std::optional{std::move(func->GetParentCustomTypeDef()->GetSrcCodeIdentifier())}
                  : std::nullopt,
              func->GetSrcCodeIdentifier(), std::move(func->GetFuncType()->GetParamTypes()))
    {
    }

    std::string getClassName() const
    {
        return className.value();
    }

    std::string getFuncName() const
    {
        return funcName.value();
    }

    std::string getQualifiedName() const
    {
        return qualifiedName;
    }

    std::string genPatchClassName() const
    {
        if (className.has_value()) {
            auto name = className.value();
            return name.insert(name.length() - GUARD_CLASS_SUFFIX.length(), "Impl");
        }
        return BASE_DELIMITER + "PatchImpl" + GUARD_CLASS_SUFFIX;
    }

    std::string genSyntheticCtorName() const
    {
        CJC_ASSERT_WITH_MSG(className.has_value(), "expected class name to be present");
        return genName(BASE_DELIMITER, "$CTOR");
    }

    std::string genPackageInitFlagAccessor(const PackageInitAccessorKind kind, const bool isGetter) const
    {
        std::string suffix;
        switch (kind) {
            case PackageInitAccessorKind::PACKAGE_INIT:
                if (isGetter) {
                    suffix = PACKAGE_INIT_GETTER_NAME;
                } else {
                    suffix = PACKAGE_INIT_SETTER_NAME;
                }
                break;
            case PackageInitAccessorKind::PACKAGE_LITERAL_INIT:
                if (isGetter) {
                    suffix = PACKAGE_LITERAL_INIT_GETTER_NAME;
                } else {
                    suffix = PACKAGE_LITERAL_INIT_SETTER_NAME;
                }
                break;
        }
        CJC_ASSERT_WITH_MSG(!suffix.empty(), "expected suffix to be non empty");
        return genName("", suffix);
    }

    std::string genGuardMethodName(const GuardMethodKind kind) const
    {
        switch (kind) {
            case GuardMethodKind::PLAIN: {
                auto guardMethodName = genName(false, std::nullopt, GUARD_FUNC_SUFFIX);
                guardMethodName.insert(packageName.length() + BASE_DELIMITER.length(), GUARD_CLASS_NAME);
                return guardMethodName;
            }
            case GuardMethodKind::PACKAGE_INIT_CTOR:
                return genName(true, PACKAGE_INIT_CTOR_NAME, std::nullopt);
            case GuardMethodKind::PACKAGE_INIT:
                return genName(true, PACKAGE_INIT_GUARD_METHOD_NAME, std::nullopt);
            case GuardMethodKind::PACKAGE_LITERAL_INIT:
                return genName(true, PACKAGE_LITERAL_INIT_GUARD_METHOD_NAME, std::nullopt);
        }
        throw std::runtime_error("unexpected guard method kind");
    }

    std::string genGuardClassName(const bool forPackageInit) const
    {
        return genName(forPackageInit, PACKAGE_INIT_GUARD_CLASS_NAME, GUARD_CLASS_NAME);
    }

    std::string genGuardVarName(const bool forPackageInit) const
    {
        return genName(forPackageInit, PACKAGE_INIT_GUARD_VAR_NAME, GUARD_VAR_SUFFIX);
    }

    std::string genGuardVarFlagName(const bool forPackageInit) const
    {
        return genName(forPackageInit, PACKAGE_INIT_GUARD_VAR_FLAG_NAME, GUARD_VAR_FLAG_SUFFIX);
    }

    std::string genGuardVarInitializedName() const
    {
        return genName(true, GUARD_VARS_INITIALIZER_NAME, std::nullopt);
    }

    bool operator==(const PatchableName& other) const
    {
        return qualifiedName == other.qualifiedName;
    }

    bool operator!=(const PatchableName& other) const
    {
        return qualifiedName != other.qualifiedName;
    }

    bool operator<(const PatchableName& other) const
    {
        return qualifiedName < other.qualifiedName;
    }

private:
    const std::string BASE_DELIMITER = "$";
    const std::string PARAM_START = "$$";
    const std::string AUX_DELIMITER = "$_";

    const std::string GUARD_CLASS_SUFFIX = BASE_DELIMITER + "GC";
    const std::string GUARD_VAR_SUFFIX = BASE_DELIMITER + "GV";
    const std::string GUARD_VAR_FLAG_SUFFIX = BASE_DELIMITER + "GVF";
    const std::string GUARD_FUNC_SUFFIX = BASE_DELIMITER + "GF";

    const std::string GUARD_VARS_INITIALIZER_NAME = BASE_DELIMITER + "GVI";

    const std::string GUARD_CLASS_NAME = BASE_DELIMITER + "Patch" + GUARD_CLASS_SUFFIX;

    const std::string PACKAGE_INIT_GUARD_CLASS_NAME = BASE_DELIMITER + "PackageInitPatch" + GUARD_CLASS_SUFFIX;
    const std::string PACKAGE_INIT_GUARD_VAR_NAME = BASE_DELIMITER + "packageInit" + GUARD_VAR_SUFFIX;
    const std::string PACKAGE_INIT_GUARD_VAR_FLAG_NAME = BASE_DELIMITER + "packageInit" + GUARD_VAR_FLAG_SUFFIX;
    const std::string PACKAGE_INIT_CTOR_NAME = PACKAGE_INIT_GUARD_CLASS_NAME + BASE_DELIMITER + "<init>" + GUARD_FUNC_SUFFIX;
    const std::string PACKAGE_INIT_GUARD_METHOD_NAME =
        PACKAGE_INIT_GUARD_CLASS_NAME + BASE_DELIMITER + "packageInit" + GUARD_FUNC_SUFFIX;
    const std::string PACKAGE_LITERAL_INIT_GUARD_METHOD_NAME =
        PACKAGE_INIT_GUARD_CLASS_NAME + BASE_DELIMITER + "packageLiteralInit" + GUARD_FUNC_SUFFIX;
    const std::string PACKAGE_INIT_GETTER_NAME = "packageInitGetter" + GUARD_FUNC_SUFFIX;
    const std::string PACKAGE_INIT_SETTER_NAME = "packageInitSetter" + GUARD_FUNC_SUFFIX;
    const std::string PACKAGE_LITERAL_INIT_GETTER_NAME = "packageLiteralInitGetter" + GUARD_FUNC_SUFFIX;
    const std::string PACKAGE_LITERAL_INIT_SETTER_NAME = "packageLiteralInitSetter" + GUARD_FUNC_SUFFIX;

    std::string packageName;
    std::optional<std::string> className;
    std::optional<std::string> funcName;
    std::vector<Type*> funcParams;
    std::string qualifiedName;

    PatchableName(std::string packageName, std::optional<std::string> className, std::optional<std::string> funcName,
        std::vector<Type*>&& funcParams)
        : className(std::move(className)), funcName(std::move(funcName))
    {
        this->packageName = packageName;
        this->qualifiedName = std::move(packageName);
        if (this->className) {
            this->qualifiedName += "." + *this->className;
        }
        if (this->funcName) {
            this->qualifiedName += "." + *this->funcName;
        }
        this->funcParams = funcParams;
    }

    explicit PatchableName(std::string packageName)
        : PatchableName(std::move(packageName), std::nullopt, std::nullopt, {})
    {
    }

    std::string genName(const bool forPackageInit, const std::optional<std::string>& packageInitPatchableSuffix,
        const std::optional<std::string>& patchableSuffix) const
    {
        if (forPackageInit) {
            const auto copy = PatchableName(packageName);
            return copy.genName(GUARD_CLASS_SUFFIX, packageInitPatchableSuffix.value());
        }
        return genName(GUARD_CLASS_SUFFIX, patchableSuffix.value());
    }

    std::string genName(const std::string& classSuffix, const std::string& suffix) const
    {
        std::string name = BASE_DELIMITER + packageName;

        if (className.has_value()) {
            name += BASE_DELIMITER + className.value() + classSuffix;
        }

        if (funcName.has_value()) {
            name += BASE_DELIMITER + funcName.value();

            const auto nfuncParams = funcParams.size();
            if (nfuncParams > 0) {
                name += PARAM_START;
                for (int i = 0; i < nfuncParams; i++) {
                    name += ToCjQualifiedName(*funcParams[i]);
                    if (i < nfuncParams - 1) {
                        name += AUX_DELIMITER;
                    }
                }
            }
        }

        name += suffix;

        return name;
    }
};

}

#endif //CHIR_DIFF_GENERATOR_PATCHGEN_H