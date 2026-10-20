// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
func recover<T>(value: Any): ?T { value as T }
main(): Int64 {
    if (recover<Int64>(7).getOrThrow() != 7 || recover<String>("x").getOrThrow() != "x" || recover<Int64>("x").isSome()) { return 1 }
    println("generic_recovery passed")
    0
}
