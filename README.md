# Cangjie hotfix tools

This repository contains two related tools:

- `cjc-plugin` — a Cangjie compiler plugin that prepares CHIR for hot fixes.
- `patch-gen` — a utility that generates a hotfix patch.

## Environment

Set the toolchain and `stdx` paths before building.

## Build and test

Build the compiler plugin and optionally run its tests:

```bash
python3 build.py build-plugin --run-tests normal
python3 build.py build-plugin --run-tests stub
python3 build.py build-plugin --run-tests throw-stub
```

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
python3 build.py build-patch-gen --plugin <plugin_path> --build-type release --run-tests
```

Useful shared options are `--build-type`, `--filter-tests`, and
`--test-compile-level`. Run `python3 build.py COMMAND --help` for details.

Build artifacts are written to `output/cjc-plugin` and `output/patch-gen`.
Remove all artifacts with:

```bash
python3 build.py clean
```

The resulting binaries are `output/cjc-plugin/libhotfix-plugin.so` and `output/patch-gen/patch-gen` on Linux (`libhotfix-plugin.dylib` on macOS).
