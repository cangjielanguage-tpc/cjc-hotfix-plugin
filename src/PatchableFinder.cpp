#include "PatchableFinder.h"
#include "cangjie/CHIR/IR/Value/Value.h"

#include <cangjie/Option/OptionTable.h>
#include <string>
#include <vector>

using namespace Cangjie::CHIR;

namespace HotfixPlugin {

bool matchesRegexpFilter(const std::optional<std::regex>& regexpFilter, const std::string& name)
{
    return regexpFilter.has_value() && std::regex_match(name, regexpFilter.value());
}

bool isPatchable(const AnnoInfo& annoInfo)
{
    if (annoInfo.IsAvailable()) {
        for (const auto& annoInstance : annoInfo.GetCustomAnnoInstances()) {
            if (annoInstance.GetAnnoClassName() == "patchable") {
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
    const PatchableName packagePatchableName(&package);
    const auto guardVarInitializerName = packagePatchableName.genGuardVarInitializedName();
    const auto packageInitGuardClassName = packagePatchableName.genGuardClassName(true);
    for (const auto func : package.GetGlobalFuncsWithBody()) {
        const auto funcName = PatchableName(func);
        const auto funcQualifiedName = funcName.getQualifiedName();
#if DEBUG
        std::cout << "func: " << func->GetIdentifierWithoutPrefix() << std::endl;
#endif

        const auto declType = func->GetParentCustomTypeDef();

        const auto funcIdentifier = func->GetSrcCodeIdentifier();
        if (const auto funcKind = func->GetFuncKind(); func->IsImportedFunc() ||
            func->TestAttr(Attribute::INITIALIZER) || funcKind == ANNOFACTORY_FUNC ||

            funcIdentifier == Cangjie::MAIN_INVOKE ||
            funcIdentifier == guardVarInitializerName ||
            funcIdentifier == packagePatchableName.genPackageInitFlagAccessor(PatchableName::PackageInitAccessorKind::PACKAGE_INIT, false) ||
            funcIdentifier == packagePatchableName.genPackageInitFlagAccessor(PatchableName::PackageInitAccessorKind::PACKAGE_INIT, true) ||
            funcIdentifier == packagePatchableName.genPackageInitFlagAccessor(PatchableName::PackageInitAccessorKind::PACKAGE_LITERAL_INIT, false) ||
            funcIdentifier == packagePatchableName.genPackageInitFlagAccessor(PatchableName::PackageInitAccessorKind::PACKAGE_LITERAL_INIT, true) ||

            declType && declType->GetSrcCodeIdentifier() == packageInitGuardClassName) {
            continue;
        }

        if (matches(func->GetAnnoInfo(), regexpFilter, funcQualifiedName) ||
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
} // namespace HotfixPlugin