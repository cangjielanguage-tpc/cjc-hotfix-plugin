#include "PluginContext.h"
#include "Patcher.h"

using namespace Cangjie;

namespace HotfixPlugin {
class PatcherStub final {
public:
    PatcherStub(CHIR::Package& package, CHIR::CHIRBuilder& builder,
        CHIR::Func* patchableInitFunc, CHIR::ImportedValue* printlnStringFunc)
        : package(package),
          builder(builder),
          patchableInitFunc(patchableInitFunc),
          printlnStringFunc(printlnStringFunc)
    {
    }

    static std::unique_ptr<PatcherStub> Create(CHIR::Package& package, CHIR::CHIRBuilder& builder)
    {
        CHIR::Func* patchableInitFunc = nullptr;
        for (const auto func : package.GetGlobalFuncs()) {
            if (func->GetSrcCodeIdentifier() == PATCHABLE_INIT_FUNC) {
                patchableInitFunc = func;
                break;
            }
        }
        CJC_ASSERT_WITH_MSG(patchableInitFunc, "unable to find patchable init func definition")

        const auto newBg = builder.CreateBlockGroup(*patchableInitFunc);
        patchableInitFunc->ReplaceBody(*newBg);

        const auto newBlock = builder.CreateBlock(newBg);
        newBg->SetEntryBlock(newBlock);

        const auto newTerminator = builder.CreateTerminator<CHIR::Exit>(newBlock);
        newBlock->AppendExpression(newTerminator);

        CHIR::ImportedValue* printlnStringFunc;
        for (const auto globalFunc : package.GetImportedVarAndFuncs()) {
            if (globalFunc->GetSrcCodeIdentifier() != "println") {
                continue;
            }
            if (const auto globalFuncType = dynamic_cast<CHIR::FuncType*>(globalFunc->GetType());
                globalFuncType->GetNumOfParams() == 1 && globalFuncType->GetParamType(0) == builder.GetStringTy()) {
                printlnStringFunc = globalFunc;
                break;
            }
        }

        return std::make_unique<PatcherStub>(package, builder, patchableInitFunc, printlnStringFunc);
    }

    CHIR::ClassDef* findGuardClass(const std::string_view name) const
    {
        CHIR::ClassDef* guardClass = nullptr;
        for (const auto classDef : package.GetClasses()) {
            if (classDef->GetSrcCodeIdentifier() == name) {
                guardClass = classDef;
                break;
            }
        }
        CJC_ASSERT_WITH_MSG(guardClass, "unable to find guard class");
        return guardClass;
    }

    CHIR::ClassDef* createGuardClassImpl(const CHIR::ClassDef* guardClass) const
    {
        const auto guardClassImplName = guardClass->GetSrcCodeIdentifier() + "Impl";
        const auto guardClassImpl = builder.CreateClass(CHIR::INVALID_LOCATION, guardClassImplName, guardClassImplName,
            package.GetName(), true, false);
        const auto guardClassTypeImpl = builder.GetType<CHIR::ClassType>(guardClassImpl);
        guardClassImpl->SetType(*guardClassTypeImpl);
        guardClassImpl->SetSuperClassTy(*guardClass->GetType());
        guardClassImpl->EnableAttr(CHIR::Attribute::INTERNAL);
        guardClassImpl->EnableAttr(CHIR::Attribute::COMPILER_ADD);
        guardClassImpl->Set<CHIR::LinkTypeInfo>(Linkage::EXTERNAL);
        return guardClassImpl;
    }

    void overrideMethodWithStub(const CHIR::AbstractMethodInfo& baseMethod, CHIR::ClassDef* guardClassImpl) const
    {
        const auto baseMethodType = dynamic_cast<CHIR::FuncType*>(baseMethod.methodTy);

        std::vector paramTypes = baseMethodType->GetParamTypes();
        paramTypes.front() = builder.GetType<CHIR::RefType>(guardClassImpl->GetType());
        const auto overriddenMethodType = builder.GetType<CHIR::FuncType>(paramTypes, baseMethodType->GetReturnType());

        const auto overriddenMethod = builder.CreateFunc(CHIR::INVALID_LOCATION, overriddenMethodType,
            baseMethod.methodName, baseMethod.methodName, baseMethod.methodName,
            package.GetName(), {});
        overriddenMethod->EnableAttr(CHIR::Attribute::OVERRIDE);
        overriddenMethod->EnableAttr(CHIR::Attribute::PROTECTED);
        for (const auto paramType : paramTypes) {
            builder.CreateParameter(paramType, CHIR::INVALID_LOCATION, *overriddenMethod);
        }

        const auto bg = builder.CreateBlockGroup(*overriddenMethod);
        overriddenMethod->InitBody(*bg);
        const auto body = builder.CreateBlock(bg);
        bg->SetEntryBlock(body);

        const auto retVal = builder.CreateExpression<CHIR::Allocate>(CHIR::INVALID_LOCATION,
            builder.GetType<CHIR::RefType>(builder.GetUnitTy()), builder.GetUnitTy(),
            body)->GetResult();
        overriddenMethod->SetReturnValue(*retVal);

        const auto terminator = builder.CreateTerminator<CHIR::Exit>(body);
        body->AppendExpression(terminator);

        const auto helloLiteral = builder.CreateConstantExpression<CHIR::StringLiteral>(builder.GetStringTy(), body,
            "Hello from stub");
        helloLiteral->MoveBefore(terminator);

        const auto applyPrintln = builder.CreateExpression<CHIR::Apply>(builder.GetUnitTy(), printlnStringFunc,
            CHIR::FuncCallContext{
                .args = {helloLiteral->GetResult()},
                .instTypeArgs = {},
                .thisType = nullptr,
            }, body);
        applyPrintln->MoveAfter(helloLiteral);

        guardClassImpl->AddMethod(overriddenMethod);
    }

    void updatePatchableInitFunc(const Patchable& patchable, const CHIR::ClassDef* guardClass,
        const CHIR::ClassDef* guardClassImpl) const
    {
        const auto patchableInitFuncBody = patchableInitFunc->GetEntryBlock();
        const auto patchableInitFuncTerminator = patchableInitFuncBody->GetTerminator();

        const auto guardClassImplType = guardClassImpl->GetType();
        const auto alloc = builder.CreateExpression<CHIR::Allocate>(builder.GetType<CHIR::RefType>(guardClassImplType),
            guardClassImplType, patchableInitFuncBody);
        alloc->MoveBefore(patchableInitFuncTerminator);

        const auto typeCast = builder.CreateExpression<CHIR::TypeCast>(
            builder.GetType<CHIR::RefType>(guardClass->GetType()),
            alloc->GetResult(), patchableInitFuncBody);
        typeCast->MoveAfter(alloc);

        const auto falseExpr = builder.CreateConstantExpression<CHIR::BoolLiteral>(builder.GetBoolTy(),
            patchableInitFuncBody, false);
        falseExpr->MoveAfter(typeCast);

        const auto patchFieldName = patchable.funcName.GetPatchFieldName();
        for (auto var : package.GetGlobalVars()) {
            if (var->GetSrcCodeIdentifier() == patchFieldName) {
                const auto fieldType = dynamic_cast<CHIR::RefType*>(var->GetType())->GetBaseType();
                const auto tuple = builder.CreateExpression<CHIR::Tuple>(fieldType,
                    std::vector<CHIR::Value*>{falseExpr->GetResult(), typeCast->GetResult()}, patchableInitFuncBody);
                tuple->MoveAfter(falseExpr);

                const auto storeToField = builder.CreateExpression<CHIR::Store>(builder.GetUnitTy(), tuple->GetResult(),
                    var, patchableInitFuncBody);
                storeToField->MoveAfter(tuple);

                break;
            }
        }
    }

    void patch(const Patchable& patchable) const
    {
        const auto guardClassName = patchable.funcName.GetPatchClassName();
        const auto guardClass = findGuardClass(guardClassName);
        const auto guardClassImpl = createGuardClassImpl(guardClass);
        overrideMethodWithStub(guardClass->GetAbstractMethods().front(), guardClassImpl);
        updatePatchableInitFunc(patchable, guardClass, guardClassImpl);
    }

private:
    CHIR::Package& package;
    CHIR::CHIRBuilder& builder;
    CHIR::Func* const patchableInitFunc;
    CHIR::ImportedValue* const printlnStringFunc;
};
}