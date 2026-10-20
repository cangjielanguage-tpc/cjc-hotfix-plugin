# Annotation-only functional test results

Toolchain: `/home/s00827109/Projects/software/hotfix-20.10`.
Configuration: release plugin, normal mode, **HOTFIX_PATCH_ALL=false**.
Current suite: **79 generated + 4 retained = 83 standalone tests**.

| Optimization | Passed | Failed |
| --- | ---: | ---: |
| O0 | 83 | 0 |
| O1 | 83 | 0 |

The plugin unit test `typeFilterSimpleTest` passed. After renaming drivers to `doTest` and inlining 17 thin wrappers, the CHIR audit
compiled and executed all 83 cases again at both levels: all 166 executions
passed and selected input node coverage remained 74/82.

## Validation with deliberate failure controls

The all-pass result is limited to compilation and original program behavior.
With the same toolchain and `HOTFIX_PATCH_ALL=false`, **all 83 tests also passed
at O0 with the compiler plugin omitted**. This is consistent with normal-mode
semantics, but means output comparisons alone cannot detect a missing plugin.
The separate CHIR audit checks inserted guards; the functional runner does not
require those guards to exist.

The official runner was also exercised against isolated copies of
`ordinary_arithmetic.cj`; checked-in test sources were unchanged.

| Control | Runner result | Interpretation |
| --- | --- | --- |
| Unchanged test with plugin | PASS | Baseline |
| Remove patchable annotations, keep plugin | PASS | Missing ordinary-function instrumentation is not detected |
| Deliberately wrong expected output | FAIL | Output mismatch detected |
| Return an incorrect result | FAIL | Nonzero program exit detected |
| Introduce invalid source syntax | FAIL | Compiler error detected |

These controls validate the runner's error handling, but expose a gap in its
plugin-specific assertions. Guard flags start false, so these normal runs do
not demonstrate successful dispatch to a replacement implementation. Coverage
of 74/82 CHIR expression kinds describes selected input bodies, not active patch
execution or every signature/type combination. Some fixtures also have weak
runtime assertions and primarily exercise compilation.

Evidence: `output/plugin-controls/no-plugin-results.json`,
`output/plugin-controls/runner-controls.json`, and per-control `runner.log`
files under `output/plugin-controls/runner/`.

## Failed tests and errors

| Test | Level | Error |
| --- | --- | --- |
| None | O0, O1 | No compile, runtime, or expected-output failures in the final run |

## All passed tests

| Test | O0 | O1 |
| --- | --- | --- |
| `apply_class.cj` | PASS | PASS |
| `apply_constrained.cj` | PASS | PASS |
| `apply_generic_class.cj` | PASS | PASS |
| `apply_generic_struct.cj` | PASS | PASS |
| `apply_global.cj` | PASS | PASS |
| `apply_struct.cj` | PASS | PASS |
| `array_construction.cj` | PASS | PASS |
| `bound_struct_method.cj` | PASS | PASS |
| `box_generic_class.cj` | PASS | PASS |
| `box_generic_struct.cj` | PASS | PASS |
| `box_recursive_enum.cj` | PASS | PASS |
| `box_struct_interface.cj` | PASS | PASS |
| `boxed_mutable_capture.cj` | PASS | PASS |
| `boxed_reference_capture.cj` | PASS | PASS |
| `casts_rtti.cj` | PASS | PASS |
| `cfunc_signature.cj` | PASS | PASS |
| `classCtor.cj` | PASS | PASS |
| `complex.cj` | PASS | PASS |
| `exceptional_array_literal.cj` | PASS | PASS |
| `exceptional_intrinsic.cj` | PASS | PASS |
| `extension_alias.cj` | PASS | PASS |
| `extension_constrained.cj` | PASS | PASS |
| `extension_default.cj` | PASS | PASS |
| `extension_interface.cj` | PASS | PASS |
| `extension_static_value.cj` | PASS | PASS |
| `generic_aggregate_fields.cj` | PASS | PASS |
| `generic_extension.cj` | PASS | PASS |
| `generic_extension_function_value.cj` | PASS | PASS |
| `generic_global_function_value.cj` | PASS | PASS |
| `generic_instance_function_value.cj` | PASS | PASS |
| `generic_mut_receiver.cj` | PASS | PASS |
| `generic_nested_closure.cj` | PASS | PASS |
| `generic_nested_enum.cj` | PASS | PASS |
| `generic_property.cj` | PASS | PASS |
| `generic_recovery.cj` | PASS | PASS |
| `generic_static_function_value.cj` | PASS | PASS |
| `generic_static_reference.cj` | PASS | PASS |
| `generic_static_value.cj` | PASS | PASS |
| `generic_variant_function.cj` | PASS | PASS |
| `global_constant.cj` | PASS | PASS |
| `higher_order_wrapper.cj` | PASS | PASS |
| `invoke_constrained.cj` | PASS | PASS |
| `invoke_static.cj` | PASS | PASS |
| `invoke_static_closure.cj` | PASS | PASS |
| `invoke_static_instance_context.cj` | PASS | PASS |
| `iterator_lowering.cj` | PASS | PASS |
| `lambda_nothing_wrapper.cj` | PASS | PASS |
| `local_const_function.cj` | PASS | PASS |
| `loop_lowering.cj` | PASS | PASS |
| `multibranch.cj` | PASS | PASS |
| `mut_property.cj` | PASS | PASS |
| `mut_receiver.cj` | PASS | PASS |
| `mutable_array.cj` | PASS | PASS |
| `mutable_class.cj` | PASS | PASS |
| `mutable_struct.cj` | PASS | PASS |
| `nested_class_store.cj` | PASS | PASS |
| `nested_struct_increment.cj` | PAmutable_classSS | PASS |
| `nested_struct_store.cj` | PASS | PASS |
| `open_generic_class.cj` | PASS | PASS |
| `open_generic_extension.cj` | PASS | PASS |
| `open_generic_interface.cj` | PASS | PASS |
| `open_generic_mut_extension.cj` | PASS | PASS |
| `ordinary_arithmetic.cj` | PASS | PASS |
| `package_init.cj` | PASS | PASS |
| `pattern_enum_interface.cj` | PASS | PASS |
| `pattern_interface_tuple.cj` | PASS | PASS |
| `spawn.cj` | PASS | PASS |
| `structCtor.cj` | PASS | PASS |
| `try_allocate_intrinsic.cj` | PASS | PASS |
| `try_apply.cj` | PASS | PASS |
| `try_arithmetic.cj` | PASS | PASS |
| `try_invoke.cj` | PASS | PASS |
| `try_invoke_static.cj` | PASS | PASS |
| `try_numeric_cast.cj` | PASS | PASS |
| `try_spawn.cj` | PASS | PASS |
| `unary_binary_nodes.cj` | PASS | PASS |
| `unit_any.cj` | PASS | PASS |
| `varray_builder.cj` | PASS | PASS |
| `varray_nested_repeat.cj` | PASS | PASS |
| `varray_writeback.cj` | PASS | PASS |
| `vtable_multiple_parents.cj` | PASS | PASS |
| `vtable_value_parameter_wrapper.cj` | PASS | PASS |
| `vtable_value_return_wrapper.cj` | PASS | PASS |

## Coverage is measured separately

[CHIR_NODE_COVERAGE.md](CHIR_NODE_COVERAGE.md) records **74/82 expression kinds in
selected plugin input**, the remaining phase distinctions, and limitations.
`structCtor.cj` passes but selects no functions; it is retained as an existing
fixture and does not establish ordinary-function instrumentation coverage.

Earlier debug runs with patch-all enabled had compiler-internal SIGABRT failures.
They are historical results for an earlier test revision, not results for all 83
current cases. Their logs and the earlier report are preserved in
`output/chir-test-results/` (`debug.log`, `debug-retry.log`, and
`historical-patch-all-results.md`). The latest annotation-only run reported no
compiler aborts. The debug failures' root cause has not been diagnosed.

## Reproduce

```bash
source /home/s00827109/Projects/software/hotfix-20.10/envsetup.sh
export CANGJIE_STDX_PATH="$CANGJIE_HOME/stdx/dynamic/stdx"
export HOTFIX_PATCH_ALL=false
python3 build.py build-plugin --build-type release --run-tests normal
```

Raw log: `output/chir-test-results/annotation-only-release.log`.
Machine-readable results: `annotation-only-results.csv` and
`annotation-only-results.json` in the same directory.

Latest verification after wrapper inlining: `output/chir-test-results/inlined-audit.log`,
with per-test statuses in `inlined-results.csv` and `inlined-results.json`.
