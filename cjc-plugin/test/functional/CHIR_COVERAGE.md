# Standalone CHIR variation coverage

The listed `.cj` cases replace the previous language-feature selection. They are
selected/adapted from LLT/compiler/CHIR (plus the requested global-constant
optimization case). Tests sharing syntax but differing in receiver representation,
generic signature, dispatch, boxing, or exceptional control flow remain separate.
Closely related regression duplicates and repeated type-width cases are omitted.

Each file has `main()`, explicitly annotated global functions and declaring
types, deterministic output, and a nonempty
`.cj.expected`. The entry wrapper checks the original result before printing its
success marker. LLT directives/diagnostic comments are removed; license notices
are preserved. No new stub files are generated.

The matrix names intended CHIR variations. Optimizations can eliminate or rewrite
nodes, so source coverage alone does not establish that every IR opcode survives
every compiler stage. Multi-package imports/linkage, external native libraries,
custom macro packages, diagnostics, debug/serialization-only and compiler-pass
infrastructure cases are outside this standalone functional-test scope.

| Test | Distinct CHIR variation | LLT source | Conversion |
| --- | --- | --- | --- |
| `apply_struct.cj` | Apply: concrete struct constructor, static and instance receiver | `testsuites/LLT/compiler/CHIR/UG/CallExpr/apply_like_03.cj` | Original exercise; checked output wrapper |
| `apply_class.cj` | Apply: concrete class instance receiver; optional/Int32 parameters and Float64 static call | `testsuites/LLT/compiler/CHIR/Class/testClass04.cj`; `cjc-plugin/test/functional/class.cj (folded into this case)` | Adapted/activated with runtime checks |
| `apply_generic_struct.cj` | Apply: generic method on value receiver; mixed scalar/Option/reference fields and methods | `testsuites/LLT/compiler/CHIR/Struct/testStruct04.cj`; `cjc-plugin/test/functional/struct.cj (folded into this case)` | Adapted/activated with runtime checks |
| `apply_generic_class.cj` | Apply: generic method on reference receiver | `testsuites/LLT/compiler/CHIR/Class/testClass05.cj` | Original exercise; checked output wrapper |
| `apply_constrained.cj` | Apply: generic receiver with class bound | `testsuites/LLT/compiler/CHIR/AST2CHIR/TranslateGenericDecl/OneGenericParam/typeT_apply.cj` | Original exercise; checked output wrapper |
| `invoke_constrained.cj` | Invoke: overridden method through generic class bound | `testsuites/LLT/compiler/CHIR/AST2CHIR/TranslateGenericDecl/OneGenericParam/typeT_invoke.cj` | Original exercise; checked output wrapper |
| `invoke_static.cj` | InvokeStatic: generic static dispatch through class bound | `testsuites/LLT/compiler/CHIR/InvokeStatic/abstractclass.cj` | Original exercise; checked output wrapper |
| `invoke_static_instance_context.cj` | InvokeStatic: inherited static lookup from instance and static methods | `testsuites/LLT/compiler/CHIR/InvokeStatic/callstatic_ininstance.cj` | Original exercise; checked output wrapper |
| `invoke_static_closure.cj` | InvokeStatic in a closure capturing type arguments | `testsuites/LLT/compiler/CHIR/InvokeStatic/ongeneric.cj` | Original exercise; checked output wrapper |
| `generic_property.cj` | Invoke property getters through generic interface bound and primitive extension | `testsuites/LLT/compiler/CHIR/Generic/testGeneric09.cj` | Original exercise; checked output wrapper |
| `generic_static_function_value.cj` | GetInstantiateValue and closure wrapper for constrained static method | `testsuites/LLT/compiler/CHIR/Generic/testGeneric16.cj` | Adapted/activated with runtime checks |
| `generic_instance_function_value.cj` | Bound method closure for generic interface receiver | `testsuites/LLT/compiler/CHIR/Generic/testGeneric17.cj` | Adapted/activated with runtime checks |
| `open_generic_class.cj` | Virtual generic methods: overridden method, abstract method, type bounds | `testsuites/LLT/compiler/CHIR/UG/OpenGeneric/basic.cj` | Original exercise; checked output wrapper |
| `open_generic_interface.cj` | Virtual generic methods: class and struct implementations through interface | `testsuites/LLT/compiler/CHIR/UG/OpenGeneric/inter.cj` | Original exercise; checked output wrapper |
| `open_generic_extension.cj` | Virtual generic methods supplied by class/struct extensions | `testsuites/LLT/compiler/CHIR/UG/OpenGeneric/extend.cj` | Original exercise; checked output wrapper |
| `open_generic_mut_extension.cj` | Generic mut extension methods and interface receiver wrappers | `testsuites/LLT/compiler/CHIR/UG/OpenGeneric/mut.cj` | Original exercise; checked output wrapper |
| `generic_variant_function.cj` | Generic bound method with covariant return and function signature adaptation | `testsuites/LLT/compiler/CHIR/UG/OpenGeneric/lambda.cj` | Original exercise; checked output wrapper |
| `extension_static_value.cj` | Primitive static extension method used as a function value | `testsuites/LLT/compiler/CHIR/Extend/testExtend35.cj` | Original exercise; checked output wrapper |
| `extension_interface.cj` | Class extension conformance, Invoke through interface argument | `testsuites/LLT/compiler/CHIR/Extend/testExtend02.cj` | Original exercise; checked output wrapper |
| `extension_default.cj` | Default interface method supplied through extension | `testsuites/LLT/compiler/CHIR/Extend/testExtend04.cj` | Original exercise; checked output wrapper |
| `extension_alias.cj` | Extension of aliased class type | `testsuites/LLT/compiler/CHIR/Extend/testExtend03.cj` | Original exercise; checked output wrapper |
| `generic_extension.cj` | Generic extension method modifies class field | `testsuites/LLT/compiler/CHIR/AST2CHIR/TranslateGenericDecl/OneGenericParam/extend_T.cj` | Original exercise; checked output wrapper |
| `box_struct_interface.cj` | Box struct as interface; InstanceOf and UnBoxToValue in type pattern | `testsuites/LLT/compiler/CHIR/UG/Box/box03.cj` | Adapted/activated with runtime checks |
| `box_generic_struct.cj` | CastToGeneric and UnBoxToValue for struct generic argument | `testsuites/LLT/compiler/CHIR/UG/Box/box02.cj` | Adapted/activated with runtime checks |
| `box_generic_class.cj` | CastToGeneric and reference recovery for class generic argument | `testsuites/LLT/compiler/CHIR/UG/Box/box05.cj` | Adapted/activated with runtime checks |
| `box_recursive_enum.cj` | Recursive enum/struct payload representation, discriminant and payload extraction | `testsuites/LLT/compiler/CHIR/UG/RecursionEnum/recursionEnum04.cj` | Original exercise; checked output wrapper |
| `generic_nested_enum.cj` | Generic enum with nested generic class payload and multi-field alternative | `testsuites/LLT/compiler/CHIR/UG/TypeCast/typecast07.cj` | Adapted/activated with runtime checks |
| `generic_static_value.cj` | Generic static field: primitive initializer and GetInstantiateValue | `testsuites/LLT/compiler/CHIR/UG/StaticVar/static_var_01.cj` | Original exercise; checked output wrapper |
| `generic_static_reference.cj` | Generic static field: reference initializer and field load | `testsuites/LLT/compiler/CHIR/UG/StaticVar/static_var_02.cj` | Original exercise; checked output wrapper |
| `global_constant.cj` | Global let versus global var, constant propagation candidate | `testsuites/LLT/compiler/CHIROpt/Optimizations/GlobalConstVars/testGlobalConstVar01.cj` | Original exercise; checked output wrapper |
| `mutable_struct.cj` | Value receiver copy and field store independence | `testsuites/LLT/compiler/CHIR/MutableSemantics/testMutableRecordField.cj` | Original exercise; checked output wrapper |
| `mutable_class.cj` | Reference assignment aliasing and mutation through argument | `testsuites/LLT/compiler/CHIR/MutableSemantics/testMutableClass.cj` | Original exercise; checked output wrapper |
| `mutable_array.cj` | Array representation sharing and element mutation through argument | `testsuites/LLT/compiler/CHIR/MutableSemantics/testMutableArray.cj` | Original exercise; checked output wrapper |
| `mut_receiver.cj` | Mut struct receiver and nested reference field mutation | `testsuites/LLT/compiler/CHIR/MutableSemantics/testMutableInMethod.cj` | Original exercise; checked output wrapper |
| `nested_struct_store.cj` | GetElementRef and StoreElementRef through nested value aggregates | `testsuites/LLT/compiler/CHIR/Assign/nestStruct.cj` | Original exercise; checked output wrapper |
| `nested_class_store.cj` | Load class references and compound store through field chain | `testsuites/LLT/compiler/CHIR/Assign/testMemberAccessAsg02.cj` | Original exercise; checked output wrapper |
| `mut_property.cj` | Mut struct getter/setter receiver, property lowered to calls | `testsuites/LLT/compiler/CHIR/Struct/testStruct05.cj` | Original exercise; checked output wrapper |
| `nested_struct_increment.cj` | Nested value field increment and writeback | `testsuites/LLT/compiler/CHIR/IncOrDecExpr/testIncOrDecExpr02.cj` | Original exercise; checked output wrapper |
| `boxed_mutable_capture.cj` | Boxed local primitive captured by recursive local function | `testsuites/LLT/compiler/CHIR/BoxingLocalMutVar/mutVarCapturedByLocalFunc01.cj` | Original exercise; checked output wrapper |
| `boxed_reference_capture.cj` | Boxed local reference rebound inside recursive closure | `testsuites/LLT/compiler/CHIR/BoxingLocalMutVar/varWithTypeClass_box.cj` | Original exercise; checked output wrapper |
| `bound_struct_method.cj` | Boxed struct receiver in bound default-interface function value | `testsuites/LLT/compiler/CHIR/AST2CHIR/Apply/boxin_lambda.cj` | Original exercise; checked output wrapper |
| `lambda_nothing_wrapper.cj` | Lambda wrapper with Nothing parameter signature conversion | `testsuites/LLT/compiler/CHIR/LambdaWrapper/lambdaWrapper02.cj` | Original exercise; checked output wrapper |
| `higher_order_wrapper.cj` | Function/lambda wrapper returning another function | `testsuites/LLT/compiler/CHIR/LambdaWrapper/lambdaWrapper08.cj` | Original exercise; checked output wrapper |
| `pattern_interface_tuple.cj` | Tuple Field, interface InstanceOf and Invoke under pattern guard | `testsuites/LLT/compiler/CHIR/PatternMatch/testMatchWithBox01.cj` | Original exercise; checked output wrapper |
| `varray_writeback.cj` | VArray literal, indexed store, ordered index evaluation | `testsuites/LLT/compiler/CHIR/VArray/varray02.cj` | Original exercise; checked output wrapper |
| `varray_nested_repeat.cj` | Nested VArray repeat construction and element extraction | `testsuites/LLT/compiler/CHIR/VArray/varray01.cj` | Original exercise; checked output wrapper |
| `unit_any.cj` | Unit boxed to Any at match merge, mixed value branch | `testsuites/LLT/compiler/CHIR/AST2CHIR/Unit/anyunit_switch.cj` | Original exercise; checked output wrapper |
| `generic_aggregate_fields.cj` | Generic class/struct/tuple fields: concrete value and reference type arguments | `testsuites/LLT/compiler/CHIR/AST2CHIR/TranslateGenericDecl/EmbeddedGenericParam/T_is_generic_struct.cj` | Adapted/activated with runtime checks |
| `generic_mut_receiver.cj` | UnBoxToRef and writeback for constrained mut generic value receiver | `testsuites/LLT/compiler/CHIR/UG/OpenGeneric/mut.cj` | Adapted/activated with runtime checks |
| `extension_constrained.cj` | Conditional generic extension conformance and interface dispatch | `testsuites/LLT/compiler/CHIR/UG/Box/box18.cj` | Adapted/activated with runtime checks |
| `try_apply.cj` | TryApply, RaiseException, GetException, catch and finally edges | `testsuites/LLT/compiler/CHIR/Exception/testTryCatch01.cj` | Adapted/activated with runtime checks |
| `try_invoke.cj` | TryInvoke through interface, normal and exceptional successors | `testsuites/LLT/compiler/CHIR/InvokeNodetest/testDevirt01.cj` | Adapted/activated with runtime checks |
| `try_invoke_static.cj` | TryInvokeStatic with generic static interface constraint | `testsuites/LLT/compiler/CHIR/InvokeStatic/ongeneric.cj` | Adapted/activated with runtime checks |
| `try_numeric_cast.cj` | NumericCast and TryNumericCast with overflow catch | `testsuites/LLT/compiler/CHIR/AST2CHIR/Typecast/exception.cj` | Adapted/activated with runtime checks |
| `try_arithmetic.cj` | TryNeg/Add/Sub/Mul/Div/Mod/Exp/LShift/RShift and overflow policy attributes | `testsuites/LLT/compiler/CHIR/SplitOperator/ovf_mul.cj` | Adapted/activated with runtime checks |
| `array_construction.cj` | RawArrayAllocate, RawArrayLiteralInit, RawArrayInitByValue and array builder lambda | `testsuites/LLT/compiler/CHIR/ScalarReplacementOfArray/normal01.cj` | Adapted/activated with runtime checks |
| `varray_builder.cj` | VArrayBuilder with captured variable and fixed size aggregate result | `testsuites/LLT/compiler/CHIR/VArray/varray01.cj` | Adapted/activated with runtime checks |
| `try_allocate_intrinsic.cj` | TryAllocate, TryRawArrayAllocate and TryIntrinsic array access under handler | `testsuites/LLT/compiler/CHIR/MemAccessExpr/arrayIntrinsic.cj` | Adapted/activated with runtime checks |
| `spawn.cj` | Spawn with captured scalar and Future result | `testsuites/LLT/compiler/CHIR/CallExpr/spawn.cj` | Adapted/activated with runtime checks |
| `try_spawn.cj` | TrySpawn and exception propagation through Future.get | `testsuites/LLT/compiler/CHIR/UG/ClosureConversion/spawn01.cj` | Adapted/activated with runtime checks |
| `loop_lowering.cj` | ForInRange, ForInClosedRange, ForInIter, Branch/GoTo and loop exits | `testsuites/LLT/compiler/CHIR/AST2CHIR/forin/ham.cj` | Adapted/activated with runtime checks |
| `multibranch.cj` | MultiBranch lowering for constant and enum match selectors | `testsuites/LLT/compiler/CHIR/Enum/enumIndex.cj` | Adapted/activated with runtime checks |
| `casts_rtti.cj` | InstanceOf, ClassStaticCast, RTTI on concrete and generic class/struct | `testsuites/LLT/compiler/CHIR/UG/InstanceOf/isExpr1.cj` | Adapted/activated with runtime checks |
| `cfunc_signature.cj` | CFunc wrapper, C struct value ABI and unsafe Apply | `testsuites/LLT/compiler/CHIR/CallExpr/nativeCall.cj` | Adapted/activated with runtime checks |
| `generic_global_function_value.cj` | GetInstantiateValue for concrete and enclosing generic type arguments | `testsuites/LLT/compiler/CHIR/UG/ClosureConversion/globalFunc01.cj` | Original exercise; checked output wrapper |
| `generic_nested_closure.cj` | Lambda environment with enclosing generic types and generic nominal result | `testsuites/LLT/compiler/CHIR/UG/ClosureConversion/nestedFunc01.cj` | Original exercise; checked output wrapper |
| `generic_extension_function_value.cj` | Static generic extension function value, inherited defaults, overloaded interface bounds | `testsuites/LLT/compiler/CHIR/UG/ClosureConversion/extendFunc01.cj` | Original exercise; checked output wrapper |
| `vtable_value_parameter_wrapper.cj` | Virtual signature wrapper: generic parameter specialized to struct | `testsuites/LLT/compiler/CHIR/UG/VTable/same_signature_wrapper/apply01.cj` | Adapted/activated with runtime checks |
| `vtable_value_return_wrapper.cj` | Virtual signature wrapper: generic return specialized to struct | `testsuites/LLT/compiler/CHIR/UG/VTable/same_signature_wrapper/apply02.cj` | Adapted/activated with runtime checks |
| `vtable_multiple_parents.cj` | Class method fulfills interface slot inherited alongside superclass | `testsuites/LLT/compiler/CHIR/UG/VTable/non_generic/multi_parent01.cj` | Adapted/activated with runtime checks |
| `local_const_function.cj` | Const local function translated inside static initializer closure | `testsuites/LLT/compiler/CHIR/UG/LocalConstDecl/constFunc01.cj` | Original exercise; checked output wrapper |
| `unary_binary_nodes.cj` | Neg/Not/BitNot, bitwise operators, comparisons, boolean short circuit | `testsuites/LLT/compiler/CHIR/AST2CHIR/BaseUseCase/binaryExpr/binaryExpr01.cj` | Adapted/activated with runtime checks |
| `apply_global.cj` | Apply: global Unit functions with Int32 and Float32 arguments | `cjc-plugin/test/functional/global.cj (folded into this case)` | Adapted/activated with runtime checks |

## Patchability boundaries

`@patchable` on a declaring type also selects its constructors and property
accessors. Global functions and functions in extension blocks are explicitly annotated;
nominal declarations carry the annotation for all their methods. Cangjie forbids custom
annotations on `main()` and local functions; the runner selects them via its
patch-all configuration.
The standard normal runner additionally builds with `HOTFIX_PATCH_ALL=true`,
which selects eligible generated helpers and lifted local functions. Cangjie
forbids custom annotations on local function declarations.

The plugin skips initializers, annotation factories, struct constructors and
`mut` methods in `src/patchable_finder.cj`, even if annotated. Mut tests therefore
cover CHIR receiver lowering and calls from patchable functions; they do not
assert that the plugin instruments mut methods. Supporting that requires a
separate plugin change.

## Original test consolidation

`class.cj` was folded into `apply_class.cj`; `struct.cj` into
`apply_generic_struct.cj`; `global.cj` into `apply_global.cj`. Their old source and
expectation files were removed, including associated obsolete stub expectations.
The mixed parameter/field representations and floating point calls were preserved.

`classCtor.cj` and `structCtor.cj` retain distinct constructor delegation/patch
fixtures. `package_init.cj` retains named-package initialization coverage.
`complex.cj` retains large nested VArray/container/concurrency stress coverage.
Those four existing tests and their existing support files remain.

Validation results and the complete per-test status matrix: [CHIR_TEST_RESULTS.md](CHIR_TEST_RESULTS.md).
