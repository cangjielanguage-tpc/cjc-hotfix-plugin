#include "Patcher.h"
#include "cangjie/CHIR/Utils/Utils.h"
#include <iostream>
#include <memory>

using namespace Cangjie;
using namespace HotfixPlugin;

std::unique_ptr<Patcher> Patcher::Create(const std::string& packageName, CHIR::CHIRBuilder& builder,
    const std::shared_ptr<PluginContext>& pluginCtx)
{
    std::vector<CHIR::Type*> paramTypes;
    const auto funcType = builder.GetType<CHIR::FuncType>(paramTypes, builder.GetUnitTy());
    // TODO mangle?
    const auto fieldsInitializer = builder.CreateFunc(CHIR::INVALID_LOCATION, funcType, PATCHABLE_INIT_FUNC,
        PATCHABLE_INIT_FUNC, PATCHABLE_INIT_FUNC, packageName, {});

    const auto bg = builder.CreateBlockGroup(*fieldsInitializer);
    fieldsInitializer->InitBody(*bg);
    const auto entryBlock = builder.CreateBlock(bg);
    bg->SetEntryBlock(entryBlock);

    const auto retVal = builder.CreateExpression<CHIR::Allocate>(
        CHIR::INVALID_LOCATION, builder.GetType<CHIR::RefType>(builder.GetUnitTy()), builder.GetUnitTy(),
        entryBlock)->GetResult();
    fieldsInitializer->SetReturnValue(*retVal);

    const auto terminator = builder.CreateTerminator<CHIR::Exit>(entryBlock);
    entryBlock->AppendExpression(terminator);

    const auto packageInitBody = pluginCtx->packageInit->GetBody();
    for (const auto block : packageInitBody->GetBlocks()) {
        if (block->TestAttr(CHIR::Attribute::INITIALIZER)) {
            const auto fieldsInitializerCall = builder.CreateExpression<CHIR::Apply>(
                builder.GetUnitTy(), fieldsInitializer, CHIR::FuncCallContext{
                    .args = {},
                    .instTypeArgs = {},
                    .thisType = nullptr,
                }, block);
            fieldsInitializerCall->MoveBefore(block->GetTerminator());
            break;
        }
    }

    return std::unique_ptr<Patcher>(new Patcher(builder, pluginCtx, fieldsInitializer));
}

CHIR::ClassType* Patcher::createGuardClass(const Patchable& patchable) const
{
    const auto& funcName = patchable.funcName;
#ifdef DEBUG
    std::cout << "Create guard class" << std::endl;
#endif
    const auto guardClassName = funcName.GetPatchClassName(); // TODO mangle?
    const auto guardClass = builder.CreateClass(CHIR::INVALID_LOCATION, guardClassName, guardClassName,
        patchable.func->GetPackageName(), true, false);
    const auto guardClassType = builder.GetType<CHIR::ClassType>(guardClass);
    guardClass->SetType(*guardClassType);
    guardClass->SetSuperClassTy(*builder.GetObjectTy());
    guardClass->EnableAttr(CHIR::Attribute::ABSTRACT);
    guardClass->EnableAttr(CHIR::Attribute::INTERNAL);
    guardClass->EnableAttr(CHIR::Attribute::COMPILER_ADD);
    guardClass->Set<CHIR::LinkTypeInfo>(Linkage::EXTERNAL);

#ifdef DEBUG
    std::cout << "@patchable func:" << std::endl;
    std::cout << patchable.func->ToString() << std::endl;
#endif

    const auto funcType = patchable.func->GetFuncType();
    const auto paramTypes = funcType->GetParamTypes();
    std::vector<CHIR::AbstractMethodParam> patchableParams;
    std::vector<CHIR::Type*> patchableParamTypes;
    if (!patchable.func->TestAttr(CHIR::Attribute::STATIC)) {
        patchableParamTypes.emplace_back(builder.GetType<CHIR::RefType>(guardClassType));
    }
    for (size_t i = 0; i < paramTypes.size(); ++i) {
        patchableParams.emplace_back(CHIR::AbstractMethodParam{"p" + std::to_string(i), paramTypes.at(i)});
        patchableParamTypes.emplace_back(paramTypes.at(i));
    }

    const auto patchableMethodType = builder.GetType<CHIR::FuncType>(patchableParamTypes, funcType->GetReturnType());

    CHIR::AttributeInfo attr;
    attr.SetAttr(CHIR::Attribute::ABSTRACT, true);
    attr.SetAttr(CHIR::Attribute::PROTECTED, true);
    attr.SetAttr(CHIR::Attribute::NO_DEBUG_INFO, true);

    auto patchMethod = CHIR::AbstractMethodInfo{
        funcName.GetFuncName(), funcName.GetQualifiedName(), // TODO mangle?
        patchableMethodType, patchableParams, attr, CHIR::AnnoInfo{}, std::vector<CHIR::GenericType*>{}, false,
        guardClass};
    guardClass->AddAbstractMethod(std::move(patchMethod));

#ifdef DEBUG
    std::cout << "Guard class:" << std::endl;
    std::cout << guardClass->ToString() << std::endl;
#endif
    return guardClassType;
}

CHIR::GlobalVar* Patcher::createField(const std::string& fieldName, CHIR::EnumType* fieldType,
    const std::string& packageName) const
{
#ifdef DEBUG
    std::cout << "Create field " << fieldName << " with type " << fieldType->ToString() << std::endl;
#endif
    // TODO mangle
    const auto field = builder.CreateGlobalVar(CHIR::INVALID_LOCATION, builder.GetType<CHIR::RefType>(fieldType),
        fieldName, fieldName, fieldName, packageName);
    field->SetInitFunc(*fieldsInitializer);
#ifdef DEBUG
    std::cout << "Instantiate field" << std::endl;
#endif
    const auto bg = fieldsInitializer->GetBody();
    const auto body = bg->GetEntryBlock();
    const auto terminator = body->GetTerminator();

    const auto trueExpr = builder.CreateConstantExpression<CHIR::BoolLiteral>(builder.GetBoolTy(), body, true);
    trueExpr->MoveBefore(terminator);

    const auto tuple = builder.CreateExpression<CHIR::Tuple>(fieldType,
        std::vector<CHIR::Value*>{trueExpr->GetResult()}, body);
    auto tupleRes = tuple->GetResult();
    tuple->MoveAfter(trueExpr);

    const auto storeToField = builder.CreateExpression<CHIR::Store>(builder.GetUnitTy(), tupleRes, field, body);
    storeToField->MoveAfter(tuple);

#ifdef DEBUG
    std::cout << "Finished field creation" << std::endl;
#endif

    return field;
}

void Patcher::addFieldCheck(CHIR::GlobalVar* field, CHIR::Type* fieldType,
    const CHIR::BlockGroup* funcBlockGroup, const CHIR::AbstractMethodInfo& method) const
{
#ifdef DEBUG
    std::cout << "Add field check" << std::endl;
#endif
    const auto body = funcBlockGroup->GetEntryBlock();
    CJC_ASSERT_WITH_MSG(body->GetSuccessors().size() == 1, "entry block is expected to have the only successor");
    const auto entryBlockTerminator = body->GetTerminator();

    const auto loadField = builder.CreateExpression<CHIR::Load>(fieldType, field, body);
    loadField->MoveBefore(entryBlockTerminator);
    const auto getFieldVal = static_cast<CHIR::Expression*>(loadField);
    CJC_ASSERT_WITH_MSG(getFieldVal, "get field val should be instantiated");

    const auto optionIsNoneCall = builder.CreateExpression<CHIR::Apply>(builder.GetBoolTy(), pluginCtx->optionIsNoneDef,
        CHIR::FuncCallContext{
            .args = {getFieldVal->GetResult()},
            .instTypeArgs = {},
            .thisType = fieldType,
        }, body);
    optionIsNoneCall->MoveAfter(getFieldVal);

    const auto thenBlock = builder.CreateBlock(body->GetParentBlockGroup());
    const auto thenBlockTerminator = builder.CreateTerminator<CHIR::GoTo>(body->GetSuccessors().front(), thenBlock);
    thenBlock->AppendExpression(thenBlockTerminator);

    const auto elseBlock = builder.CreateBlock(body->GetParentBlockGroup());
    const auto elseBlockTerminator = builder.CreateTerminator<CHIR::Exit>(elseBlock);
    elseBlock->AppendExpression(elseBlockTerminator);

    const auto optionGetOrThrowCall = builder.CreateExpression<CHIR::Apply>(
        fieldType->GetTypeArgs().front(), pluginCtx->optionGetOrThrowDef, CHIR::FuncCallContext{
            .args = {getFieldVal->GetResult()},
            .instTypeArgs = {},
            .thisType = fieldType,
        }, elseBlock);
    optionGetOrThrowCall->MoveBefore(elseBlockTerminator);

    const auto funcType = dynamic_cast<CHIR::FuncType*>(method.methodTy);
    const auto funcParameters = CHIR::GetFuncParams(*funcBlockGroup);

    const auto callContext =
        CHIR::InvokeCallContext{
            .caller = optionGetOrThrowCall->GetResult(),
            .funcCallCtx = CHIR::FuncCallContext{
                .args = std::vector<CHIR::Value*>(funcParameters.begin(), funcParameters.end()),
                .instTypeArgs = {}, // TODO support generics
                .thisType = fieldType->GetTypeArgs().front(),
            },
            .virMethodCtx = CHIR::VirMethodContext{
                .srcCodeIdentifier = method.methodName,
                .originalFuncType = funcType,
                .genericTypeParams = method.methodGenericTypeParams,
            }
        };
    const auto callMethod = builder.CreateExpression<CHIR::Invoke>(funcType->GetReturnType(), callContext, elseBlock);
    callMethod->MoveAfter(optionGetOrThrowCall);

    const auto ifExpr = builder.CreateTerminator<CHIR::Branch>(optionIsNoneCall->GetResult(), thenBlock, elseBlock,
        body);
    entryBlockTerminator->ReplaceWith(*ifExpr);

#ifdef DEBUG
    std::cout << "Field check result: " << std::endl;
    std::cout << body->ToString() << std::endl;
    std::cout << thenBlock->ToString() << std::endl;
    std::cout << elseBlock->ToString() << std::endl;
#endif
}

void Patcher::patch(const Patchable& patchable) const
{
    CJC_ASSERT_WITH_MSG(fieldsInitializer, "expected field initializer not to be null");

    const auto guardClass = createGuardClass(patchable);

    const auto fieldName = patchable.funcName.GetPatchFieldName();

    std::vector<CHIR::Type*> typeArgs;
    typeArgs.emplace_back(builder.GetType<CHIR::RefType>(guardClass));
    const auto fieldType = builder.GetType<CHIR::EnumType>(pluginCtx->optionDef, typeArgs);

    const auto func = dynamic_cast<CHIR::Func*>(patchable.func);
    const auto bg = func->GetBody();

    const auto& abstractMethods = guardClass->GetClassDef()->GetAbstractMethods();
    CJC_ASSERT_WITH_MSG(abstractMethods.size() == 1, "guard class must contain only one abstract method");

    const auto field = createField(fieldName, fieldType, patchable.func->GetPackageName());

    addFieldCheck(field, fieldType, bg, abstractMethods.front());
}