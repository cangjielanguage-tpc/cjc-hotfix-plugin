#ifndef PATCHER_H
#define PATCHER_H

#include "PatchableFinder.h"

namespace HotfixPlugin {
using namespace Cangjie;

const std::string PATCHABLE_INIT_FUNC = "patchableInitFunc";

class Patcher {
public:
    Patcher(const Patcher&) = delete;

    Patcher& operator=(const Patcher&) = delete;

    static std::unique_ptr<Patcher> Create(const std::string& packageName,
        CHIR::CHIRBuilder& builder,
        const std::shared_ptr<PluginContext>& pluginCtx);

    void patch(const Patchable& patchable) const;

private:
    CHIR::CHIRBuilder& builder;
    std::shared_ptr<PluginContext> pluginCtx;
    CHIR::Func* fieldsInitializer = nullptr;

    Patcher(CHIR::CHIRBuilder& builder, const std::shared_ptr<PluginContext>& pluginCtx,
        CHIR::Func* fieldsInitializer)
        : builder(builder),
          pluginCtx(pluginCtx),
          fieldsInitializer(fieldsInitializer)
    {

    }

    CHIR::ClassType* createGuardClass(const Patchable& patchable) const;

    CHIR::GlobalVar* createField(const std::string& fieldName, CHIR::EnumType* fieldType,
        const std::string& packageName) const;

    void addFieldCheck(CHIR::GlobalVar* field, CHIR::Type* fieldType,
        const CHIR::BlockGroup* funcBlockGroup, const CHIR::AbstractMethodInfo& method) const;
};
}
#endif // PATCHER_H