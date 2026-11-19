# Cangjie hotfix tools

This repository contains `cjc-plugin`, a Cangjie compiler plugin that prepares
CHIR for hot fixes. The patch generator and its functional tests are maintained
in the CBC compiler repository (`core/cbc-patchgen` and `test/patch-gen`).
Build patch-gen there with `sbt patchGen/nativeImage` (GraalVM) or
`sbt patchGen/assembly` (JVM).

## Environment

Source the selected SDK's `envsetup.sh` and set `CANGJIE_STDX_PATH` to
its matching stdx library directory before building. Keep that directory in
`LD_LIBRARY_PATH` when running plugin tests. The build script uses this
environment; `--version` selects compatibility code, not an SDK installation.

Hotfix SDK API versions have their own numbering, independent of compiler
releases and CHIR format versions:

| Hotfix SDK API version | Supported stdx.chir API |
| --- | --- |
| `1` | Baseline API without linkage mutation |
| `2` (default) | Version 1 plus `Function.setLinkageType` and `GlobalVar.setLinkageType` |

Supported versions are listed in [`VERSION`](VERSION), one
version number per line. `build.py` reads this file; the last
entry is the default. Add a version by appending the next number and documenting
its added APIs in the table above. Each version includes the APIs of earlier
versions.

The runner passes this independent API version through `HOTFIX_VERSION` and the
generic CJPM `--sdk` option for builds and unit tests. For direct CJPM builds,
set `HOTFIX_VERSION` and use `cjpm build --sdk`. The plugin uses compile-time
`@When` conditions for version compatibility. These conditions take string
values: `version == "1"` selects the baseline fallback, while
`version > "1"` selects linkage mutation for version 2 and later. Ordered
string comparisons would compare `"10"` before `"2"`.

## Build and test

Build the compiler plugin and optionally run its tests:

```bash
python3 build.py build-plugin --version 1 --build-type release --run-tests normal
python3 build.py build-plugin --version 1 --build-type release --run-tests stub
python3 build.py build-plugin --version 1 --build-type release --run-tests throw-stub
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
it to `true`; normal and stub plugin tests default it to
`false` to test annotation-based selection.

Plugin tests use the following layout:

```
test/
  functional/
    tests_data/          # sources, expected outputs, maps, and test libraries
    exclude_list         # optional shared exclusions
    exclude_list.stub    # optional shared plugin stub-mode exclusions
    <version>/
      exclude_list       # optional exclusions for the selected hotfix SDK API version
```

Plugin tests also load `exclude_list.normal`, `exclude_list.stub`, or
`exclude_list.throw-stub` under the selected version directory. A version's
`exclude_list` applies to all its modes. The shared plugin `exclude_list`
applies to normal mode, and the shared `exclude_list.stub` applies to stub mode.

Entries are test names with or without `.cj`; blank lines and `#` comments are
ignored. Add a reason with each exclusion, for example:

```
example_test # Explain the SDK limitation that prevents this test from passing.
```

Skips are printed. Missing lists or version directories exclude nothing; create
lists only when there are tests to skip. Default functional test levels are O0,
O1, and O2; select a subset with `--test-compile-level`. Tests write temporary
files into `tests_data/`, so run SDK/mode test commands sequentially.

Useful options are `--build-type`, `--filter-tests`, and
`--test-compile-level`. Run `python3 build.py COMMAND --help` for details.

Build artifacts are written to `output/<version>/cjc-plugin`.
Remove all artifacts with:

```bash
python3 build.py clean
```

The resulting library is `output/<version>/cjc-plugin/libhotfix-plugin.so` on
Linux (`libhotfix-plugin.dylib` on macOS).
