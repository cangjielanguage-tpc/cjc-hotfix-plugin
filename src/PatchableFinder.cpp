#include "PatchableFinder.h"
#include "cangjie/CHIR/IR/Value/Value.h"

#include <string>
#include <vector>
#include <cangjie/Option/OptionTable.h>

using namespace Cangjie;

namespace HotfixPlugin {
void addPatchableFunctions(const PatchableFilter& filter, CHIR::FuncBase* func, std::set<Patchable>& result)
{
    if (const auto funcName = PatchableName{func}; filter(func, funcName.GetQualifiedName())) {
        result.insert(Patchable{
            .funcName = funcName,
            .func = func
        });
    }
}

void addPatchableMethods(const std::vector<CHIR::ClassDef*>& classDefs, const PatchableFilter& filter,
    std::set<Patchable>& result)
{
    for (const auto classDef : classDefs) {
        if (const auto className = PatchableName{classDef}.GetQualifiedName(); filter(classDef, className)) {
            for (const auto method : classDef->GetMethods()) {
                const PatchableFilter methodFilter{PatchableFilter::Kind::NONE, std::nullopt};
                addPatchableFunctions(methodFilter, method, result);
            }
        } else {
            for (const auto method : classDef->GetMethods()) {
                addPatchableFunctions(filter, method, result);
            }
        }
    }
}

std::set<Patchable> findPatchables(const CHIR::Package& package, const std::optional<std::regex>& regexpFilter)
{
    std::set<Patchable> result;
    const auto packageName = package.GetName();
    // if the canonical name of a Cangjie package matches regex, all functions and classes defined in this package are patchable.
    if (const PatchableFilter packageFilter{PatchableFilter::Kind::ONLY_REGEXP, regexpFilter}; packageFilter(package)) {
        const PatchableFilter inPackageFilter{PatchableFilter::Kind::ONLY_REGEXP, std::regex{packageName + "\\..+"}};
        for (const auto func : package.GetGlobalFuncs()) {
            addPatchableFunctions(inPackageFilter, func, result);
        }
        addPatchableMethods(package.GetAllClassDef(), inPackageFilter, result);
    }
    const PatchableFilter funcFilter{PatchableFilter::Kind::ALL, regexpFilter};
    // if the canonical name of a Cangjie function matches regex, it is patchable
    for (const auto func : package.GetGlobalFuncs()) {
        addPatchableFunctions(funcFilter, func, result);
    }
    // if the canonical name of a Cangjie class matches regex, it is patchable, which means all member functions defined in this class are patchable.
    addPatchableMethods(package.GetAllClassDef(), funcFilter, result);
    return result;
}
}