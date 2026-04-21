#ifndef PLUGIN_CONTEXT_H
#define PLUGIN_CONTEXT_H

#include "cangjie/CHIR/IR/CHIRBuilder.h"
#include <regex>

namespace HotfixPlugin {
using namespace Cangjie::CHIR;

const std::string PATCHABLE_GUARD_VAR_FLAG_NAME_SUFFIX = "PatchVarFlag";

const std::string PATCHABLE_GUARD_VARS_INITIALIZER = "$guardVarsInit";
const std::string PATCHABLE_GUARD_CLASS_NAME = "$Patch";
const std::string PATCHABLE_GUARD_VAR_NAME = "$patchVar";

const std::string PACKAGE_INIT_GUARD_CLASS_NAME = "$PackageInitPatch";
const std::string PACKAGE_INIT_GUARD_VAR_NAME = "$packageInitPatchVar";
const std::string PACKAGE_INIT_GUARD_VAR_FLAG_NAME = "$packageInit" + PATCHABLE_GUARD_VAR_FLAG_NAME_SUFFIX;
const std::string PACKAGE_INIT_GUARD_METHOD_NAME = "packageInitPatched";
const std::string PACKAGE_LITERAL_INIT_GUARD_METHOD_NAME = "packageLiteralInitPatched";

/**
 * Represents function/class qualified name.
 * <p>
 * Additionally provides the names of corresponding guard class name, guard var name, etc.
 */
class PatchableName final {
public:
    explicit PatchableName(const CustomTypeDef* typeDef)
        : PatchableName(
            std::move(typeDef->GetPackageName()), std::move(typeDef->GetSrcCodeIdentifier()), std::nullopt, {})
    {
    }

    explicit PatchableName(const Function* func)
        : PatchableName(
            func->GetPackageName(),
            func->GetParentCustomTypeDef()
            ? std::optional{std::move(func->GetParentCustomTypeDef()->GetSrcCodeIdentifier())}
            : std::nullopt,
            func->GetSrcCodeIdentifier(),
            std::move(func->GetFuncType()->GetParamTypes()))
    {
    }

    std::string getFuncName() const
    {
        return funcName.value();
    }

    std::string getQualifiedName() const
    {
        return qualifiedName;
    }

    std::string getGuardMethodName() const
    {
        return genGuardName("Patch");
    }

    std::string getGuardVarFlagName() const
    {
        return genGuardName("PatchVarFlag");
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
    std::optional<std::string> className;
    std::optional<std::string> funcName;
    std::vector<Type*> funcParams;
    std::string qualifiedName;

    PatchableName(std::string packageName, std::optional<std::string> className, std::optional<std::string> funcName,
        std::vector<Type*>&& funcParams)
        : className(std::move(className)),
          funcName(std::move(funcName))
    {
        this->qualifiedName = std::move(packageName);
        if (this->className) {
            this->qualifiedName += "." + *this->className;
        }
        if (this->funcName) {
            this->qualifiedName += "." + *this->funcName;
        }
        this->funcParams = funcParams;
    }

    std::string genGuardName(const std::string& postfix) const
    {
        auto toLowerCase = [](std::string& s) -> std::string& {
            const auto value = s[0];
            s[0] = static_cast<char>(std::tolower(static_cast<unsigned char>(value)));
            return s;
        };
        auto toUpperCase = [](std::string& s) -> std::string& {
            const auto value = s[0];
            s[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(value)));
            return s;
        };
        std::string name = "$";
        if (className.has_value()) {
            auto n = className.value();
            name += toLowerCase(n);
        }
        if (funcName.has_value()) {
            auto n = funcName.value();
            name += className.has_value() ? toUpperCase(n) : n;
            for (const auto param : funcParams) {
                name += "_" + param->ToSrcCodeString();
            }
        }
        name += "_" + postfix;
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
                return firstType->ToSrcCodeString() < secondType->ToSrcCodeString();
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
        : optionDef(optionDef),
          exceptionDef(unsupportedExceptionDef),
          exceptionInitDef(unsupportedExceptionInitDef)
    {
    }
};
}
#endif // PLUGIN_CONTEXT_H