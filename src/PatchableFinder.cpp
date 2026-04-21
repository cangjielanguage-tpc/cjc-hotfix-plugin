#include "PatchableFinder.h"
#include "cangjie/CHIR/IR/Value/Value.h"

#include <string>
#include <vector>
#include <cangjie/Option/OptionTable.h>

using namespace Cangjie::CHIR;

namespace HotfixPlugin {

bool matchesRegexpFilter(const std::optional<std::regex>& regexpFilter, const std::string& name)
{
    return regexpFilter.has_value() && std::regex_match(name, regexpFilter.value());
}

bool isPatchable(const AnnoInfo& annoInfo)
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

bool matches(const AnnoInfo& annoInfo, const std::optional<std::regex>& regexpFilter, const std::string& name)
{
    return isPatchable(annoInfo) || matchesRegexpFilter(regexpFilter, name);
}

std::set<Patchable> findPatchables(const Package& package, const std::optional<std::regex>& regexpFilter)
{
    std::set<Patchable> result;
    for (const auto func : package.GetGlobalFuncs()) {
        const auto funcName = PatchableName(func);
        const auto funcQualifiedName = funcName.getQualifiedName();
#if DEBUG
        std::cout << "func: " << func->GetIdentifierWithoutPrefix() << std::endl;
#endif

        const auto funcIdentifier = func->GetSrcCodeIdentifier();
        if (const auto funcKind = func->GetFuncKind();
            func->IsImportedFunc() ||

            func->TestAttr(Attribute::INITIALIZER) ||

            funcKind == ANNOFACTORY_FUNC ||

            funcIdentifier == Cangjie::MAIN_INVOKE ||
            funcIdentifier == PATCHABLE_GUARD_VARS_INITIALIZER ||
            funcIdentifier == PACKAGE_INIT_GUARD_METHOD_NAME ||
            funcIdentifier == PACKAGE_LITERAL_INIT_GUARD_METHOD_NAME) {

            continue;
        }

        if (const auto declType = func->GetParentCustomTypeDef();
            matches(func->GetAnnoInfo(), regexpFilter, funcQualifiedName) ||
            (declType && matches(declType->GetAnnoInfo(), regexpFilter, PatchableName(declType).getQualifiedName()))) {
            const auto [_, added] = result.insert(Patchable{
                .funcName = funcName,
                .func = func
            });
#if DEBUG
            std::cout << "added " << added << std::endl;
#endif
        }
    }
    return result;
}
}