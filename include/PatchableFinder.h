#ifndef PATCHABLEFINDER_H
#define PATCHABLEFINDER_H

#include "cangjie/CHIR/IR/CHIRBuilder.h"
#include "PluginContext.h"
#include <regex>

using namespace Cangjie;

namespace HotfixPlugin {
std::set<Patchable> findPatchables(const CHIR::Package& package, const std::optional<std::regex>& regexpFilter);
}

#endif //PATCHABLEFINDER_H