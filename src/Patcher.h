#ifndef PATCHER_H
#define PATCHER_H

#include "PatchableFinder.h"

namespace HotfixPlugin {
using namespace Cangjie::CHIR;

class Patcher {
public:
    Patcher(CHIRBuilder& builder, const std::shared_ptr<PluginContext>& pluginCtx);

    void patch(const Patchable& patchable);

    void genPatchClass() const;

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
        GlobalVar* guardVarFlag, Function* guardMethod, Function* guardClassCtor) const;

    void genPackageInitAccessors(GlobalVar* flag, const PatchableName::PackageInitAccessorKind kind) const;

    ClassDef* genGuardClass() const;

    Function* genGuardVarsInitializer() const;

    Function* genGuardMethod(const std::string& name, const Function* patchable) const;

    GlobalVar* genGuardVar(ClassType* guardClassType, bool forPackageInit) const;

    GlobalVar* genGuardVarFlag(const std::string& name) const;

    void genGuardChecks(const Function* patchable, GlobalVar* guardVarFlag, Function* guardMethod);

    void genShouldNotReachHere(Block* block) const;

    Function* createEmptyFunctionWithUnitRetVal(const std::string& name, std::vector<Type*>& paramTypes) const;
};
}
#endif // PATCHER_H