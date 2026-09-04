#include "Equality.h"

#include "cangjie/CHIR/Analysis/ValueDomain.h"
#include "cangjie/CHIR/Utils/Utils.h"

namespace Cangjie::CHIR {
bool operator==(const Package& l, const Package& r)
{
    return l.GetName() == r.GetName();
}
}

namespace PatchGenerator {

bool areEqual(const CustomTypeDef* l, const CustomTypeDef* r)
{
    return CustomDefEquality()(l, r);
}

bool areEqual(const Function* l, const Function* r)
{
    return FunctionEquality()(l, r);
}

template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};

template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

// region Type
using ConcreteType = std::variant<
    CustomType*,
    FuncType*,
    GenericType*,
    Type*>;

ConcreteType wrap(Type* b)
{
    if (!b) {
        return static_cast<Type*>(nullptr);
    }
    if (typeid(*b) == typeid(BooleanType)) {
        return b;
    }
    if (typeid(*b) == typeid(BoxType)) {
        return b;
    }
    if (typeid(*b) == typeid(CPointerType)) {
        return b;
    }
    if (typeid(*b) == typeid(CStringType)) {
        return b;
    }
    if (typeid(*b) == typeid(ClassType)) {
        return static_cast<CustomType*>(b);
    }
    if (typeid(*b) == typeid(EnumType)) {
        return static_cast<CustomType*>(b);
    }
    if (typeid(*b) == typeid(FloatType)) {
        return b;
    }
    if (typeid(*b) == typeid(FuncType)) {
        return static_cast<FuncType*>(b);
    }
    if (typeid(*b) == typeid(GenericType)) {
        return static_cast<GenericType*>(b);
    }
    if (typeid(*b) == typeid(IntType)) {
        return b;
    }
    if (typeid(*b) == typeid(NothingType)) {
        return b;
    }
    if (typeid(*b) == typeid(RawArrayType)) {
        return b;
    }
    if (typeid(*b) == typeid(RefType)) {
        return b;
    }
    if (typeid(*b) == typeid(RuneType)) {
        return b;
    }
    if (typeid(*b) == typeid(RawArrayType)) {
        return static_cast<CustomType*>(b);
    }
    if (typeid(*b) == typeid(StructType)) {
        return static_cast<CustomType*>(b);
    }
    if (typeid(*b) == typeid(ThisType)) {
        return b;
    }
    if (typeid(*b) == typeid(TupleType)) {
        return b;
    }
    if (typeid(*b) == typeid(UnitType)) {
        return b;
    }
    if (typeid(*b) == typeid(VArrayType)) {
        return b;
    }
    if (typeid(*b) == typeid(VoidType)) {
        return b;
    }
    CJC_ABORT_WITH_MSG("unknown Type " + b->ToString()); // throw exception
    return b;
}

bool areEqual(Type* first, Type* second)
{
    if (first == second) {
        return true;
    }

    const auto v1 = wrap(first);
    const auto v2 = wrap(second);

    if (v1.index() != v2.index()) {
        return false;
    }

    const auto res = std::visit(overloaded{
        [](const CustomType* l, const CustomType* r) {
            return l->GetTypeKind() == r->GetTypeKind() &&
                areEqual(l->GetCustomTypeDef(), r->GetCustomTypeDef()) &&
                areEqual(l->GetGenericArgs(), r->GetGenericArgs());
        },
        [](const FuncType* l, const FuncType* r) {
            return l->IsCFunc() == r->IsCFunc() &&
                l->HasVarArg() == r->HasVarArg() &&
                areEqual(l->GetReturnType(), r->GetReturnType()) &&
                areEqual(l->GetParamTypes(), r->GetParamTypes());
        },
        [](const GenericType* l, const GenericType* r) {
            return l->GetIdentifier() == r->GetIdentifier() &&
                l->GetSrcCodeIdentifier() == r->GetSrcCodeIdentifier();
        },
        [](const Type* l, const Type* r) {
            return l->GetTypeKind() == r->GetTypeKind() && areEqual(l->GetTypeArgs(), r->GetTypeArgs());
        },
    }, v1, v2);
#if DEBUG
    if (!res) {
        std::cout << "Types differ" << std::endl;
        std::cout << "first " << std::endl;
        std::cout << first->ToString() << std::endl;
        std::cout << "second " << std::endl;
        std::cout << second->ToString() << std::endl;
    }
#endif
    return res;
}

bool areEqual(const std::vector<Type*>& l, const std::vector<Type*>& r)
{
    return std::equal(l.begin(), l.end(), r.begin(), r.end(),
        [](Type* lType, Type* rType) {
            return areEqual(lType, rType);
        });
}

bool areEqual(const std::vector<GenericType*>& l, const std::vector<GenericType*>& r)
{
    return std::equal(l.begin(), l.end(), r.begin(), r.end(),
        [](GenericType* lVal, GenericType* rVal) {
            return areEqual(lVal, rVal);
        });
};

// endregion Type

// region Value
using ConcreteValue = std::variant<
    BoolLiteral*,
    Block*,
    BlockGroup*,
    FloatLiteral*,
    Function*,
    GlobalVar*,
    IntLiteral*,
    NullLiteral*,
    LocalVar*,
    Parameter*,
    RuneLiteral*,
    StringLiteral*,
    UnitLiteral*,
    Value*>;

ConcreteValue wrap(Value* b)
{
    if (!b) {
        return static_cast<Value*>(nullptr);
    }
    if (typeid(*b) == typeid(AbstractObject)) {
        return b;
    }
    if (typeid(*b) == typeid(BoolLiteral)) {
        return static_cast<BoolLiteral*>(b);
    }
    if (typeid(*b) == typeid(Block)) {
        return static_cast<Block*>(b);
    }
    if (typeid(*b) == typeid(BlockGroup)) {
        return static_cast<BlockGroup*>(b);
    }
    if (typeid(*b) == typeid(FloatLiteral)) {
        return static_cast<FloatLiteral*>(b);
    }
    if (typeid(*b) == typeid(Function)) {
        return dynamic_cast<Function*>(b);
    }
    if (typeid(*b) == typeid(GlobalVar)) {
        return dynamic_cast<GlobalVar*>(b);
    }
    if (typeid(*b) == typeid(IntLiteral)) {
        return static_cast<IntLiteral*>(b);
    }
    if (typeid(*b) == typeid(NullLiteral)) {
        return static_cast<NullLiteral*>(b);
    }
    if (typeid(*b) == typeid(LocalVar)) {
        return static_cast<LocalVar*>(b);
    }
    if (typeid(*b) == typeid(Parameter)) {
        return static_cast<Parameter*>(b);
    }
    if (typeid(*b) == typeid(RuneLiteral)) {
        return static_cast<RuneLiteral*>(b);
    }
    if (typeid(*b) == typeid(StringLiteral)) {
        return static_cast<StringLiteral*>(b);
    }
    if (typeid(*b) == typeid(UnitLiteral)) {
        return static_cast<UnitLiteral*>(b);
    }
    CJC_ABORT_WITH_MSG("unknown Value " + b->ToString(0)); // throw exception
    return b;
}

bool FuncCodeEquality::areEqual(Value* first, Value* second)
{
    if (first == second) {
        return true;
    }

    if (const auto [_, inserted] = visitedValues.insert(first); !inserted) {
        return true;
    }

    const auto v1 = wrap(first);
    const auto v2 = wrap(second);

    if (v1.index() != v2.index()) {
        return false;
    }

    const auto areEqualExprs = [this](const std::vector<Expression*>& l, const std::vector<Expression*>& r) -> bool {
        return std::equal(l.begin(), l.end(), r.begin(), r.end(),
            [this](Expression* lVal, Expression* rVal) {
                return areEqual(lVal, rVal);
            });
    };

    const auto res = std::visit(overloaded{
        [](const BoolLiteral* l, const BoolLiteral* r) {
            return l->GetVal() == r->GetVal() &&
                l->GetAttributeInfo().GetRawAttrs() == r->GetAttributeInfo().GetRawAttrs();
        },
        [&](const Block* l, const Block* r) {
            return l->GetAttributeInfo().GetRawAttrs() == r->GetAttributeInfo().GetRawAttrs() &&
                areEqualExprs(l->GetExpressions(), r->GetExpressions());
        },
        [this](const BlockGroup* l, const BlockGroup* r) {
            return areEqual(l->GetBlocks(), r->GetBlocks());
        },
        [](const FloatLiteral* l, const FloatLiteral* r) {
            return l->GetVal() == r->GetVal() &&
                l->GetAttributeInfo().GetRawAttrs() == r->GetAttributeInfo().GetRawAttrs() &&
                PatchGenerator::areEqual(l->GetType(), r->GetType());
        },
        [](const Function* l, const Function* r) {
            return l->GetAttributeInfo().GetRawAttrs() == r->GetAttributeInfo().GetRawAttrs() &&
                PatchGenerator::areEqual(l, r);
        },
        [](const GlobalVar* l, const GlobalVar* r) {
            return l->GetPackageName() == r->GetPackageName() &&
                l->GetIdentifier() == r->GetIdentifier() &&
                l->GetSrcCodeIdentifier() == r->GetSrcCodeIdentifier() &&
                l->GetAttributeInfo().GetRawAttrs() == r->GetAttributeInfo().GetRawAttrs() &&
                PatchGenerator::areEqual(l->GetType(), r->GetType());
        },
        [](const IntLiteral* l, const IntLiteral* r) {
            return l->GetSignedVal() == r->GetSignedVal() &&
                l->GetAttributeInfo().GetRawAttrs() == r->GetAttributeInfo().GetRawAttrs() &&
                PatchGenerator::areEqual(l->GetType(), r->GetType());
        },
        [](const NullLiteral* l, const NullLiteral* r) {
            return l->GetAttributeInfo().GetRawAttrs() == r->GetAttributeInfo().GetRawAttrs() &&
                PatchGenerator::areEqual(l->GetType(), r->GetType());
        },
        [this](const LocalVar* l, const LocalVar* r) {
            return l->GetAttributeInfo().GetRawAttrs() == r->GetAttributeInfo().GetRawAttrs() &&
                l->GetIdentifier() == r->GetIdentifier() &&
                areEqual(l->GetExpr(), r->GetExpr()) &&
                PatchGenerator::areEqual(l->GetType(), r->GetType());
        },
        [this](const Parameter* l, const Parameter* r) {
            return l->GetIdentifier() == r->GetIdentifier() &&
                areEqual(l->GetOwnerFunc(), r->GetOwnerFunc()) &&
                areEqual(l->GetOwnerLambda(), r->GetOwnerLambda()) &&
                l->GetAttributeInfo().GetRawAttrs() == r->GetAttributeInfo().GetRawAttrs() &&
                PatchGenerator::areEqual(l->GetType(), r->GetType());
        },
        [](const RuneLiteral* l, const RuneLiteral* r) {
            return l->GetVal() == r->GetVal() &&
                l->GetAttributeInfo().GetRawAttrs() == r->GetAttributeInfo().GetRawAttrs();
        },
        [](const StringLiteral* l, const StringLiteral* r) {
            return l->GetVal() == r->GetVal() &&
                l->GetAttributeInfo().GetRawAttrs() == r->GetAttributeInfo().GetRawAttrs();
        },
        [](const UnitLiteral* l, const UnitLiteral* r) {
            return PatchGenerator::areEqual(l->GetType(), r->GetType()) &&
                l->GetAttributeInfo().GetRawAttrs() == r->GetAttributeInfo().GetRawAttrs();
        },
        [](const Value* l, const Value* r) {
            return l->GetAttributeInfo().GetRawAttrs() == r->GetAttributeInfo().GetRawAttrs() &&
                PatchGenerator::areEqual(l->GetType(), r->GetType());
        }
    }, v1, v2);
#if DEBUG
    if (!res) {
        std::cout << "Values differ" << std::endl;
        std::cout << "first " << std::endl;
        std::cout << first->ToString(0) << std::endl;
        std::cout << "second " << std::endl;
        std::cout << second->ToString(0) << std::endl;
    }
#endif
    return res;
}

// endregion Value

// region Expression
using ConcreteExpr = std::variant<
    AllocateBase*,
    Apply*,
    BinaryExpressionBase*,
    Branch*,
    Debug*,
    DynamicDispatch*,
    Field*,
    FieldByName*,
    GetElementByName*,
    GetElementRef*,
    GetInstantiateValue*,
    GetRTTIStatic*,
    InstanceOf*,
    IntrinsicBase*,
    Lambda*,
    MultiBranch*,
    NumericCastBase*,
    RawArrayAllocateBase*,
    SpawnBase*,
    StoreElementByName*,
    StoreElementRef*,
    UnaryExpressionBase*,
    Expression*>;

ConcreteExpr wrap(Expression* b)
{
    if (!b) {
        return static_cast<Expression*>(nullptr);
    }
    if (typeid(*b) == typeid(Allocate)) {
        return static_cast<AllocateBase*>(b);
    }
    if (typeid(*b) == typeid(Apply)) {
        return static_cast<ApplyBase*>(b);
    }
    if (typeid(*b) == typeid(BinaryExpression)) {
        return static_cast<BinaryExpressionBase*>(b);
    }
    if (typeid(*b) == typeid(Box)) {
        return b;
    }
    if (typeid(*b) == typeid(Branch)) {
        return b;
    }
    if (typeid(*b) == typeid(CastToConcrete)) {
        return b;
    }
    if (typeid(*b) == typeid(CastToGeneric)) {
        return b;
    }
    if (typeid(*b) == typeid(ClassStaticCast)) {
        return b;
    }
    if (typeid(*b) == typeid(Constant)) {
        return b;
    }
    if (typeid(*b) == typeid(Debug)) {
        return b;
    }
    if (typeid(*b) == typeid(Exit)) {
        return b;
    }
    if (typeid(*b) == typeid(Field)) {
        return b;
    }
    if (typeid(*b) == typeid(FieldByName)) {
        return b;
    }
    if (typeid(*b) == typeid(ForInClosedRange)) {
        return b;
    }
    if (typeid(*b) == typeid(ForInIter)) {
        return b;
    }
    if (typeid(*b) == typeid(ForInRange)) {
        return b;
    }
    if (typeid(*b) == typeid(GetElementByName)) {
        return b;
    }
    if (typeid(*b) == typeid(GetElementRef)) {
        return b;
    }
    if (typeid(*b) == typeid(GetException)) {
        return b;
    }
    if (typeid(*b) == typeid(GetInstantiateValue)) {
        return b;
    }
    if (typeid(*b) == typeid(GetRTTI)) {
        return b;
    }
    if (typeid(*b) == typeid(GetRTTIStatic)) {
        return b;
    }
    if (typeid(*b) == typeid(GoTo)) {
        return b;
    }
    if (typeid(*b) == typeid(InstanceOf)) {
        return b;
    }
    if (typeid(*b) == typeid(Intrinsic)) {
        return static_cast<IntrinsicBase*>(b);
    }
    if (typeid(*b) == typeid(Invoke)) {
        return static_cast<DynamicDispatch*>(b);
    }
    if (typeid(*b) == typeid(InvokeStatic)) {
        return static_cast<DynamicDispatch*>(b);
    }
    if (typeid(*b) == typeid(Lambda)) {
        return static_cast<Lambda*>(b);
    }
    if (typeid(*b) == typeid(Load)) {
        return b;
    }
    if (typeid(*b) == typeid(MultiBranch)) {
        return static_cast<MultiBranch*>(b);
    }
    if (typeid(*b) == typeid(NumericCast)) {
        return static_cast<NumericCastBase*>(b);
    }
    if (typeid(*b) == typeid(RaiseException)) {
        return b;
    }
    if (typeid(*b) == typeid(RawArrayAllocate)) {
        return static_cast<RawArrayAllocateBase*>(b);
    }
    if (typeid(*b) == typeid(RawArrayInitByValue)) {
        return b;
    }
    if (typeid(*b) == typeid(RawArrayLiteralInit)) {
        return b;
    }
    if (typeid(*b) == typeid(Spawn)) {
        return static_cast<SpawnBase*>(b);
    }
    if (typeid(*b) == typeid(Store)) {
        return b;
    }
    if (typeid(*b) == typeid(StoreElementByName)) {
        return b;
    }
    if (typeid(*b) == typeid(StoreElementRef)) {
        return b;
    }
    if (typeid(*b) == typeid(TryAllocate)) {
        return static_cast<AllocateBase*>(b);
    }
    if (typeid(*b) == typeid(TryApply)) {
        return static_cast<DynamicDispatch*>(b);
    }
    if (typeid(*b) == typeid(TryBinaryExpression)) {
        return static_cast<BinaryExpressionBase*>(b);
    }
    if (typeid(*b) == typeid(TryIntrinsic)) {
        return static_cast<IntrinsicBase*>(b);
    }
    if (typeid(*b) == typeid(TryInvoke)) {
        return static_cast<DynamicDispatch*>(b);
    }
    if (typeid(*b) == typeid(TryInvokeStatic)) {
        return static_cast<DynamicDispatch*>(b);
    }
    if (typeid(*b) == typeid(TryNumericCast)) {
        return static_cast<NumericCastBase*>(b);
    }
    if (typeid(*b) == typeid(TryRawArrayAllocate)) {
        return static_cast<RawArrayAllocateBase*>(b);
    }
    if (typeid(*b) == typeid(TrySpawn)) {
        return static_cast<SpawnBase*>(b);
    }
    if (typeid(*b) == typeid(TryUnaryExpression)) {
        return static_cast<UnaryExpressionBase*>(b);
    }
    if (typeid(*b) == typeid(Tuple)) {
        return b;
    }
    if (typeid(*b) == typeid(UnBoxToRef)) {
        return b;
    }
    if (typeid(*b) == typeid(UnBoxToValue)) {
        return b;
    }
    if (typeid(*b) == typeid(UnaryExpression)) {
        return static_cast<UnaryExpressionBase*>(b);
    }
    if (typeid(*b) == typeid(VArray)) {
        return b;
    }
    if (typeid(*b) == typeid(VArrayBuilder)) {
        return b;
    }
    CJC_ABORT_WITH_MSG("unknown Expression " + b->ToString(0)); // throw exception
    return b;
}

bool FuncCodeEquality::areEqual(Expression* first, Expression* second)
{
    if (first == second) {
        return true;
    }

    if (first->GetExprKind() != second->GetExprKind()) {
        return false;
    }

    const auto v1 = wrap(first);
    const auto v2 = wrap(second);

    if (v1.index() != v2.index()) {
        return false;
    }

    const auto areEqualValues = [this](const std::vector<Value*>& l, const std::vector<Value*>& r) -> bool {
        return std::equal(l.begin(), l.end(), r.begin(), r.end(),
            [this](Value* lVal, Value* rVal) {
                return areEqual(lVal, rVal);
            });
    };
    if (!areEqualValues(first->GetOperands(), second->GetOperands())) {
        return false;
    }

    const auto areEqualsFuncCalls = [](const FuncCall* l, const FuncCall* r) {
            return PatchGenerator::areEqual(l->GetThisType(), r->GetThisType()) &&
            PatchGenerator::areEqual(l->GetInstantiatedTypeArgs(), r->GetInstantiatedTypeArgs());
    };

    const auto areEqualsDynamicDispatches = [&](const DynamicDispatch* l, const DynamicDispatch* r) {
        // checking overflowStrategy as one has no setter
        return l->GetMethodName() == r->GetMethodName() && areEqualsFuncCalls(l, r);
    };

    const auto res = std::visit(overloaded{
        [](const AllocateBase* l, const AllocateBase* r) {
            return PatchGenerator::areEqual(l->GetType(), r->GetType());
        },
        [&](Apply* l, const Apply* r) {
            return l->IsSuperCall() == r->IsSuperCall() &&
                areEqualsDynamicDispatches(dynamic_cast<DynamicDispatch*>(l), dynamic_cast<DynamicDispatch*>(l));
        },
        [](const BinaryExpressionBase* l, const BinaryExpressionBase* r) {
            return l->GetOverflowStrategy() == r->GetOverflowStrategy();
        },
        [&](const Branch* l, const Branch* r) {
            return l->GetSourceExpr() == r->GetSourceExpr();
        },
        [](const Debug* l, const Debug* r) {
            return l->GetSrcCodeIdentifier() == r->GetSrcCodeIdentifier();
        },
        [&](const DynamicDispatch* l, const DynamicDispatch* r) {
            return areEqualsDynamicDispatches(l, r);
        },
        [](const Field* l, const Field* r) {
            return l->GetPath() == r->GetPath();
        },
        [](const FieldByName* l, const FieldByName* r) {
            return l->GetNames() == r->GetNames();
        },
        [](const GetElementByName* l, const GetElementByName* r) {
            return l->GetNames() == r->GetNames();
        },
        [](const GetElementRef* l, const GetElementRef* r) {
            return l->GetPath() == r->GetPath();
        },
        [](const GetInstantiateValue* l, const GetInstantiateValue* r) {
            return PatchGenerator::areEqual(l->GetInstantiateTypes(), r->GetInstantiateTypes());
        },
        [](const GetRTTIStatic* l, const GetRTTIStatic* r) {
            return PatchGenerator::areEqual(l->GetRTTIType(), r->GetRTTIType());
        },
        [](const InstanceOf* l, const InstanceOf* r) {
            return PatchGenerator::areEqual(l->GetType(), r->GetType());
        },
        [&](const IntrinsicBase* l, const IntrinsicBase* r) {
            return l->GetIntrinsicKind() == r->GetIntrinsicKind() && areEqualsFuncCalls(l, r);
        },
        [](const Lambda* l, const Lambda* r) {
            return l->IsLocalFunc() == r->IsLocalFunc() &&
                l->GetIdentifier() == r->GetIdentifier() &&
                l->GetSrcCodeIdentifier() == r->GetSrcCodeIdentifier() &&
                PatchGenerator::areEqual(l->GetFuncType(), r->GetFuncType()) &&
                PatchGenerator::areEqual(l->GetGenericTypeParams(), r->GetGenericTypeParams());
        },
        [](const MultiBranch* l, const MultiBranch* r) {
            return l->GetCaseVals() == r->GetCaseVals();
        },
        [](const NumericCastBase* l, const NumericCastBase* r) {
            return l->GetOverflowStrategy() == r->GetOverflowStrategy();
        },
        [](const RawArrayAllocateBase* l, const RawArrayAllocateBase* r) {
            return PatchGenerator::areEqual(l->GetElementType(), r->GetElementType());
        },
        [](const SpawnBase* l, const SpawnBase* r) {
            return PatchGenerator::areEqual(l->GetExecuteClosure(), r->GetExecuteClosure());
        },
        [](const StoreElementByName* l, const StoreElementByName* r) {
            return l->GetNames() == r->GetNames();
        },
        [](const StoreElementRef* l, const StoreElementRef* r) {
            return l->GetPath() == r->GetPath();
        },
        [](const UnaryExpressionBase* l, const UnaryExpressionBase* r) {
            return l->GetOverflowStrategy() == r->GetOverflowStrategy();
        },
        [&](const Expression*, const Expression*) {
            return true;
        },
    }, v1, v2);
#if DEBUG
    if (!res) {
        std::cout << "Exprs differ" << std::endl;
        std::cout << "first " << std::endl;
        std::cout << first->ToString(0) << std::endl;
        std::cout << "second " << std::endl;
        std::cout << second->ToString(0) << std::endl;
    }
#endif
    return res;
}

bool FuncCodeEquality::areEqual(const std::vector<Block*>& l, const std::vector<Block*>& r)
{
    return std::equal(l.begin(), l.end(), r.begin(), r.end(),
        [this](Block* lVal, Block* rVal) {
            return areEqual(lVal, rVal);
        });
};

// endregion Expression

// region Equality & Hasher

bool CustomDefEquality::operator()(const CustomTypeDef* l, const CustomTypeDef* r) const
{
    return l == r || (
        l->GetCustomKind() == r->GetCustomKind() &&
        l->GetPackageName() == r->GetPackageName() &&
        l->GetIdentifier() == r->GetIdentifier() &&
        l->GetSrcCodeIdentifier() == r->GetSrcCodeIdentifier());
}

size_t CustomDefHasher::operator()(const CustomTypeDef* cl) const
{
    if (!cl) {
        return 0;
    }
    size_t seed = 0;
    Cangjie::hash_combine(seed, cl->GetPackageName());
    Cangjie::hash_combine(seed, cl->GetIdentifier());
    Cangjie::hash_combine(seed, cl->GetSrcCodeIdentifier());
    return seed;
}

bool FunctionEquality::operator()(const Function* l, const Function* r) const
{
    return l == r || (l->IsFuncWithBody() == r->IsFuncWithBody() &&
        l->IsImportedFunc() == r->IsImportedFunc() &&
        l->GetPackageName() == r->GetPackageName() &&
        l->GetIdentifier() == r->GetIdentifier() &&
        l->GetSrcCodeIdentifier() == r->GetSrcCodeIdentifier() &&
        areEqual(l->GetFuncType(), r->GetFuncType()) &&
        areEqual(l->GetParentCustomTypeDef(), r->GetParentCustomTypeDef()));
}

size_t FunctionHasher::operator()(const Function* func) const
{
    if (!func) {
        return 0;
    }
    size_t seed = 0;
    Cangjie::hash_combine(seed, func->IsFuncWithBody());
    Cangjie::hash_combine(seed, func->IsImportedFunc());
    Cangjie::hash_combine(seed, func->GetPackageName());
    Cangjie::hash_combine(seed, func->GetIdentifier());
    Cangjie::hash_combine(seed, func->GetSrcCodeIdentifier());
    return seed;
}

bool GlobalVarEquality::operator()(const GlobalVar* l, const GlobalVar* r) const
{
    return l == r || (l->GetIdentifier() == r->GetIdentifier() &&
        l->GetSrcCodeIdentifier() == r->GetSrcCodeIdentifier() &&
            l->GetPackageName() == r->GetPackageName());
}

size_t GlobalVarHasher::operator()(const GlobalVar* var) const
{
    if (!var) {
        return 0;
    }
    size_t seed = 0;
    Cangjie::hash_combine(seed, var->GetIdentifier());
    Cangjie::hash_combine(seed, var->GetSrcCodeIdentifier());
    Cangjie::hash_combine(seed, var->GetPackageName());
    return seed;
}

// endregion Equality & Hasher
}