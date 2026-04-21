#ifndef PATCHER_H
#define PATCHER_H

#include "PatchableFinder.h"

namespace HotfixPlugin {
using namespace Cangjie::CHIR;

class Patcher {
public:
    Patcher(CHIRBuilder& builder, const std::shared_ptr<PluginContext>& pluginCtx);

    void patch(const Patchable& patchable);

private:
    CHIRBuilder& builder;
    Package* package;
    std::shared_ptr<PluginContext> pluginCtx;

    // are initialized during first call of patch method
    Function* guardVarsInitializer = nullptr;
    ClassDef* guardClass = nullptr;
    GlobalVar* guardVar = nullptr;

    ClassDef* genPackageInitGuardClass() const;

    void genPackageInitGuardChecks(const Function* patchable, Type* guardClassType, GlobalVar* guardVar,
        GlobalVar* guardVarFlag, const Function* guardMethod, Function* guardClassCtor) const;

    ClassDef* genGuardClass() const;

    Function* genGuardVarsInitializer() const;

    AbstractMethodInfo genGuardMethod(const std::string& name, const Function* patchable) const;

    GlobalVar* genGuardVar(const std::string& name, ClassType* guardClassType, bool needToInstantiate) const;

    GlobalVar* genGuardVarFlag(const std::string& name) const;

    void genGuardChecks(const Function* patchable, GlobalVar* guardVarFlag, const AbstractMethodInfo& guardMethod);

    void genShouldNotReachHere(Block* block) const;
};
}
#endif // PATCHER_H