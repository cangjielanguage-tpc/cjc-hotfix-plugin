# Cangjie hotfix tools

This repository contains two related tools:

- `cjc-plugin` — a Cangjie compiler plugin that prepares CHIR for hot fixes.
- `patch-gen` — a utility that generates a hotfix patch.

## Environment

Set the toolchain and `stdx` paths before building.

## Build and test

Build the compiler plugin and optionally run its normal or stub tests:

```bash
python3 build.py build-plugin --run-tests normal
python3 build.py build-plugin --run-tests stub
```

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
