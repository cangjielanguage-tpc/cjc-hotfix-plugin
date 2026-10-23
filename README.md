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

Patch generator fixtures can check standard output with `.expected` files and
generated CHIR with `.checks` files. Each nonempty, non-comment line in a
`.checks` file is a regular expression matched against the `chir-dis` dump in
multiline mode. A fixture must provide at least one of these expectation files;
when both are present, both are checked.

Build artifacts are written to `output/cjc-plugin` and `output/patch-gen`.
Remove all artifacts with:

```bash
python3 build.py clean
```

The resulting binaries are `output/cjc-plugin/libhotfix-plugin.so` and `output/patch-gen/patch-gen` on Linux (`libhotfix-plugin.dylib` on macOS).
