#include "PatchableFinder.h"
#include "cangjie/CHIR/IR/Value/Value.h"

#include <cangjie/Option/OptionTable.h>
#include <string>
#include <vector>

using namespace Cangjie::CHIR;

namespace HotfixPlugin {

bool matchesRegexpFilter(const std::optional<std::regex>& regexpFilter, const std::string& name)
{
#ifdef TEST
    return regexpFilter.has_value() && std::regex_match(name, regexpFilter.value());
#else
    return !regexpFilter.has_value() || std::regex_match(name, regexpFilter.value());
#endif
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
    const auto packageInitFlagGetter =
        packagePatchableName.genPackageInitFlagAccessor(PatchableName::PackageInitAccessorKind::PACKAGE_INIT, false);
    const auto packageInitFlagSetter =
        packagePatchableName.genPackageInitFlagAccessor(PatchableName::PackageInitAccessorKind::PACKAGE_INIT, true);
    const auto packageLiteralInitFlagGetter = packagePatchableName.genPackageInitFlagAccessor(
        PatchableName::PackageInitAccessorKind::PACKAGE_LITERAL_INIT, false);
    const auto packageLiteralInitFlagSetter = packagePatchableName.genPackageInitFlagAccessor(
    PatchableName::PackageInitAccessorKind::PACKAGE_LITERAL_INIT, true);
    for (const auto func : package.GetGlobalFuncsWithBody()) {
#if DEBUG
        std::cout << "func: " << func->GetIdentifierWithoutPrefix() << std::endl;
#endif

        // no need to generate guards for imported functions and the functions from another packages
        // (e.g. belonging to generic instantiated types)
        if (func->IsImportedFunc() || func->GetPackageName() != package.GetName()) {
            continue;
        }

        // no need to generate guards for initializers and anno factories, as ones are reachable from package inits
        if (const auto funcKind = func->GetFuncKind();
            func->TestAttr(Attribute::INITIALIZER) || funcKind == ANNOFACTORY_FUNC ||
            // TODO support ctors for generics structs properly, mut functions
            funcKind == STRUCT_CONSTRUCTOR || funcKind == PRIMAL_STRUCT_CONSTRUCTOR || func->TestAttr(Attribute::MUT)) {
            continue;
        }

        // no need to generate guards for plugin-generated methods
        if (const auto funcIdentifier = func->GetSrcCodeIdentifier();
            funcIdentifier == Cangjie::MAIN_INVOKE ||
            funcIdentifier == guardVarInitializerName ||
            funcIdentifier == packageInitFlagGetter ||
            funcIdentifier == packageInitFlagSetter ||
            funcIdentifier == packageLiteralInitFlagGetter ||
            funcIdentifier == packageLiteralInitFlagSetter) {
            continue;
        }

        // no need to generate guards for plugin-generated package inits' guard methods
        const auto declType = func->GetParentCustomTypeDef();
        if (declType && declType->GetSrcCodeIdentifier() == packageInitGuardClassName) {
            continue;
        }

        const auto funcName = PatchableName(func);
        if (const auto funcQualifiedName = funcName.getQualifiedName();
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
} // namespace HotfixPlugin