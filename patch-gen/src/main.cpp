#include "PatchGen.h"
#include "cangjie/CHIR/Serializer/CHIRDeserializer.h"
#include "cangjie/CHIR/Serializer/CHIRSerializer.h"
#include <fstream>
#include <iostream>
#include <string>

using namespace Cangjie::CHIR;
using namespace PatchGenerator;

std::optional<std::string> genPatch(Package* sourcePackage, const std::string& patchedChirPath)
{
    std::ifstream patchedChirFile(patchedChirPath);
    if (!patchedChirFile.good()) {
        std::cerr << "Error: CHIR file does not exist: " << patchedChirPath << "\n";
        return std::nullopt;
    }

    std::unordered_map<unsigned int, std::string> patchedFileNameMap;
    CHIRContext patchedCctx(&patchedFileNameMap);
    CHIRBuilder patchedCb(patchedCctx);
    if (ToCHIR::Phase phase; !CHIRDeserializer::Deserialize(patchedChirPath, patchedCb, phase)) {
        std::cerr << "Error: Couldn't deserialize: " << patchedChirPath << "\n";
        return std::nullopt;
    }

    for (const auto imported : patchedCctx.GetCurPackage()->GetImportedClasses()) {
        if (imported->GetSrcCodeIdentifier() == "Any") {
            patchedCctx.SetAnyTy(imported->GetType());
            break;
        }
    }

#if DEBUG
    std::cout << "Create patch for package " << patchedCctx.GetCurPackage()->GetName() << std::endl;
#endif
    // TODO rename to patch everywhere
    auto patchFileName = "diff" + patchedChirPath.substr(patchedChirPath.find_last_of('/') + 1);
    try {
        PatchGen(patchFileName, sourcePackage, patchedCctx.GetCurPackage(), &patchedCb).createPatch();
        return patchFileName;
    } catch (const PatchGenException& e) {
        std::cerr << e.what() << std::endl;
        return std::nullopt;
    }
}

int main(int argc, char* argv[])
{
    if (argc != 2 && argc != 3) {
        std::cerr << "Usage:\n"
                  << argv[0] << " <chir_path>\n"
                  << "or\n"
                  << argv[0] << " <source_chir_path> <patched_chir_path>\n";
        return 1;
    }

    std::optional<std::string> patchFileName;
    if (argc == 3) {
        const std::string sourceChirPath = argv[1];
        std::ifstream sourceChirFile(sourceChirPath);
        if (!sourceChirFile.good()) {
            std::cerr << "Error: Source CHIR file does not exist: " << sourceChirPath << "\n";
            return 1;
        }
        std::unordered_map<unsigned int, std::string> sourceFileNameMap;
        CHIRContext sourceCctx(&sourceFileNameMap);
        CHIRBuilder sourceCb(sourceCctx);
        if (ToCHIR::Phase phase; !CHIRDeserializer::Deserialize(sourceChirPath, sourceCb, phase)) {
            std::cerr << "Error: Couldn't deserialize: " << sourceChirPath << "\n";
            return 1;
        }
        patchFileName = genPatch(sourceCctx.GetCurPackage(), argv[2]);
    } else {
        patchFileName = genPatch(nullptr, argv[1]);
    }

    if (!patchFileName.has_value()) {
        return 1;
    }

#if DEBUG
    std::cout << "Patch was successfully generated in " << patchFileName.value() << std::endl;
#endif

#if TEST
    std::unordered_map<unsigned int, std::string> fileNameMap;
    CHIRContext cctx(&fileNameMap);
    CHIRBuilder cb(cctx);
    if (ToCHIR::Phase phase; !CHIRDeserializer::Deserialize(patchFileName.value(), cb, phase)) {
        std::cerr << "Error: Couldn't deserialize: " << patchFileName.value() << "\n";
        return 1;
    }
    const auto package = cb.GetCurPackage();
    std::cout << "global vars" << std::endl;
    for (const auto globalVar : package->GetGlobalVars()) {
        if (!globalVar->IsImportedVar()) {
            std::cout << globalVar->GetIdentifierWithoutPrefix() << std::endl;
        }
    }

    std::cout << "global funcs" << std::endl;
    for (const auto globalFunc : package->GetGlobalFuncsWithBody()) {
        std::cout << globalFunc->GetIdentifierWithoutPrefix() << std::endl;
    }

    std::cout << "types" << std::endl;
    for (const auto customTypeDef : package->GetCurPkgCustomTypeDef()) {
        std::cout << customTypeDef->GetIdentifierWithoutPrefix() << std::endl;
    }
#endif

    return 0;
}
