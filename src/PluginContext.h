#ifndef PLUGIN_CONTEXT_H
#define PLUGIN_CONTEXT_H

#include "cangjie/CHIR/IR/CHIRBuilder.h"
#include <regex>
#include <sstream>

namespace HotfixPlugin {
using namespace Cangjie::CHIR;

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

/**
 * Minimal unit for patch creation.
 * <p>
 * Contains pointer to CHIR function to be patched and additional information
 * about corresponding guard class name, guard var name, etc.
 */
struct Patchable final {
    PatchableName funcName;
    Function* func = nullptr;

    bool operator<(const Patchable& other) const
    {
        if (funcName != other.funcName) {
            return funcName < other.funcName;
        }

        const auto firstParamsNum = func->GetNumOfParams();
        const auto secondParamsNum = other.func->GetNumOfParams();
        if (firstParamsNum != secondParamsNum) {
            return firstParamsNum < secondParamsNum;
        }

        const auto firstParams = func->GetParams();
        const auto secondParams = other.func->GetParams();
        for (auto it1 = firstParams.begin(), it2 = secondParams.begin();
            it1 != firstParams.end() && it2 != secondParams.end(); ++it1, ++it2) {
            const auto firstType = (*it1)->GetType();
            const auto secondType = (*it2)->GetType();
            if (firstType != secondType) {
                return ToCjQualifiedName(*firstType) < ToCjQualifiedName(*secondType);
            }
        }

        return false;
    }
};

/**
 * Contains efs to perform proper CHIR generation.
 */
struct PluginContext final {
    EnumDef* const optionDef = nullptr;
    ClassDef* const exceptionDef = nullptr;
    Function* const exceptionInitDef = nullptr;

    explicit PluginContext(EnumDef* optionDef, ClassDef* unsupportedExceptionDef, Function* unsupportedExceptionInitDef)
        : optionDef(optionDef), exceptionDef(unsupportedExceptionDef), exceptionInitDef(unsupportedExceptionInitDef)
    {
    }
};
} // namespace HotfixPlugin
#endif // PLUGIN_CONTEXT_H
