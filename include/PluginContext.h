#ifndef PLUGIN_CONTEXT_H
#define PLUGIN_CONTEXT_H

#include "cangjie/CHIR/IR/CHIRBuilder.h"
#include <regex>

namespace HotfixPlugin {
using namespace Cangjie;

/**
 * Represents function/class qualified name.
 * <p>
 * Additionally provides the names of corresponding guard class name, patch field name, etc.
 */
class PatchableName final {
public:
    explicit PatchableName(const CHIR::ClassDef* classDef)
        : PatchableName(
            std::move(classDef->GetPackageName()), std::move(classDef->GetSrcCodeIdentifier()), std::nullopt)
    {
    }

    explicit PatchableName(const CHIR::FuncBase* func)
        : PatchableName(
            func->GetPackageName(),
            func->GetParentCustomTypeDef()
            ? std::optional{std::move(func->GetParentCustomTypeDef()->GetSrcCodeIdentifier())}
            : std::nullopt,
            func->GetSrcCodeIdentifier())
    {
    }

    std::string GetFuncName() const
    {
        return funcName.value();
    }

    std::string GetQualifiedName() const
    {
        return qualifiedName;
    }

    // TODO cache
    std::string GetPatchClassName() const
    {
        auto toUpperCase = [](std::string& s) -> std::string& {
            const auto value = s[0];
            s[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(value)));
            return s;
        };
        std::string patchName;
        if (className.has_value()) {
            auto n = className.value();
            patchName += toUpperCase(n);
        }
        if (funcName.has_value()) {
            auto n = funcName.value();
            patchName += toUpperCase(n);
        }
        patchName += "Patch";
        return patchName;
    }

    std::string GetPatchFieldName() const
    {
        auto name = funcName.value();
        name += "Patch";
        return name;
    }

    bool operator<(const PatchableName& other) const
    {
        return qualifiedName < other.qualifiedName;
    }

private:
    std::optional<std::string> className;
    std::optional<std::string> funcName;
    std::string qualifiedName;

    PatchableName(std::string packageName, std::optional<std::string> className,
        std::optional<std::string> funcName)
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
    }
};

/**
 * Minimal unit for patch creation.
 * <p>
 * Contains pointer to CHIR function to be patched and additional information
 * about corresponding guard class name, patch field name, etc.
 */
struct Patchable final {
    PatchableName funcName;
    CHIR::FuncBase* func = nullptr;

    bool operator<(const Patchable& other) const
    {
        return funcName < other.funcName;
    }
};

/**
 * Contains class/method defs to perform proper CHIR generation.
 */
struct PluginContext final {
    CHIR::EnumDef* const optionDef = nullptr;
    CHIR::FuncBase* const optionIsNoneDef = nullptr;
    CHIR::FuncBase* const optionGetOrThrowDef = nullptr;
    CHIR::Func* const packageInit = nullptr;

    PluginContext(CHIR::EnumDef* optionDef, CHIR::FuncBase* optionIsNoneDef,
        CHIR::FuncBase* optionGetOrThrowDef, CHIR::Func* packageInit)
        : optionDef(optionDef),
          optionIsNoneDef(optionIsNoneDef),
          optionGetOrThrowDef(optionGetOrThrowDef),
          packageInit(packageInit)
    {
    }
};

/**
 * Checks if the class/method contain @patchable annotation and it's qualified name matches to regexp filter.
 */
class PatchableFilter final {
public:
    enum class Kind {
        NONE,
        ONLY_REGEXP,
        ALL
    };

    explicit PatchableFilter(const Kind kind,
        const std::optional<std::regex>& regexpFilter)
        : regexpFilter(regexpFilter)
    {
        switch (kind) {
            case Kind::NONE:
                checkIsPatchable = false;
                checkRegexpFilter = false;
                break;
            case Kind::ONLY_REGEXP:
                checkIsPatchable = false;
                checkRegexpFilter = true;
                break;
            case Kind::ALL:
                checkIsPatchable = true;
                checkRegexpFilter = true;
                break;
        }
    }

    bool operator()(const CHIR::Package& package) const
    {
        return matchesRegexpFilter(package.GetName());
    }

    bool operator()(const CHIR::ClassDef* classDef, const std::string& name) const
    {
        return matches(classDef->GetAnnoInfo(), name);
    }

    bool operator()(const CHIR::FuncBase* func, const std::string& name) const
    {
        if (func->TestAttr(CHIR::Attribute::COMPILER_ADD)) {
            return false;
        }
        return matches(func->GetAnnoInfo(), name);
    }

private:
    bool checkIsPatchable = false;
    bool checkRegexpFilter = false;
    std::optional<std::regex> regexpFilter;

    bool matches(const CHIR::AnnoInfo& annoInfo, const std::string& name) const
    {
        if (!checkIsPatchable && !checkRegexpFilter) {
            return true; // nothing to check
        }
        return matchesPatchable(annoInfo) || matchesRegexpFilter(name);
    }

    static bool isPatchable(const CHIR::AnnoInfo& annoInfo)
    {
        if (annoInfo.IsAvailable()) {
            for (const auto& pair : annoInfo.annoPairs) {
                if (pair.annoClassName == "patchable") {
                    return true;
                }
            }
        }
        return false;
    }

    bool matchesPatchable(const CHIR::AnnoInfo& annoInfo) const
    {
        return checkIsPatchable && isPatchable(annoInfo);
    }

    bool matchesRegexpFilter(const std::string& name) const
    {
        return checkRegexpFilter &&
            (regexpFilter.has_value() ? std::regex_match(name, regexpFilter.value()) : false);
    }
};
}
#endif // PLUGIN_CONTEXT_H