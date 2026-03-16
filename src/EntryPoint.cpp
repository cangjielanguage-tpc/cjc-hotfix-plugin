#include "cangjie/CHIR/IR/CHIRBuilder.h"
#include "cangjie/CHIR/IR/Package.h"
#include "cangjie/MetaTransformation/MetaTransform.h"
#include "cangjie/CHIR/Serializer/CHIRSerializer.h"
#include "PatchableFinder.h"
#include "Patcher.h"
#ifdef TEST
#include "PatcherStub.cpp"
#endif
#include "PluginContext.h"
#include "TypeFilterGenerator.h"
#include "toml.h"
#include <iostream>
#include <filesystem>
#include <memory>
#include <regex>
#include <set>

using namespace HotfixPlugin;

class EntryPoint final : public MetaTransform<CHIR::Package> {
public:
    explicit EntryPoint(CHIR::CHIRBuilder& b)
        : builder(b),
          pluginContext(сreatePluginContext(builder)),
          tomlFilter(formFilterByTomlDirective())
    {
    }

    void Run(CHIR::Package& package) override
    {
#ifdef DEBUG
        std::cout << "Running Hotfix plugin for package " << package.GetName() << std::endl;
#endif
        // IR checks are disabled as compiler tries to check abstract methods offsets before Canonicalization
        // and VTable generation. That seems buggy.
        builder.DisableIRCheckerAfterPlugin();

        const auto patcher = Patcher::Create(package.GetName(), builder, pluginContext);
        const auto& patchables = findPatchables(package, tomlFilter);
        for (const auto& patchable : patchables) {
#ifdef DEBUG
            std::cout << "Found patchable method: " << patchable.funcName.GetQualifiedName() << " " << patchable.
                funcName.GetPatchClassName() << std::endl;
#endif
            patcher->patch(patchable);
        }

#ifdef TEST
        const auto patcherStub = PatcherStub::Create(package, builder);
        for (const auto& patchable : patchables) {
            patcherStub->patch(patchable);
        }
#endif

#ifdef DEBUG
        CHIR::CHIRSerializer::Serialize(package, "plugin" + package.GetName() + ".chir", CHIR::ToCHIR::RAW);
        std::cout << "Finished running Hotfix plugin" << std::endl;
#endif
    }

private:
    CHIR::CHIRBuilder& builder;
    std::shared_ptr<PluginContext> pluginContext;
    std::optional<std::regex> tomlFilter;

    static std::shared_ptr<PluginContext> сreatePluginContext(const CHIR::CHIRBuilder& b)
    {
        CHIR::EnumDef* optionDef = nullptr;
        CHIR::FuncBase* optionIsNoneDef = nullptr;
        CHIR::FuncBase* optionGetOrThrowDef = nullptr;
        for (const auto enumDef : b.GetCurPackage()->GetImportedEnums()) {
            if (!CHIR::IsCoreOption(*enumDef)) {
                continue;
            }
            optionDef = enumDef;
            for (const auto method : enumDef->GetMethods()) {
                if (auto identifier = method->GetSrcCodeIdentifier(); identifier == "isNone") {
                    optionIsNoneDef = method;
                } else if (identifier == "getOrThrow" && method->GetNumOfParams() == 1) {
                    // considering return type
                    optionGetOrThrowDef = method;
                }
            }
            break;
        }
        CJC_ASSERT_WITH_MSG(optionDef, "unable to find Option definition");
        CJC_ASSERT_WITH_MSG(optionIsNoneDef, "unable to find Option.isNone method definition");
        CJC_ASSERT_WITH_MSG(optionGetOrThrowDef, "unable to find Option.getOrThrow method definition");

        const auto packageInit = b.GetCurPackage()->GetPackageInitFunc();
        CJC_ASSERT_WITH_MSG(optionDef, "unable to find package init definition");

        return std::shared_ptr{
            std::make_shared<PluginContext>(optionDef, optionIsNoneDef, optionGetOrThrowDef, packageInit)
        };
    }

    static std::optional<std::regex> formFilterByTomlDirective()
    {
        std::vector<std::string> result;

        if (fileExists("cjpm.toml")) {
            const auto table = toml::parse_file("cjpm.toml");
            if (const auto filters = table["hotfix"]["app-patchable"].as_array()) {
                for (auto&& filter : *filters) {
                    if (auto s = filter.value<std::string>(); s.has_value()) {
                        result.push_back(s.value());
                    }
                }
            }
        }
        return formRegex(result);
    }

    static bool fileExists(const std::string& path)
    {
        return std::filesystem::exists(path) && std::filesystem::is_regular_file(path);
    }
};

CHIR_PLUGIN(EntryPoint);