// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

import hotfix.patchable

@patchable
public abstract class Event {
  public static func getKey(): String { "Event" }
}

@patchable
public open class SubEvent <: Event {
  public static func getKey(): String { "SubEvent" }
}

@patchable
public class SSubEvent <: SubEvent {
  public static func getKey(): String { "SSubEvent" }
}

@patchable
public func getKey<T>(event: T): String where T <: Event { T.getKey() }

@patchable
func exercise() {
    println(getKey(SSubEvent()))
}

main(): Int64 {
    exercise()
    println("invoke_static passed")
    0
}
