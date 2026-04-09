#include "PluginContext.h"
#include "cangjie/CHIR/Utils/Utils.h"

using namespace Cangjie;
using namespace Cangjie::CHIR;

namespace HotfixPlugin {
class PatcherStub final {
public:
    PatcherStub(Package& package, CHIRBuilder& builder)
        : package(package),
          builder(builder)
    {
        printlnStringFunc = findPrintlnFunc();
        // just a hack for specific test
        if (package.GetName() == "package_init") {
            preparePackageInits();
        }
        guardClass = findGuardClass(PATCHABLE_GUARD_CLASS_NAME);
        patchClass = genPatchClass(guardClass);
        guardVarsInitializer = prepareGuardVarsInitializer();
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

            for (const auto paramType : method->GetFuncType()->GetParamTypes()) {
                builder.CreateParameter(paramType, INVALID_LOCATION, *method);
            }

            const auto newBlock = builder.CreateBlock(newBg);
            newBg->SetEntryBlock(newBlock);

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
        return patchClass;
    }

    Function* prepareGuardVarsInitializer()
    {
        Function* guardVarInitializer = nullptr;
        for (const auto func : package.GetGlobalFuncs()) {
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
        for (const auto globalFunc : package.GetImportedFunctions()) {
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
        const auto guardMethod = findGuardMethod(patchable.funcName.getGuardMethodName());
        genPatchMethodStub(guardMethod);
        updateGuardVarsInitializer(patchable);
    }

    AbstractMethodInfo findGuardMethod(const std::string& name) const
    {
        for (const auto& method : guardClass->GetAbstractMethods()) {
            if (method.methodName == name) {
                return method;
            }
        }
        CJC_ABORT_WITH_MSG("unable to find guard method with name " + name);
    }

    void genPatchMethodStub(const AbstractMethodInfo& baseMethod) const
    {
        const auto baseMethodType = dynamic_cast<FuncType*>(baseMethod.methodTy);
        const auto returnType = baseMethodType->GetReturnType();
        CJC_ASSERT_WITH_MSG(returnType == builder.GetUnitTy(),
            "unable to build stub for the guard method with return type distinct to Unit")

        std::vector paramTypes = baseMethodType->GetParamTypes();
        paramTypes.front() = builder.GetType<RefType>(patchClass->GetType());
        const auto overriddenMethodType = builder.GetType<FuncType>(paramTypes, returnType);

        const auto overriddenMethod = builder.CreateFuncWithBody(INVALID_LOCATION, overriddenMethodType,
            baseMethod.methodName, baseMethod.methodName, baseMethod.methodName,
            package.GetName(), {});
        overriddenMethod->EnableAttr(Attribute::OVERRIDE);
        overriddenMethod->EnableAttr(Attribute::PROTECTED);
        for (const auto paramType : paramTypes) {
            builder.CreateParameter(paramType, INVALID_LOCATION, *overriddenMethod);
        }

        const auto bg = builder.CreateBlockGroup(*overriddenMethod);
        overriddenMethod->InitBody(*bg);
        const auto body = builder.CreateBlock(bg);
        bg->SetEntryBlock(body);

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
    }

private:
    Package& package;
    CHIRBuilder& builder;

    ClassDef* guardClass = nullptr;
    ClassDef* patchClass = nullptr;
    Function* guardVarsInitializer = nullptr;
    Function* printlnStringFunc = nullptr;
};
}