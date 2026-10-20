# CHIR functional test results

Toolchain: `/home/s00827109/Projects/software/hotfix-20.10`.
Standard extensions: `$CANGJIE_HOME/stdx/dynamic/stdx`.
Mode: normal (no stub tests). 73 generated cases + 4 retained existing cases.

| Plugin build | Optimization | Passed | Failed |
| --- | --- | ---: | ---: |
| release | `-O0` | 77 | 0 |
| release | `-O1` | 77 | 0 |
| debug | `-O0` | 73 | 4 |
| debug | `-O1` | 74 | 3 |

The plugin unit test `typeFilterSimpleTest` passed in both full runs.
The full release run passed all 154 functional executions. The debug run had
seven compiler-internal aborts; these were not expected-output mismatches.
Failures moved between full/debug retry runs; this does not establish their
root cause. No compiler/plugin implementation changes were made.

## Failed compilations in the full debug run

| Test | Level | Error |
| --- | --- | --- |
| `extension_constrained.cj` | `-O0` | cjc runtime: Check failed: ok; SIGABRT (signal 6); Internal Compiler Error: Interrupt signal (6) received.; Please report this to Cangjie team and include the project. Error Code: 12 |
| `extension_interface.cj` | `-O0` | cjc runtime: Check failed: ok; SIGABRT (signal 6); Internal Compiler Error: Interrupt signal (6) received.; Please report this to Cangjie team and include the project. Error Code: 12 |
| `mutable_class.cj` | `-O0` | cjc runtime: Check failed: ok; SIGABRT (signal 6); Internal Compiler Error: Interrupt signal (6) received.; Please report this to Cangjie team and include the project. Error Code: 12 |
| `try_invoke_static.cj` | `-O0` | cjc runtime: Check failed: ok; SIGABRT (signal 6); Internal Compiler Error: Interrupt signal (6) received.; Please report this to Cangjie team and include the project. Error Code: 12 |
| `higher_order_wrapper.cj` | `-O1` | cjc runtime: Check failed: ok; SIGABRT (signal 6); Internal Compiler Error: Interrupt signal (6) received.; Please report this to Cangjie team and include the project. Error Code: 12 |
| `invoke_constrained.cj` | `-O1` | cjc runtime: Check failed: ok; SIGABRT (signal 6); Internal Compiler Error: Interrupt signal (6) received.; Please report this to Cangjie team and include the project. Error Code: 12 |
| `mutable_struct.cj` | `-O1` | cjc runtime: Check failed: ok; SIGABRT (signal 6); Internal Compiler Error: Interrupt signal (6) received.; Please report this to Cangjie team and include the project. Error Code: 12 |

## Complete per-test matrix

All tests listed below passed with the release plugin at both optimization levels.
Debug statuses refer to the full debug run, not replacement by a retry result.

| Test | Release O0 | Release O1 | Debug O0 | Debug O1 |
| --- | --- | --- | --- | --- |
| `apply_class.cj` | PASS | PASS | PASS | PASS |
| `apply_constrained.cj` | PASS | PASS | PASS | PASS |
| `apply_generic_class.cj` | PASS | PASS | PASS | PASS |
| `apply_generic_struct.cj` | PASS | PASS | PASS | PASS |
| `apply_global.cj` | PASS | PASS | PASS | PASS |
| `apply_struct.cj` | PASS | PASS | PASS | PASS |
| `array_construction.cj` | PASS | PASS | PASS | PASS |
| `bound_struct_method.cj` | PASS | PASS | PASS | PASS |
| `box_generic_class.cj` | PASS | PASS | PASS | PASS |
| `box_generic_struct.cj` | PASS | PASS | PASS | PASS |
| `box_recursive_enum.cj` | PASS | PASS | PASS | PASS |
| `box_struct_interface.cj` | PASS | PASS | PASS | PASS |
| `boxed_mutable_capture.cj` | PASS | PASS | PASS | PASS |
| `boxed_reference_capture.cj` | PASS | PASS | PASS | PASS |
| `casts_rtti.cj` | PASS | PASS | PASS | PASS |
| `cfunc_signature.cj` | PASS | PASS | PASS | PASS |
| `classCtor.cj` | PASS | PASS | PASS | PASS |
| `complex.cj` | PASS | PASS | PASS | PASS |
| `extension_alias.cj` | PASS | PASS | PASS | PASS |
| `extension_constrained.cj` | PASS | PASS | FAIL | PASS |
| `extension_default.cj` | PASS | PASS | PASS | PASS |
| `extension_interface.cj` | PASS | PASS | FAIL | PASS |
| `extension_static_value.cj` | PASS | PASS | PASS | PASS |
| `generic_aggregate_fields.cj` | PASS | PASS | PASS | PASS |
| `generic_extension.cj` | PASS | PASS | PASS | PASS |
| `generic_extension_function_value.cj` | PASS | PASS | PASS | PASS |
| `generic_global_function_value.cj` | PASS | PASS | PASS | PASS |
| `generic_instance_function_value.cj` | PASS | PASS | PASS | PASS |
| `generic_mut_receiver.cj` | PASS | PASS | PASS | PASS |
| `generic_nested_closure.cj` | PASS | PASS | PASS | PASS |
| `generic_nested_enum.cj` | PASS | PASS | PASS | PASS |
| `generic_property.cj` | PASS | PASS | PASS | PASS |
| `generic_static_function_value.cj` | PASS | PASS | PASS | PASS |
| `generic_static_reference.cj` | PASS | PASS | PASS | PASS |
| `generic_static_value.cj` | PASS | PASS | PASS | PASS |
| `generic_variant_function.cj` | PASS | PASS | PASS | PASS |
| `global_constant.cj` | PASS | PASS | PASS | PASS |
| `higher_order_wrapper.cj` | PASS | PASS | PASS | FAIL |
| `invoke_constrained.cj` | PASS | PASS | PASS | FAIL |
| `invoke_static.cj` | PASS | PASS | PASS | PASS |
| `invoke_static_closure.cj` | PASS | PASS | PASS | PASS |
| `invoke_static_instance_context.cj` | PASS | PASS | PASS | PASS |
| `lambda_nothing_wrapper.cj` | PASS | PASS | PASS | PASS |
| `local_const_function.cj` | PASS | PASS | PASS | PASS |
| `loop_lowering.cj` | PASS | PASS | PASS | PASS |
| `multibranch.cj` | PASS | PASS | PASS | PASS |
| `mut_property.cj` | PASS | PASS | PASS | PASS |
| `mut_receiver.cj` | PASS | PASS | PASS | PASS |
| `mutable_array.cj` | PASS | PASS | PASS | PASS |
| `mutable_class.cj` | PASS | PASS | FAIL | PASS |
| `mutable_struct.cj` | PASS | PASS | PASS | FAIL |
| `nested_class_store.cj` | PASS | PASS | PASS | PASS |
| `nested_struct_increment.cj` | PASS | PASS | PASS | PASS |
| `nested_struct_store.cj` | PASS | PASS | PASS | PASS |
| `open_generic_class.cj` | PASS | PASS | PASS | PASS |
| `open_generic_extension.cj` | PASS | PASS | PASS | PASS |
| `open_generic_interface.cj` | PASS | PASS | PASS | PASS |
| `open_generic_mut_extension.cj` | PASS | PASS | PASS | PASS |
| `package_init.cj` | PASS | PASS | PASS | PASS |
| `pattern_interface_tuple.cj` | PASS | PASS | PASS | PASS |
| `spawn.cj` | PASS | PASS | PASS | PASS |
| `structCtor.cj` | PASS | PASS | PASS | PASS |
| `try_allocate_intrinsic.cj` | PASS | PASS | PASS | PASS |
| `try_apply.cj` | PASS | PASS | PASS | PASS |
| `try_arithmetic.cj` | PASS | PASS | PASS | PASS |
| `try_invoke.cj` | PASS | PASS | PASS | PASS |
| `try_invoke_static.cj` | PASS | PASS | FAIL | PASS |
| `try_numeric_cast.cj` | PASS | PASS | PASS | PASS |
| `try_spawn.cj` | PASS | PASS | PASS | PASS |
| `unary_binary_nodes.cj` | PASS | PASS | PASS | PASS |
| `unit_any.cj` | PASS | PASS | PASS | PASS |
| `varray_builder.cj` | PASS | PASS | PASS | PASS |
| `varray_nested_repeat.cj` | PASS | PASS | PASS | PASS |
| `varray_writeback.cj` | PASS | PASS | PASS | PASS |
| `vtable_multiple_parents.cj` | PASS | PASS | PASS | PASS |
| `vtable_value_parameter_wrapper.cj` | PASS | PASS | PASS | PASS |
| `vtable_value_return_wrapper.cj` | PASS | PASS | PASS | PASS |

## Targeted debug retry

Seven cases were rerun at both optimization levels. 12/14 executions passed.
The retry still aborted on these compilations:

| Test | Level | Error |
| --- | --- | --- |
| `extension_interface.cj` | `-O1` | cjc runtime: Check failed: ok; SIGABRT (signal 6); Internal Compiler Error: Interrupt signal (6) received.; Please report this to Cangjie team and include the project. Error Code: 12 |
| `higher_order_wrapper.cj` | `-O1` | cjc runtime: Check failed: ok; SIGABRT (signal 6); Internal Compiler Error: Interrupt signal (6) received.; Please report this to Cangjie team and include the project. Error Code: 12 |

## Reproduce the complete runs

```bash
source /home/s00827109/Projects/software/hotfix-20.10/envsetup.sh
export CANGJIE_STDX_PATH="$CANGJIE_HOME/stdx/dynamic/stdx"
cd /home/s00827109/Projects/hotfix_tools
python3 build.py build-plugin --build-type release --run-tests normal
python3 build.py build-plugin --build-type debug --run-tests normal
```

Full raw logs and machine-readable `results.csv`/`results.json` are in
`output/chir-test-results/`.
