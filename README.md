# Cangjie hotfix tools

This repository contains two related tools:

- `cjc-plugin` — a Cangjie compiler plugin that prepares CHIR for hot fixes.
- `patch-gen` — a utility that generates a hotfix patch.

## Environment

Source the selected SDK's `envsetup.sh` and set `CANGJIE_STDX_PATH` to
its matching stdx library directory before building. Keep that directory in
`LD_LIBRARY_PATH` when running patch-gen tests. The build script uses this
environment; `--version` selects compatibility code, not an SDK installation.

| SDK version |
| --- |
| `13008` |
| `13010` (default) |

Supported versions are listed in [`VERSION`](VERSION), one
version number per line. Both `build.py` and CMake read this file; the last
entry is the default. Add a version by appending its number.

The runner passes the selected version through `HOTFIX_SDK_VERSION` and the
generic CJPM `--sdk` option for builds and unit tests. For direct CJPM builds,
set `HOTFIX_SDK_VERSION` and use `cjpm build --sdk`. The plugin uses compile-time
`@When` conditions for version compatibility. Patch-gen receives the numeric
C++ definition `HOTFIX_SDK_VERSION`. Use Clang for patch-gen (the build script
defaults to `clang++`; override with `CXX` if needed).

## Build and test

Build the compiler plugin and optionally run its tests:

```bash
python3 build.py build-plugin --version 13008 --build-type release --run-tests normal
python3 build.py build-plugin --version 13008 --build-type release --run-tests stub
python3 build.py build-plugin --version 13008 --build-type release --run-tests throw-stub
```

Set `HOTFIX_STUB_TEST=true` when invoking `cjc` to enable the
test stub patcher. This runtime option defaults to `false` and requires no
plugin rebuild. The test runner sets it according to `--run-tests`.

The `throw-stub` test mode sets `HOTFIX_GEN_THROW_STUBS=true` when running the compiler.
It runs all functional test sources and checks for exit status 1 with
`Exception: should not reach here`, without reading expected-output files.
Set `HOTFIX_GEN_THROW_STUBS=true` when invoking `cjc` to replace selected patchables'
bodies with throwing stubs. This runtime option is disabled by default and
requires no plugin rebuild.

`HOTFIX_PATCH_ALL` defaults to `true`, selecting all eligible functions when no
`app-patchable` filter is present. Set `HOTFIX_PATCH_ALL=false` when invoking `cjc`
to select only annotated functions and functions matching the filter.
This runtime option requires no plugin rebuild. Throw-stub plugin tests default
it to `true`; normal and stub plugin tests and patch-gen tests default it to
`false` to test annotation-based selection.

Build the patch generator and run its tests (the plugin must be built first):

```bash
python3 build.py build-patch-gen --version 13008 --build-type release --run-tests
```

By default patch-gen tests use the plugin from the selected version's output
directory. Use `--plugin <path>` to override it; it must match the active SDK.

Both tools use the same functional test layout:

```text
test/
  functional/
    tests_data/          # sources, expected outputs, maps, and test libraries
    exclude_list         # optional shared exclusions
    exclude_list.stub    # optional shared plugin stub-mode exclusions
    <version>/
      exclude_list       # optional exclusions for the selected SDK version
```

Plugin tests also load `exclude_list.normal`, `exclude_list.stub`, or
`exclude_list.throw-stub` under the selected version directory. A version's
`exclude_list` applies to all its modes. The shared plugin `exclude_list`
applies to normal mode, and the shared `exclude_list.stub` applies to stub mode.
Patch-gen loads both its shared and selected version's `exclude_list`.

Entries are test names with or without `.cj`; blank lines and `#` comments are
ignored. Add a reason with each exclusion, for example:

```text
example_test # Explain the SDK limitation that prevents this test from passing.
```

Skips are printed. Missing lists or version directories exclude nothing; create
lists only when there are tests to skip. Default functional test levels are O0,
O1, and O2; select a subset with `--test-compile-level`. Tests write temporary
files into `tests_data/`, so run SDK/mode test commands sequentially.

Useful shared options are `--build-type`, `--filter-tests`, and
`--test-compile-level`. Run `python3 build.py COMMAND --help` for details.

Build artifacts are written to `output/<version>/cjc-plugin` and `output/<version>/patch-gen`.
Remove all artifacts with:

```bash
python3 build.py clean
```

The resulting binaries are `output/<version>/cjc-plugin/libhotfix-plugin.so` and `output/<version>/patch-gen/patch-gen` on Linux (`libhotfix-plugin.dylib` on macOS).
