#include "PatchableFinder.h"
#include "Patcher.h"
#include "cangjie/CHIR/IR/CHIRBuilder.h"
#include "cangjie/CHIR/IR/Package.h"
#include "cangjie/CHIR/Serializer/CHIRSerializer.h"
#include "cangjie/MetaTransformation/MetaTransform.h"
#ifdef STUB_TEST
#include "PatcherStub.h"
#endif
#include "PluginContext.h"
#include "TypeFilterGenerator.h"
#include "toml.h"
#include <filesystem>
#include <iostream>
#include <memory>
#include <regex>
#include <set>

using namespace HotfixPlugin;
using namespace Cangjie;
using namespace Cangjie::CHIR;

class EntryPoint final : public MetaTransform<Package> {
public:
    explicit EntryPoint(CHIRBuilder& b)
        : builder(b), pluginContext(createPluginContext(builder)), tomlFilter(formFilterByTomlDirective())
    {
    }

    void Run(Package& package) override
    {
#ifdef DEBUG
        std::cout << "Running Hotfix plugin for package " << package.GetName() << std::endl;
#endif
        // IR checks are disabled in release mode as compiler tries to check abstract methods offsets before
        // Canonicalization and VTable generation. That seems buggy.
        builder.DisableIRCheckerAfterPlugin();

        auto patcher = Patcher(builder, pluginContext);
        const auto& patchables = findPatchables(package, tomlFilter);
        for (const auto& patchable : patchables) {
#ifdef DEBUG
            std::cout << std::endl
                      << std::endl
                      << std::endl
                      << "Found patchable method: " << patchable.funcName.getQualifiedName() << "("
                      << patchable.func->GetIdentifierWithoutPrefix() << ")" << std::endl;
#endif
            patcher.patch(patchable);
        }

#ifdef STUB_TEST
        const auto patcherStub = PatcherStub(package, builder);
        for (const auto& patchable : patchables) {
            patcherStub.patch(patchable);
        }
#endif

#ifdef DEBUG
        CHIRSerializer::Serialize(package, "plugin" + package.GetName() + ".chir", ToCHIR::RAW);
        std::cout << "Finished running Hotfix plugin" << std::endl << std::endl << std::endl;
#endif
    }

private:
    CHIRBuilder& builder;
    std::shared_ptr<PluginContext> pluginContext;
    std::optional<std::regex> tomlFilter;

    static std::shared_ptr<PluginContext> createPluginContext(CHIRBuilder& b)
    {
        const auto package = b.GetCurPackage();

        EnumDef* optionDef = nullptr;
        for (const auto enumDef : package->GetImportedEnums()) {
            if (IsCoreOption(*enumDef)) {
                optionDef = enumDef;
                break;
            }
        }
        CJC_ASSERT_WITH_MSG(optionDef, "unable to find Option definition");

        ClassDef* exceptionDef = nullptr;
        Function* exceptionInitDef = nullptr;
        for (const auto& importedClass : package->GetImportedClasses()) {
            if (importedClass->GetSrcCodeIdentifier() == "Exception") {
                exceptionDef = importedClass;
                for (const auto& method : importedClass->GetMethods()) {
                    if (method->IsConstructor() && method->GetNumOfParams() == 2 &&
                        method->GetParam(1)->GetType() == b.GetStringTy()) {
                        exceptionInitDef = method;
                        break;
                    }
                }
                break;
            }
        }
        CJC_ASSERT_WITH_MSG(exceptionDef, "unable to find Exception definition");
        CJC_ASSERT_WITH_MSG(exceptionInitDef, "unable to find Exception.<init>(String) definition");

        return std::shared_ptr{std::make_shared<PluginContext>(optionDef, exceptionDef, exceptionInitDef)};
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
