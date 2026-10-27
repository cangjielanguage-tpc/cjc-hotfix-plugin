/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026. All rights reserved.
 * This source file is part of the Cangjie project, licensed under Apache-2.0
 * with Runtime Library Exception.
 *
 * See https://cangjie-lang.cn/pages/LICENSE for license information.
 */

/*
 * ABI compat shim for the CHIR v13008 format (compiler v1.3.0-alpha.08+).
 *
 * The complete-object constructor (the `C1` mangling,
 * `_ZN7Cangjie4CHIR10ExpressionC1E...`) of the out-of-line
 * `Expression::Expression(ExprKind, const std::vector<Value*>&,
 * const std::vector<BlockGroup*>&, Block*)` is no longer exported by
 * `libcangjie-lsp.so` in this compiler version, while every inline
 * delegating constructor in the SDK headers (e.g. the 3-parameter
 * `Expression`, and consequently `Store`, `Apply`, ... created via
 * `CHIRBuilder::CreateExpression<T>`) is required by the Itanium C++ ABI
 * to call exactly the `C1` variant. Any TU compiled with GCC therefore
 * carries an undefined `C1` reference and the link of patch-gen fails.
 *
 * Defining the constructor out-of-line here emits the full `C1`/`C2` pair
 * as strong symbols inside the patch-gen executable, so all references
 * from patch-gen TUs bind to this copy (executable definitions take
 * precedence over the shared library at static-link time). For a class
 * without virtual bases `C1` and `C2` are semantically identical, and the
 * library keeps using its own internal copy, so the duplication is safe.
 *
 * The body mirrors the upstream implementation in
 * `src/CHIR/IR/Expression/Expression.cpp` and must be kept in sync with it.
 */

#include "cangjie/CHIR/IR/Expression/Expression.h"
#include "cangjie/CHIR/IR/Value/Value.h"
#include "cangjie/Utils/CheckUtils.h"

#include <vector>

namespace Cangjie::CHIR {

Expression::Expression(
    ExprKind kind, const std::vector<Value*>& operands, const std::vector<BlockGroup*>& blockGroups, Block* parent)
    : kind(kind), operands(operands), blockGroups(blockGroups), parent(parent)
{
    CJC_NULLPTR_CHECK(parent);
    for (auto op : operands) {
        // note: a hack here, remove later
        if (op == nullptr) {
            continue;
        }
        op->AddUserOnly(this);
    }
    for (auto blockGroup : blockGroups) {
        blockGroup->AddUserOnly(this);
    }
}

} // namespace Cangjie::CHIR
