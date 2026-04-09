#include "Patcher.h"


#include "cangjie/CHIR/Serializer/CHIRSerializer.h"
#include "cangjie/CHIR/Utils/Utils.h"
#include <iostream>
#include <memory>

using namespace Cangjie;
using namespace Cangjie::CHIR;
using namespace HotfixPlugin;

Patcher::Patcher(CHIRBuilder& builder, const std::shared_ptr<PluginContext>& pluginCtx)
    : builder(builder),
      package(builder.GetCurPackage()),
      pluginCtx(pluginCtx)
{
    const auto gClass = genPackageInitGuardClass();
    const auto gVar = genGuardVar(PACKAGE_INIT_GUARD_VAR_NAME, gClass->GetType(), false);
    gVar->SetInitFunc(*package->GetPackageInitFunc());
    const auto gVarFlag = genGuardVarFlag(PACKAGE_INIT_GUARD_VAR_FLAG_NAME);
    const auto gClassMethods = gClass->GetMethods();
    CJC_ASSERT_WITH_MSG(gClassMethods.size() == 3, "expected package init guard class to have 3 methods");
    CJC_ASSERT_WITH_MSG(gClassMethods.back()->GetFuncKind() == CLASS_CONSTRUCTOR,
        "expected package init guard class to have ctor");
    genPackageInitGuardChecks(package->GetPackageInitFunc(), gClass->GetType(), gVar, gVarFlag,
        gClassMethods.front(), gClassMethods.back());
    genPackageInitGuardChecks(package->GetPackageLiteralInitFunc(), gClass->GetType(), gVar, gVarFlag,
        gClassMethods.at(1), gClassMethods.back());
}

void Patcher::patch(const Patchable& patchable)
{
    if (!guardVarsInitializer) {
        guardVarsInitializer = genGuardVarsInitializer(builder.GetCurPackage());
        guardClass = genGuardClass();
        guardVar = genGuardVar(PATCHABLE_GUARD_VAR_NAME, guardClass->GetType(), true);
        guardVar->SetInitFunc(*guardVarsInitializer);
    }
    CJC_ASSERT_WITH_MSG(guardVarsInitializer && guardClass && guardVar,
        "guard var, guard class or guard vars initializer is expected to be generated");
    const auto guardVarFlag = genGuardVarFlag(patchable.funcName.getGuardVarFlagName());
    const auto guardMethod = genGuardMethod(guardClass, patchable.funcName.getGuardMethodName(), patchable.func);
    genGuardChecks(patchable.func, guardVarFlag, guardMethod);
}

/*
  CHIR pseudocode:

  [virtual] class @$PackageInitPatch <: @_CNat6ObjectE {
    [protected] [virtual] func @packageInitPatched(%0: Class-$PackageInitPatch&) : Unit {
      Block #0:
        [ret] %1: Unit& = Allocate(Unit)
        %2: Struct-_CNat6StringE = Constant("should not reach here")
        %3: Class-_CNat9ExceptionE& = Allocate(Class-_CNat9ExceptionE)
        %4: Unit = Apply(ThisType: Class-_CNat9ExceptionE&, @_CNat9Exception6<init>HRNat6StringE, %3, %2)
        RaiseException(%3)
    }

    [protected] [virtual] func @packageLiteralInitPatched(%0: Class-$PackageInitPatch&) : Unit {
      Block #0:
        [ret] %1: Unit& = Allocate(Unit)
        %2: Struct-_CNat6StringE = Constant("should not reach here")
        %3: Class-_CNat9ExceptionE& = Allocate(Class-_CNat9ExceptionE)
        %4: Unit = Apply(ThisType: Class-_CNat9ExceptionE&, @_CNat9Exception6<init>HRNat6StringE, %3, %2)
        RaiseException(%3)
    }

    [public] func @<init>(%0: Class-$PackageInitPatch&) : Unit {
      Block #0:
        [ret] %1: Unit& = Allocate(Unit)
        Exit()
    }
  }
 */
ClassDef* Patcher::genPackageInitGuardClass() const
{
#ifdef DEBUG
    std::cout << "Create guard class for package inits" << std::endl;
#endif
    const auto cl = builder.CreateClass(INVALID_LOCATION, PACKAGE_INIT_GUARD_CLASS_NAME, PACKAGE_INIT_GUARD_CLASS_NAME,
        package->GetName(), true, false);
    const auto guardClassType = builder.GetType<ClassType>(cl);
    cl->SetType(*guardClassType);
    cl->SetSuperClassTy(*builder.GetObjectTy());
    cl->EnableAttr(Attribute::VIRTUAL);
    cl->EnableAttr(Attribute::INTERNAL);
    cl->EnableAttr(Attribute::COMPILER_ADD);
    cl->Set<LinkTypeInfo>(Linkage::EXTERNAL);

    std::vector<Type*> paramTypes{builder.GetType<RefType>(cl->GetType())};
    auto defVTable = cl->GetModifiableDefVTable();

    const auto genPatchableMethod = [&](const std::string& name) -> void {
        const auto mt = builder.GetType<FuncType>(paramTypes, builder.GetUnitTy());

        const auto m = builder.CreateFuncWithBody(INVALID_LOCATION, mt, name,
            name, name, package->GetName(), {});
        m->EnableAttr(Attribute::VIRTUAL);
        m->EnableAttr(Attribute::COMPILER_ADD);
        m->EnableAttr(Attribute::NO_REFLECT_INFO);
        m->EnableAttr(Attribute::NO_INLINE);
        m->EnableAttr(Attribute::PROTECTED);

        for (const auto paramType : paramTypes) {
            builder.CreateParameter(paramType, INVALID_LOCATION, *m);
        }

        const auto bg = builder.CreateBlockGroup(*m);
        m->InitBody(*bg);
        const auto block = builder.CreateBlock(bg);
        bg->SetEntryBlock(block);

        const auto retVal = CHIR::CreateAndAppendExpression<Allocate>(builder, INVALID_LOCATION,
            builder.GetType<RefType>(builder.GetUnitTy()), builder.GetUnitTy(), block)->GetResult();
        m->SetReturnValue(*retVal);

        genShouldNotReachHere(block);

        cl->AddMethod(m);

#ifdef DEBUG
        std::cout << "method:" << std::endl;
        std::cout << m->ToString() << std::endl;
#endif
    };

    genPatchableMethod(PACKAGE_INIT_GUARD_METHOD_NAME);
    genPatchableMethod(PACKAGE_LITERAL_INIT_GUARD_METHOD_NAME);

    const auto ctorType = builder.GetType<FuncType>(paramTypes, builder.GetUnitTy());

    const auto ctor = builder.CreateFuncWithBody(INVALID_LOCATION, ctorType, "<init>",
        "init", "<init>", package->GetName(), {});
    ctor->SetFuncKind(CLASS_CONSTRUCTOR);
    ctor->EnableAttr(Attribute::COMPILER_ADD);
    ctor->EnableAttr(Attribute::PUBLIC);

    for (const auto paramType : paramTypes) {
        builder.CreateParameter(paramType, INVALID_LOCATION, *ctor);
    }

    const auto bg = builder.CreateBlockGroup(*ctor);
    ctor->InitBody(*bg);
    const auto block = builder.CreateBlock(bg);
    bg->SetEntryBlock(block);

    const auto retVal = CHIR::CreateAndAppendExpression<Allocate>(builder, INVALID_LOCATION,
        builder.GetType<RefType>(builder.GetUnitTy()), builder.GetUnitTy(), block)->GetResult();
    ctor->SetReturnValue(*retVal);

    CHIR::CreateAndAppendTerminator<Exit>(builder, block);

    cl->AddMethod(ctor);

#ifdef DEBUG
    std::cout << "ctor:" << std::endl;
    std::cout << ctor->ToString() << std::endl;
#endif

#ifdef DEBUG
    std::cout << "result:" << std::endl;
    std::cout << cl->ToString() << std::endl;
#endif

    return cl;
}

/*
  CHIR pseudocode:

  package init before:

    @$has_applied_pkg_init_func: Bool& = false
    @$packageInitPatchVar: Enum-_CNat6OptionIG_E<Class-$PackageInitPatch&>&
    @$packageInitPatchVarFlag: Bool& = false

    func packageInit() : Unit {
      Block #0:
        [ret] %0: Unit& = Allocate(Unit)
        %1: Bool = Load(@$has_applied_pkg_init_func)
        Branch(%1, #1, #2)
      Block #1:
        Exit()
      Block #2:
        %2: Bool = Constant(true)
        %3: Unit = Store(%2, @$has_applied_pkg_init_func)
        ...
        Exit()
    }

  package init after:

    @$has_applied_pkg_init_func: Bool& = false
    @$packageInitPatchVar: Enum-_CNat6OptionIG_E<Class-$PackageInitPatch&>&
    @$packageInitPatchVarFlag: Bool& = false

    func packageInit() : Unit {
      Block #3:
        %17: Bool = Load(@$packageInitPatchVarFlag)
        %18: Bool = Not(%17)
        Branch(%18, #0, #4)
      Block #0:
        [ret] %0: Unit& = Allocate(Unit)
        %1: Bool = Load(@$has_applied_pkg_init_func)
        Branch(%1, #1, #2)
      Block #1:
        Exit()
      Block #2:
        %2: Bool = Constant(true)
        %3: Unit = Store(%2, @$has_applied_pkg_init_func)
        ...
        Exit()
      Block #4:
        %19: Enum-_CNat6OptionIG_E<Class-$PackageInitPatch&>& = Allocate(Enum-_CNat6OptionIG_E<Class-$PackageInitPatch&>)
        %20: Enum-_CNat6OptionIG_E<Class-$PackageInitPatch&> = Load(@$packageInitPatchVar)
        %21: Bool = Field(%20, 0)
        %22: Bool = Constant(false)
        %23: Bool = Equal(%21, %22)
        Branch(%23, #5, #6)
      Block #5:
        %24: Unit = Store(%20, %19)
        GoTo(#7)
      Block #6:
        %25: Class-$PackageInitPatch& = Allocate(Class-$PackageInitPatch)
        %26: Unit = Apply(ThisType: Class-$PackageInitPatch&, @<init>, %25)
        %27: Bool = Constant(false)
        %28: Enum-_CNat6OptionIG_E<Class-$PackageInitPatch&> = Tuple(%27, %25)
        %29: Unit = Store(%28, %19)
        GoTo(#7)
      Block #7:
        %30: Enum-_CNat6OptionIG_E<Class-$PackageInitPatch&> = Load(%19)
        %31: Unit = Store(%30, @$packageInitPatchVar)
        %32: Enum-_CNat6OptionIG_E<Class-$PackageInitPatch&> = Load(@$packageInitPatchVar)
        %33: Tuple(Bool,Class-$PackageInitPatch&) = TypeCast(%32)
        %34: Class-$PackageInitPatch& = Field(%33, 1)
        %35: Unit = Invoke(ThisType: Class-$PackageInitPatch&, packageInitPatched: (Class-$PackageInitPatch&) -> Unit, %34)
        Exit()
    }
  }
 */
void Patcher::genPackageInitGuardChecks(const Function* patchable, Type* guardClassType, GlobalVar* guardVar,
    GlobalVar* guardVarFlag, const Function* guardMethod, Function* guardClassCtor) const
{
#ifdef DEBUG
    std::cout << "Gen guard check" << std::endl;
#endif

    const auto bg = patchable->GetBody();
    auto entryBlock = patchable->GetEntryBlock();
    auto successors = entryBlock->GetSuccessors();
    CJC_ASSERT_WITH_MSG(successors.size() == 2, "entry block of package inits is expected to have two successors");
    CJC_ASSERT_WITH_MSG(entryBlock->GetPredecessors().empty(),
        "entry block of package inits is expected to have zero predecessors");
    auto newEntryBlock = builder.CreateBlock(entryBlock->GetParentBlockGroup());
    newEntryBlock->AppendExpression(builder.CreateTerminator<GoTo>(entryBlock, newEntryBlock));
    entryBlock = newEntryBlock;
    successors = entryBlock->GetSuccessors();
    bg->SetEntryBlock(newEntryBlock);
    CJC_ASSERT_WITH_MSG(successors.size() == 1, "new entry block of package inits is expected to have one successor");
    const auto entryBlockTerminator = entryBlock->GetTerminator();

    const auto loadGuardVarFlag = builder.CreateExpression<Load>(builder.GetBoolTy(), guardVarFlag, entryBlock);
    loadGuardVarFlag->MoveBefore(entryBlockTerminator);
    const auto guardVarFlagCond = builder.CreateExpression<UnaryExpression>(builder.GetBoolTy(), CHIR::ExprKind::NOT,
        loadGuardVarFlag->GetResult(), OverflowStrategy::THROWING, entryBlock);
    guardVarFlagCond->MoveAfter(loadGuardVarFlag);

    const auto guardVarCheckBlock = builder.CreateBlock(entryBlock->GetParentBlockGroup());

    const auto guardVarFlagCheckTerminator = builder.CreateTerminator<Branch>(guardVarFlagCond->GetResult(),
        successors.front(), guardVarCheckBlock, entryBlock);
    entryBlockTerminator->ReplaceWith(*guardVarFlagCheckTerminator);

    const auto guardVarType = guardVar->GetType();
    CJC_ASSERT_WITH_MSG(guardVarType->IsRef(), "expected guard var type to be ref type");
    const auto guardVarBaseType = static_cast<RefType*>(guardVarType)->GetBaseType();
    CJC_ASSERT_WITH_MSG(guardVarBaseType->IsEnum(), "expected guard var base type to be enum");
    const auto guardVarBaseEnumType = static_cast<EnumType*>(guardVarBaseType);
    CJC_ASSERT_WITH_MSG(guardVarBaseEnumType->IsOption(), "expected guard var base type to be option");

    const auto guardVarAlloc = CHIR::CreateAndAppendExpression<Allocate>(builder, guardVarType, guardVarBaseType,
        guardVarCheckBlock);

    auto loadGuardVar = CHIR::CreateAndAppendExpression<Load>(builder, guardVarBaseType, guardVar,
        guardVarCheckBlock);

    auto field = CHIR::CreateAndAppendExpression<Field>(builder, builder.GetBoolTy(), loadGuardVar->GetResult(),
        std::vector<uint64_t>{0}, guardVarCheckBlock);

    auto falseConst = builder.CreateConstantExpression<BoolLiteral>(builder.GetBoolTy(), guardVarCheckBlock,
        false);
    guardVarCheckBlock->AppendExpression(falseConst);

    const auto guardVarCheckCond = CHIR::CreateAndAppendExpression<BinaryExpression>(builder, builder.GetBoolTy(),
        CHIR::ExprKind::EQUAL, field->GetResult(), falseConst->GetResult(), guardVarCheckBlock);

    const auto guardVarInitializedBlock = builder.CreateBlock(entryBlock->GetParentBlockGroup());
    const auto guardVarNotInitializedBlock = builder.CreateBlock(entryBlock->GetParentBlockGroup());
    const auto callPatchBlock = builder.CreateBlock(entryBlock->GetParentBlockGroup());

    CHIR::CreateAndAppendTerminator<Branch>(builder, guardVarCheckCond->GetResult(), guardVarInitializedBlock,
        guardVarNotInitializedBlock, guardVarCheckBlock);

    CHIR::CreateAndAppendExpression<Store>(builder, builder.GetUnitTy(), loadGuardVar->GetResult(),
        guardVarAlloc->GetResult(), guardVarInitializedBlock);
    CHIR::CreateAndAppendTerminator<GoTo>(builder, callPatchBlock, guardVarInitializedBlock);

    auto guardClassRefType = builder.GetType<RefType>(guardClassType);
    const auto patchAlloc = CHIR::CreateAndAppendExpression<Allocate>(builder, guardClassRefType,
        guardClassType, guardVarNotInitializedBlock);

    CHIR::CreateAndAppendExpression<Apply>(builder, builder.GetUnitTy(), guardClassCtor,
        FuncCallContext{
            .args = {patchAlloc->GetResult()},
            .instTypeArgs = {},
            .thisType = guardClassRefType,
        }, guardVarNotInitializedBlock);

    falseConst = builder.CreateConstantExpression<BoolLiteral>(builder.GetBoolTy(), guardVarNotInitializedBlock, false);
    guardVarNotInitializedBlock->AppendExpression(falseConst);

    const auto tuple = CHIR::CreateAndAppendExpression<Tuple>(builder, guardVarBaseType,
        std::vector<Value*>{falseConst->GetResult(), patchAlloc->GetResult()}, guardVarNotInitializedBlock);

    CHIR::CreateAndAppendExpression<Store>(builder, builder.GetUnitTy(), tuple->GetResult(),
        guardVarAlloc->GetResult(), guardVarNotInitializedBlock);

    CHIR::CreateAndAppendTerminator<GoTo>(builder, callPatchBlock, guardVarNotInitializedBlock);

    loadGuardVar = CHIR::CreateAndAppendExpression<Load>(builder, guardVarBaseType, guardVarAlloc->GetResult(),
        callPatchBlock);

    CHIR::CreateAndAppendExpression<Store>(builder, builder.GetUnitTy(), loadGuardVar->GetResult(), guardVar,
        callPatchBlock);

    loadGuardVar = CHIR::CreateAndAppendExpression<Load>(builder, guardVarBaseType, guardVar,
        callPatchBlock);

    const auto tupleType = builder.GetType<TupleType>(std::vector<Type*>{builder.GetBoolTy(), guardClassRefType});
    const auto typeCast = CHIR::CreateAndAppendExpression<TypeCast>(builder, tupleType, loadGuardVar->GetResult(),
        callPatchBlock);

    field = CHIR::CreateAndAppendExpression<Field>(builder, guardClassRefType, typeCast->GetResult(),
        std::vector<uint64_t>{1},
        callPatchBlock);

    const auto funcParameters = GetFuncParams(*bg);
    const auto callContext =
        InvokeCallContext{
            .caller = field->GetResult(),
            .funcCallCtx = FuncCallContext{
                .args = {},
                .instTypeArgs = {}, // TODO support generics
                .thisType = guardClassRefType,
            },
            .virMethodCtx = VirMethodContext{
                .srcCodeIdentifier = guardMethod->GetSrcCodeIdentifier(),
                .originalFuncType = guardMethod->GetFuncType(),
                .genericTypeParams = guardMethod->GetGenericTypeParams(),
            }
        };
    CHIR::CreateAndAppendExpression<Invoke>(builder, guardMethod->GetReturnType(), callContext, callPatchBlock);

    CHIR::CreateAndAppendTerminator<Exit>(builder, callPatchBlock);

#ifdef DEBUG
    std::cout << patchable->ToString() << std::endl;
#endif
}

/*
  CHIR pseudocode:

  [abstract] class @$Patch <: @_CNat6ObjectE {
  }
 */
ClassDef* Patcher::genGuardClass() const
{
#ifdef DEBUG
    std::cout << "Create guard class" << std::endl;
#endif
    const auto cl = builder.CreateClass(INVALID_LOCATION, PATCHABLE_GUARD_CLASS_NAME,
        PATCHABLE_GUARD_CLASS_NAME, PATCHABLE_GUARD_CLASS_NAME, true, false);
    const auto guardClassType = builder.GetType<ClassType>(cl);
    cl->SetType(*guardClassType);
    cl->SetSuperClassTy(*builder.GetObjectTy());
    cl->EnableAttr(Attribute::ABSTRACT);
    cl->EnableAttr(Attribute::COMPILER_ADD);

#ifdef DEBUG
    std::cout << "Guard class:" << std::endl;
    std::cout << cl->ToString() << std::endl;
#endif
    return cl;
}

/*
  CHIR pseudocode:

  func package_init() {
    ...
    %x: Unit = Apply(@$guardVarsInit)
  }

  func @guardVarsInit() {
    Block #0:
      [ret] %0: Unit& = Allocate(Unit)
      Exit()
  }
 */
Function* Patcher::genGuardVarsInitializer(const Package* package) const
{
    std::vector<Type*> paramTypes;
    const auto funcType = builder.GetType<FuncType>(paramTypes, builder.GetUnitTy());
    // TODO mangle?
    const auto func = builder.CreateFuncWithBody(INVALID_LOCATION, funcType,
        PATCHABLE_GUARD_VARS_INITIALIZER,
        PATCHABLE_GUARD_VARS_INITIALIZER, PATCHABLE_GUARD_VARS_INITIALIZER, package->GetName(), {});
    func->EnableAttr(Attribute::COMPILER_ADD);
    func->EnableAttr(Attribute::NO_REFLECT_INFO);
    func->EnableAttr(Attribute::NO_INLINE);

    const auto bg = builder.CreateBlockGroup(*func);
    func->InitBody(*bg);
    const auto entryBlock = builder.CreateBlock(bg);
    bg->SetEntryBlock(entryBlock);

    const auto retVal = CHIR::CreateAndAppendExpression<Allocate>(builder, INVALID_LOCATION,
        builder.GetType<RefType>(builder.GetUnitTy()), builder.GetUnitTy(), entryBlock)->GetResult();
    func->SetReturnValue(*retVal);

    const auto terminator = builder.CreateTerminator<Exit>(entryBlock);
    entryBlock->AppendExpression(terminator);

    const auto packageInitBody = package->GetPackageInitFunc()->GetBody();
    for (const auto block : packageInitBody->GetBlocks()) {
        if (block->TestAttr(Attribute::INITIALIZER)) {
            const auto guardVarsInitializerCall = builder.CreateExpression<Apply>(
                builder.GetUnitTy(), func, FuncCallContext{
                    .args = {},
                    .instTypeArgs = {},
                    .thisType = nullptr,
                }, block);
            guardVarsInitializerCall->MoveBefore(block->GetTerminator());
            break;
        }
    }

    return func;
}

/*
 * CJ source code:

   func foo() {
     ...
   }

   CHIR pseudocode:

  [abstract] class @$Patch <: @_CNat6ObjectE {
    [protected] [abstract] func $fooPatch: (Class-$Patch&) -> Unit
  }
 */
AbstractMethodInfo Patcher::genGuardMethod(ClassDef* guardClass, const std::string& name,
    const Function* patchable) const
{
#ifdef DEBUG
    std::cout << "gen guard method for patchable func:" << std::endl;
    std::cout << patchable->ToString() << std::endl;
#endif

    std::vector<Type*> methodParamTypes;
    if (!patchable->TestAttr(Attribute::STATIC)) {
        methodParamTypes.emplace_back(builder.GetType<RefType>(guardClass->GetType()));
    }
    std::vector<AbstractMethodParam> methodParams;
    const auto patchableMethodType = static_cast<FuncType*>(patchable->GetType());
    const auto paramTypes = patchableMethodType->GetParamTypes();
    for (size_t i = 0; i < paramTypes.size(); ++i) {
        methodParams.emplace_back(AbstractMethodParam{"p" + std::to_string(i), paramTypes.at(i)});
        methodParamTypes.emplace_back(paramTypes.at(i));
    }

    const auto methodType = builder.GetType<FuncType>(methodParamTypes, patchableMethodType->GetReturnType());

    AttributeInfo attr;
    attr.SetAttr(Attribute::ABSTRACT, true);
    attr.SetAttr(Attribute::PROTECTED, true);
    attr.SetAttr(Attribute::NO_DEBUG_INFO, true);
    attr.SetAttr(Attribute::COMPILER_ADD, true);

    auto method = AbstractMethodInfo{name, name, methodType, methodParams, attr, AnnoInfo{},
                                     std::vector<GenericType*>{}, false, guardClass};
    guardClass->AddAbstractMethod(method);

#ifdef DEBUG
    std::cout << "guard class with new guard method:" << std::endl;
    std::cout << guardClass->ToString() << std::endl;
#endif

    return method;
}

/*
  CHIR pseudocode:

  $patchVar: Enum-_CNat6OptionIG_E<Class-$Patch&>&

  $guardVarsInit() {
    Block #0:
      ...
      %1: Bool = Constant(true)
      %2: Enum-_CNat6OptionIG_E<Class-$Patch&> = Tuple(%1)
      %3: Unit = Store(%2, @$patchVar)
      ...
  }
 */
GlobalVar* Patcher::genGuardVar(const std::string& name, ClassType* guardClassType, const bool needToInstantiate) const
{
#ifdef DEBUG
    std::cout << "Gen guard var " << name << " for class " << guardClassType->ToString() << std::endl;
#endif
    std::vector<Type*> typeArgs;
    typeArgs.emplace_back(builder.GetType<RefType>(guardClassType));
    const auto guardVarType = builder.GetType<EnumType>(pluginCtx->optionDef, typeArgs);

    const auto gv = builder.CreateGlobalVarWithInit(INVALID_LOCATION, builder.GetType<RefType>(guardVarType),
        name, name, name, package->GetName());
    if (needToInstantiate) {
#ifdef DEBUG
        std::cout << "Instantiate guard var" << std::endl;
#endif

        const auto bg = guardVarsInitializer->GetBody();
        const auto body = bg->GetEntryBlock();
        const auto terminator = body->GetTerminator();

        const auto trueExpr = builder.CreateConstantExpression<BoolLiteral>(builder.GetBoolTy(), body, true);
        trueExpr->MoveBefore(terminator);

        const auto tuple = builder.CreateExpression<Tuple>(guardVarType, std::vector<Value*>{trueExpr->GetResult()},
            body);
        auto tupleRes = tuple->GetResult();
        tuple->MoveAfter(trueExpr);

        const auto storeGuardVar = builder.CreateExpression<Store>(builder.GetUnitTy(), tupleRes, gv, body);
        storeGuardVar->MoveAfter(tuple);
#ifdef DEBUG
        std::cout << "guard vars initializer body:" << std::endl;
        std::cout << bg->ToString() << std::endl;
#endif
    }
#ifdef DEBUG
    std::cout << gv->ToString() << std::endl;
#endif
    return gv;
}

/*
  CHIR pseudo-code:

  @$fooPatchVarFlag: Bool& = false
 */
GlobalVar* Patcher::genGuardVarFlag(const std::string& name) const
{
#ifdef DEBUG
    std::cout << "Gen guard var flag " << name << std::endl;
#endif
    const auto gvf = builder.CreateGlobalVarWithInit(INVALID_LOCATION,
        builder.GetType<RefType>(builder.GetBoolTy()), name, name, name, package->GetName());
    const auto falseLiteral = builder.CreateLiteralValue<BoolLiteral>(builder.GetBoolTy(), false);
    gvf->SetInitializer(*falseLiteral);
#ifdef DEBUG
    std::cout << gvf->ToString() << std::endl;
#endif
    return gvf;
}

/*
  CJ source code:

  @patchable
  func foo() {
    println("foo")
  }

  CHIR pseudocode before:

  func foo() {
    Block #0:
      [ret] %0: Unit& = Allocate(Unit)
      GoTo(#1)
    Block #1:
      %1: Struct-_CNat6StringE = Constant("foo")
      %2: Unit = Apply(@_CNat7printlnHRNat6StringE, %1)
      %3: Unit = Constant(unit)
      %4: Unit = Store(%3, %0)
      Exit()
  }

  CHIR pseudocode after:

  func foo() {
    Block #0:
      [ret] %0: Unit& = Allocate(Unit)
      %5: Bool = Load(@$fooPatchVarFlag)
      %6: Bool = Not(%5)
      Branch(%6, #1, #3)
    Block #1:
      %1: Struct-_CNat6StringE = Constant("foo")
      %2: Unit = Apply(@_CNat7printlnHRNat6StringE, %1)
      %3: Unit = Constant(unit)
      %4: Unit = Store(%3, %0)
      Exit()
    Block #3:
      %7: Enum-_CNat6OptionIG_E<Class-$Patch&> = Load(@$patchVar)
      %8: Bool = Field(%7, 0)
      %9: Bool = Constant(false)
      %10: Bool = Equal(%8, %9)
      Branch(%10, #4, #5)
    Block #4:
      %14: Tuple(Bool,Class-$Patch&) = TypeCast(%7)
      [readOnly] %15: Class-$Patch& = Field(%14, 1)
      %16: Unit = Invoke(ThisType: Class-$Patch&, $fooPatch: (Class-$Patch&) -> Unit, %15)
      %17: Unit = Constant(unit)
      %18: Unit = Store(%17, %0)
      Exit()
    Block #5:
      %11: Struct-_CNat6StringE = Constant("should not reach here")
      %12: Class-_CNat9ExceptionE& = Allocate(Class-_CNat9ExceptionE)
      %13: Unit = Apply(ThisType: Class-_CNat9ExceptionE&, @_CNat9Exception6<init>HRNat6StringE, %12, %11)
      RaiseException(%12)
  }

 */
void Patcher::genGuardChecks(const Function* patchable, GlobalVar* guardVarFlag,
    const AbstractMethodInfo& guardMethod) const
{
#ifdef DEBUG
    std::cout << "Gen guard check" << std::endl;
#endif
    const auto bg = patchable->GetBody();
    auto entryBlock = patchable->GetEntryBlock();
    auto successors = entryBlock->GetSuccessors();
    CJC_ASSERT_WITH_MSG(successors.size() == 1, "entry block is expected to have one successor");
    const auto entryBlockTerminator = entryBlock->GetTerminator();

    const auto loadGuardVarFlag = builder.CreateExpression<Load>(builder.GetBoolTy(), guardVarFlag, entryBlock);
    loadGuardVarFlag->MoveBefore(entryBlockTerminator);
    const auto guardVarFlagCond = builder.CreateExpression<UnaryExpression>(builder.GetBoolTy(), CHIR::ExprKind::NOT,
        loadGuardVarFlag->GetResult(), OverflowStrategy::THROWING, entryBlock);
    guardVarFlagCond->MoveAfter(loadGuardVarFlag);

    const auto guardVarCheckBlock = builder.CreateBlock(entryBlock->GetParentBlockGroup());

    const auto guardVarFlagCheckTerminator = builder.CreateTerminator<Branch>(guardVarFlagCond->GetResult(),
        successors.front(), guardVarCheckBlock, entryBlock);
    entryBlockTerminator->ReplaceWith(*guardVarFlagCheckTerminator);

    const auto guardVarType = guardVar->GetType();
    CJC_ASSERT_WITH_MSG(guardVarType->IsRef(), "expected guard var type to be ref type");
    const auto guardVarBaseType = static_cast<RefType*>(guardVarType)->GetBaseType();
    const auto loadGuardVar = CHIR::CreateAndAppendExpression<Load>(builder, guardVarBaseType, guardVar,
        guardVarCheckBlock);

    const auto field = CHIR::CreateAndAppendExpression<Field>(builder, builder.GetBoolTy(), loadGuardVar->GetResult(),
        std::vector<uint64_t>{0}, guardVarCheckBlock);

    const auto falseConst = builder.CreateConstantExpression<BoolLiteral>(builder.GetBoolTy(), guardVarCheckBlock,
        false);
    guardVarCheckBlock->AppendExpression(falseConst);

    const auto guardVarCheckCond = CHIR::CreateAndAppendExpression<BinaryExpression>(builder, builder.GetBoolTy(),
        CHIR::ExprKind::EQUAL, field->GetResult(), falseConst->GetResult(), guardVarCheckBlock);

    const auto callPatchBlock = builder.CreateBlock(entryBlock->GetParentBlockGroup());

    const auto shouldNotReachHereBlock = builder.CreateBlock(entryBlock->GetParentBlockGroup());
    genShouldNotReachHere(shouldNotReachHereBlock);
    CHIR::CreateAndAppendTerminator<Branch>(builder, guardVarCheckCond->GetResult(), callPatchBlock,
        shouldNotReachHereBlock, guardVarCheckBlock);

    const std::vector<Type*> optionTypeArgs{builder.GetBoolTy(), guardVarBaseType->GetTypeArgs().front()};
    const auto optionType = builder.GetType<TupleType>(optionTypeArgs);
    const auto typeCast = CHIR::CreateAndAppendExpression<TypeCast>(builder, optionType, loadGuardVar->GetResult(),
        callPatchBlock);

    const auto caller = CHIR::CreateAndAppendExpression<Field>(builder, guardVarBaseType->GetTypeArgs().front(),
        typeCast->GetResult(), std::vector<uint64_t>{1}, callPatchBlock);
    caller->GetResult()->EnableAttr(Attribute::READONLY);

    const auto funcType = dynamic_cast<FuncType*>(guardMethod.methodTy);
    const auto funcParameters = GetFuncParams(*bg);
    const auto callContext =
        InvokeCallContext{
            .caller = caller->GetResult(),
            .funcCallCtx = FuncCallContext{
                .args = std::vector<Value*>(funcParameters.begin(), funcParameters.end()),
                .instTypeArgs = {}, // TODO support generics
                .thisType = guardVarBaseType->GetTypeArgs().front(),
            },
            .virMethodCtx = VirMethodContext{
                .srcCodeIdentifier = guardMethod.methodName,
                .originalFuncType = funcType,
                .genericTypeParams = guardMethod.methodGenericTypeParams,
            }
        };
    const auto callMethod = CHIR::CreateAndAppendExpression<Invoke>(builder, funcType->GetReturnType(), callContext,
        callPatchBlock);

    if (const auto resultType = callMethod->GetResultType()) {
        if (resultType->IsUnit()) {
            const auto unitConst = builder.CreateConstantExpression<UnitLiteral>(builder.GetUnitTy(), callPatchBlock);
            unitConst->MoveAfter(callMethod);
            CHIR::CreateAndAppendExpression<Store>(builder, builder.GetUnitTy(), unitConst->GetResult(),
                patchable->GetReturnValue(), callPatchBlock);
        } else {
            CHIR::CreateAndAppendExpression<Store>(builder, builder.GetUnitTy(), callMethod->GetResult(),
                patchable->GetReturnValue(), callPatchBlock);
        }
    }

    CHIR::CreateAndAppendTerminator<Exit>(builder, callPatchBlock);

#ifdef DEBUG
    std::cout << patchable->ToString() << std::endl;
#endif
}

/*
  CHIR pseudocode:

  Block #x:
    %0: Struct-_CNat6StringE = Constant("should not reach here")
    %1: Class-_CNat9ExceptionE& = Allocate(Class-_CNat9ExceptionE)
    %2: Unit = Apply(ThisType: Class-_CNat9ExceptionE&, @_CNat9Exception6<init>HRNat6StringE, %1, %0)
    RaiseException(%2)
 */
void Patcher::genShouldNotReachHere(Block* block) const
{
    const auto errMsg = builder.CreateConstantExpression<StringLiteral>(builder.GetStringTy(), block,
        "should not reach here");
    block->AppendExpression(errMsg);

    const auto exceptionType = pluginCtx->exceptionDef->GetType();
    const auto exceptionRefType = builder.GetType<RefType>(exceptionType);
    const auto allocException = CHIR::CreateAndAppendExpression<Allocate>(builder, exceptionRefType,
        exceptionType, block);

    CHIR::CreateAndAppendExpression<Apply>(builder, builder.GetUnitTy(),
        pluginCtx->exceptionInitDef, FuncCallContext{
            .args = {allocException->GetResult(), errMsg->GetResult()},
            .instTypeArgs = {},
            .thisType = exceptionRefType,
        }, block);

    CHIR::CreateAndAppendTerminator<RaiseException>(builder, allocException->GetResult(), block);
}