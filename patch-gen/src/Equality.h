#ifndef EQUALITY_H
#define EQUALITY_H
#include "cangjie/CHIR/IR/CHIRBuilder.h"

namespace Cangjie::CHIR {
bool operator==(const Package& l, const Package& r);
}

namespace PatchGenerator {

using namespace Cangjie::CHIR;

bool areEqual(const CustomTypeDef* l, const CustomTypeDef* r);

bool areEqual(const Function* l, const Function* r);

bool areEqual(Type* l, Type* r);

bool areEqual(const std::vector<Type*>& l, const std::vector<Type*>& r);

bool areEqual(const std::vector<GenericType*>& l, const std::vector<GenericType*>& r);

class FuncCodeEquality {
public:
    bool areEqual(Value* first, Value* second);

private:
    std::unordered_set<Value*> visitedValues;

    bool areEqual(const std::vector<Block*>& l, const std::vector<Block*>& r);
    bool areEqual(Expression* first, Expression* second);
};

struct CustomDefEquality {
    bool operator()(const CustomTypeDef* l, const CustomTypeDef* r) const;
};

struct CustomDefHasher {
    size_t operator()(const CustomTypeDef* cl) const;
};

struct FunctionEquality {
    bool operator()(const Function* l, const Function* r) const;
};

struct FunctionHasher {
    size_t operator()(const Function* func) const;
};

struct GlobalVarEquality {
    bool operator()(const GlobalVar* l, const GlobalVar* r) const;
};

struct GlobalVarHasher {
    size_t operator()(const GlobalVar* var) const;
};
}

#endif // EQUALITY_H