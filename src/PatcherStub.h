#ifndef PATCHERSTUB_H
#define PATCHERSTUB_H
#include "PluginContext.h"
#include "cangjie/CHIR/Utils/Utils.h"
#include "cangjie/CHIR/Serializer/CHIRDeserializer.h"

#include <filesystem>
#include <fstream>

using namespace Cangjie;
using namespace Cangjie::CHIR;

namespace HotfixPlugin {
class PatcherStub final {
public:
    PatcherStub(Package& package, CHIRBuilder& builder)
        : package(package),
          builder(builder)
    {
#if DEBUG
        std::cout << std::endl << std::endl << std::endl;
#endif

        printlnStringFunc = findPrintlnFunc();
        // just a hack for specific test
        if (package.GetName() == "package_init") {
            preparePackageInits();
        }
        guardClass = findGuardClass(PATCHABLE_GUARD_CLASS_NAME);
        patchClass = genPatchClass(guardClass);
        guardVarsInitializer = prepareGuardVarsInitializer();

        const auto testFileName = builder.GetChirContext().GetSourceFileName(1);
        if (!testFileName.rfind(".cj")) {
            return;
        }

        if (std::ifstream stubMapFile(testFileName + ".stub.map"); stubMapFile.is_open()) {
#if DEBUG
            std::cout << "found patched stub map file" << std::endl;
#endif
            std::string line;
            while (std::getline(stubMapFile, line)) {
                if (const auto delimiterPos = line.find('='); delimiterPos != std::string::npos) {
                    const auto key = line.substr(0, delimiterPos);
                    const auto value = line.substr(delimiterPos + 1);
                    stubsMap[key] = value;
                }
            }
            stubMapFile.close();
        }

        if (stubsMap.empty()) {
#if DEBUG
            std::cout << "stub map file is empty" << std::endl;
#endif
            return;
        }

        for (auto& [_, stubName] : stubsMap) {
            for (auto func : package.GetGlobalFuncsWithBody()) {
                if (const auto name = func->GetIdentifierWithoutPrefix(); name == stubName) {
                    stubCode.emplace(name, func->GetBody());
                }
            }
        }
    }

    void preparePackageInits() const
    {
        const auto guardClass = findGuardClass(PACKAGE_INIT_GUARD_CLASS_NAME);
        for (const auto& method : guardClass->GetMethods()) {
            if (method->IsConstructor()) {
                continue;
            }

            const auto newBg = builder.CreateBlockGroup(*method);
            method->ReplaceBody(*newBg);

            const auto newBlock = builder.CreateBlock(newBg);
            newBg->SetEntryBlock(newBlock);

            const auto retVal = CHIR::CreateAndAppendExpression<Allocate>(builder, INVALID_LOCATION,
            builder.GetType<RefType>(builder.GetUnitTy()), builder.GetUnitTy(), newBlock)->GetResult();
            method->SetReturnValue(*retVal);

            const auto terminator = builder.CreateTerminator<Exit>(newBlock);
            newBlock->AppendExpression(terminator);

            const auto helloLiteral = builder.CreateConstantExpression<StringLiteral>(builder.GetStringTy(), newBlock,
                "Hello from package init stub");
            helloLiteral->MoveBefore(terminator);

            const auto applyPrintln = builder.CreateExpression<Apply>(builder.GetUnitTy(), printlnStringFunc,
                FuncCallContext{
                    .args = {helloLiteral->GetResult()},
                    .instTypeArgs = {},
                    .thisType = nullptr,
                }, newBlock);
            applyPrintln->MoveAfter(helloLiteral);
        }

        GlobalVar* packageInitGuardVarFlag = nullptr;
        for (const auto& globalVar : package.GetGlobalVars()) {
            if (globalVar->GetSrcCodeIdentifier() == PACKAGE_INIT_GUARD_VAR_FLAG_NAME) {
                packageInitGuardVarFlag = globalVar;
                break;
            }
        }
        CJC_ASSERT_WITH_MSG(packageInitGuardVarFlag, "unable to find package init guard var flag definition");

        const auto packageInitBody = package.GetPackageInitFunc()->GetBody();
        const auto entryBlock = packageInitBody->GetEntryBlock();
        const auto firstExpr = entryBlock->GetExpressionByIdx(0);

        const auto trueExpr = builder.CreateConstantExpression<BoolLiteral>(builder.GetBoolTy(), entryBlock, true);
        trueExpr->MoveBefore(firstExpr);
        const auto storePackageInitGuardVarFlag = builder.CreateExpression<Store>(builder.GetUnitTy(),
            trueExpr->GetResult(), packageInitGuardVarFlag, entryBlock);
        storePackageInitGuardVarFlag->MoveAfter(trueExpr);
    }

    ClassDef* findGuardClass(const std::string_view name) const
    {
        ClassDef* guardClass = nullptr;
        for (const auto classDef : package.GetClasses()) {
            if (classDef->GetSrcCodeIdentifier() == name) {
                guardClass = classDef;
                break;
            }
        }
        CJC_ASSERT_WITH_MSG(guardClass, "unable to find guard class");
        return guardClass;
    }

    ClassDef* genPatchClass(const ClassDef* guardClass) const
    {
        const auto patchClassName = guardClass->GetSrcCodeIdentifier() + "Impl";
        const auto patchClass = builder.CreateClass(INVALID_LOCATION, patchClassName, patchClassName,
            package.GetName(), true, false);
        const auto patchClassType = builder.GetType<ClassType>(patchClass);
        patchClass->SetType(*patchClassType);
        patchClass->SetSuperClassTy(*guardClass->GetType());
        patchClass->EnableAttr(Attribute::INTERNAL);
        patchClass->EnableAttr(Attribute::COMPILER_ADD);
        patchClass->Set<LinkTypeInfo>(Linkage::EXTERNAL);
        return patchClass;
    }

    Function* prepareGuardVarsInitializer()
    {
        Function* guardVarInitializer = nullptr;
        for (const auto func : package.GetGlobalFuncsWithBody()) {
            if (func->GetSrcCodeIdentifier() == PATCHABLE_GUARD_VARS_INITIALIZER) {
                guardVarInitializer = func;
                break;
            }
        }
        CJC_ASSERT_WITH_MSG(guardVarInitializer, "unable to find guard vars initializer definition");

        const auto newBg = builder.CreateBlockGroup(*guardVarInitializer);
        guardVarInitializer->ReplaceBody(*newBg);
        guardVarsInitializer = guardVarInitializer;

        const auto newBlock = builder.CreateBlock(newBg);
        newBg->SetEntryBlock(newBlock);

        const auto retVal = CHIR::CreateAndAppendExpression<Allocate>(builder, INVALID_LOCATION,
            builder.GetType<RefType>(builder.GetUnitTy()), builder.GetUnitTy(), newBlock)->GetResult();
        guardVarInitializer->SetReturnValue(*retVal);

        GlobalVar* guardVar = nullptr;
        for (const auto& globalVar : package.GetGlobalVars()) {
            if (globalVar->GetSrcCodeIdentifier() == PATCHABLE_GUARD_VAR_NAME) {
                guardVar = globalVar;
            }
        }
        CJC_ASSERT_WITH_MSG(guardVar, "unable to find guard var definition");

        const auto patchClassType = patchClass->GetType();
        const auto alloc = CHIR::CreateAndAppendExpression<Allocate>(builder, builder.GetType<RefType>(patchClassType),
            patchClassType, newBlock);

        const auto typeCast = CHIR::CreateAndAppendExpression<TypeCast>(builder,
            builder.GetType<RefType>(guardClass->GetType()), alloc->GetResult(), newBlock);

        const auto falseExpr = builder.CreateConstantExpression<BoolLiteral>(builder.GetBoolTy(), newBlock, false);
        falseExpr->MoveAfter(typeCast);

        const auto guardVarType = dynamic_cast<RefType*>(guardVar->GetType())->GetBaseType();
        const auto tuple = CHIR::CreateAndAppendExpression<Tuple>(builder, guardVarType,
            std::vector<Value*>{falseExpr->GetResult(), typeCast->GetResult()}, newBlock);

        CHIR::CreateAndAppendExpression<Store>(builder, builder.GetUnitTy(),
            tuple->GetResult(), guardVar, newBlock);

        CHIR::CreateAndAppendTerminator<Exit>(builder, newBlock);

        return guardVarInitializer;
    }

    Function* findPrintlnFunc() const
    {
        Function* printlnFunc = nullptr;
        for (const auto globalFunc : package.GetGlobalFuncsWithoutBody()) {
            if (globalFunc->GetSrcCodeIdentifier() != "println") {
                continue;
            }
            if (const auto globalFuncType = dynamic_cast<FuncType*>(globalFunc->GetType());
                globalFuncType->GetNumOfParams() == 1 && globalFuncType->GetParamType(0) == builder.GetStringTy()) {
                printlnFunc = globalFunc;
                break;
            }
        }
        CJC_ASSERT_WITH_MSG(printlnFunc, "unable to find println(String) function");
        return printlnFunc;
    }

    void patch(const Patchable& patchable) const
    {
        const auto guardMethod = findGuardMethod(
            guardClass->GetSrcCodeIdentifier() + patchable.funcName.getGuardMethodName());
        genPatchMethodStub(guardMethod);
        updateGuardVarsInitializer(patchable);
    }

    Function* findGuardMethod(const std::string& name) const
    {
        for (const auto& method : guardClass->GetMethods()) {
            if (method->GetSrcCodeIdentifier() == name) {
                return method;
            }
        }
        CJC_ABORT_WITH_MSG("unable to find guard method with name " + name);
    }

    void genPatchMethodStub(const Function* baseMethod) const
    {
        const auto baseMethodType = baseMethod->GetFuncType();
        const auto returnType = baseMethodType->GetReturnType();

        std::vector paramTypes = baseMethodType->GetParamTypes();
        paramTypes.front() = builder.GetType<RefType>(patchClass->GetType());
        const auto overriddenMethodType = builder.GetType<FuncType>(paramTypes, returnType);

        const auto methodName = baseMethod->GetSrcCodeIdentifier();
        const auto overriddenMethod = builder.CreateFunction(overriddenMethodType,
            methodName, methodName, methodName,
            package.GetName(), {});
        overriddenMethod->EnableAttr(Attribute::OVERRIDE);
        overriddenMethod->EnableAttr(Attribute::PROTECTED);
        overriddenMethod->Set<LinkTypeInfo>(Linkage::EXTERNAL);
        for (const auto paramType : paramTypes) {
            builder.CreateParameter(paramType, INVALID_LOCATION, *overriddenMethod);
        }

        const auto bg = builder.CreateBlockGroup(*overriddenMethod);
        overriddenMethod->InitBody(*bg);
        const auto body = builder.CreateBlock(bg);
        bg->SetEntryBlock(body);

        if (const auto patchStub = stubsMap.find(methodName); patchStub == stubsMap.end()) {
#if DEBUG
            std::cout << "stub method for " << methodName << " was not found in stub map" << std::endl;
#endif

            /*
               generate default stub for method with Unit return type:

               func foo(): Unit {
                 println("Hello from stub")
               }
             */
            CJC_ASSERT_WITH_MSG(returnType == builder.GetUnitTy(),
                "unable to build stub for the guard method with return type distinct to Unit");

            const auto retVal = builder.CreateExpression<Allocate>(INVALID_LOCATION,
                builder.GetType<RefType>(builder.GetUnitTy()), builder.GetUnitTy(),
                body)->GetResult();
            overriddenMethod->SetReturnValue(*retVal);

            const auto terminator = builder.CreateTerminator<Exit>(body);
            body->AppendExpression(terminator);

            const auto helloLiteral = builder.CreateConstantExpression<StringLiteral>(builder.GetStringTy(), body,
                "Hello from stub");
            helloLiteral->MoveBefore(terminator);

            const auto applyPrintln = builder.CreateExpression<Apply>(builder.GetUnitTy(), printlnStringFunc,
                FuncCallContext{
                    .args = {helloLiteral->GetResult()},
                    .instTypeArgs = {},
                    .thisType = nullptr,
                }, body);
            applyPrintln->MoveAfter(helloLiteral);

        } else {
            // generate stub according to stub.map file
#if DEBUG
            std::cout << "search stub code for " << methodName << ": " << patchStub->second << std::endl;
#endif
            if (const auto patchStubCode = stubCode.find(patchStub->second); patchStubCode == stubCode.end()) {
#if DEBUG
                std::cout << "candidates: " << std::endl;
                for (const auto func : package.GetGlobalFuncsWithBody()) {
                    if (func->GetPackageName() == package.GetName()) {
                        std::cout << func->GetIdentifierWithoutPrefix() << std::endl;
                    }
                }
#endif
                CJC_ABORT_WITH_MSG("not found");
            } else {
#if DEBUG
                std::cout << "found:" << std::endl;
#endif

                const auto stubMethod = patchStubCode->second->GetOwnerFunc();
#if DEBUG
                std::cout << stubMethod->ToString(0) << std::endl;
#endif

                const auto currRetVal = stubMethod->GetReturnValue();
                int retValIdx = -1;
                int exprIdx = -1;
                for (const auto expr : stubMethod->GetEntryBlock()->GetNonTerminatorExpressions()) {
                    exprIdx++;
                    if (expr->GetResult() == currRetVal) {
                        retValIdx = exprIdx;
                        break;
                    }
                }

                overriddenMethod->ReplaceBody(*patchStubCode->second);

                // as a function has a new body, we need to fix param refs
                const auto parameters = GetFuncParams(*overriddenMethod->GetBody());
                const auto fixParamsRef = [&](Expression& e) {
                    for (const auto operand : e.GetOperands()) {
                        if (operand->IsParameter()) {
                            for (const auto param : parameters) {
                                if (param->GetIdentifier() == operand->GetIdentifier()) {
                                    e.ReplaceOperand(operand, param);
                                    break;
                                }
                            }
                        }
                    }
                    return VisitResult::CONTINUE;
                };
                Visitor::Visit(*overriddenMethod, [](Expression&) {
                    return VisitResult::CONTINUE;
                }, fixParamsRef);

                if (const auto declClass = stubMethod->GetParentCustomTypeDef()) {
                    std::vector<Function*> newMethods;
                    for (auto method : declClass->GetMethods()) {
                        if (method->GetIdentifierWithoutPrefix() != stubMethod->GetIdentifierWithoutPrefix()) {
                            newMethods.emplace_back(method);
                        }
                    }
                    declClass->SetMethods(newMethods);
                }
                std::vector<Function*> newGlobalFuncs;
                for (const auto& func : package.GetGlobalFunctions()) {
                    if (func->GetIdentifierWithoutPrefix() != stubMethod->GetIdentifierWithoutPrefix()) {
                        newGlobalFuncs.emplace_back(func);
                    }
                }
                package.SetAllGlobalFuncs(std::move(newGlobalFuncs));

                // regenerate expressions in block to fix identifiers
                for (const auto block : overriddenMethod->GetBody()->GetBlocks()) {
                    for (const auto expression : block->GetNonTerminatorExpressions()) {
                        const auto clonedExpr = expression->Clone(builder, *overriddenMethod->GetEntryBlock());
                        expression->ReplaceWith(*clonedExpr);
                    }
                }

                const auto newRetVal = overriddenMethod->GetEntryBlock()->GetExpressions().at(retValIdx)->GetResult();
                overriddenMethod->SetReturnValue(*newRetVal);
            }
        }
#if DEBUG
        std::cout << "patch after replacement:" << std::endl;
        std::cout << overriddenMethod->ToString(0) << std::endl;
#endif
        patchClass->AddMethod(overriddenMethod);
    }

    void updateGuardVarsInitializer(const Patchable& patchable) const
    {
        const auto guardVarFlagName = patchable.funcName.getGuardVarFlagName();

        Value* guardVarFlag = nullptr;
        for (const auto var : package.GetGlobalVars()) {
            if (var->GetSrcCodeIdentifier() == guardVarFlagName) {
                guardVarFlag = var;
                break;
            }
        }
        CJC_ASSERT_WITH_MSG(guardVarFlag, "unable to find guard var flag definition");

        const auto guardVarsInitializerBody = guardVarsInitializer->GetEntryBlock();
        const auto guardVarsInitializerTerminator = guardVarsInitializerBody->GetTerminator();

        const auto trueExpr = builder.CreateConstantExpression<BoolLiteral>(builder.GetBoolTy(),
            guardVarsInitializerBody, true);
        trueExpr->MoveBefore(guardVarsInitializerTerminator);

        const auto storeToGuardVarFlag = builder.CreateExpression<Store>(builder.GetUnitTy(), trueExpr->GetResult(),
            guardVarFlag, guardVarsInitializerBody);
        storeToGuardVarFlag->MoveAfter(trueExpr);

#if DEBUG
        std::cout << "guardVarsInit after update:" << std::endl;
        std::cout << guardVarsInitializerBody->ToString(0) << std::endl;
#endif
    }

private:
    Package& package;
    CHIRBuilder& builder;

    ClassDef* guardClass = nullptr;
    ClassDef* patchClass = nullptr;
    Function* guardVarsInitializer = nullptr;
    Function* printlnStringFunc = nullptr;

    std::unordered_map<std::string, std::string> stubsMap;
    std::unordered_map<std::string, BlockGroup*> stubCode;
};
}


#endif // PATCHERSTUB_H