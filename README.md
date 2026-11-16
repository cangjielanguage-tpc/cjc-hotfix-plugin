# Cangjie hotfix tools

This repository contains two related tools:

- `cjc-plugin` — a Cangjie compiler plugin that prepares CHIR for hot fixes.
- `patch-gen` — a utility that generates a hotfix patch.

## Environment

Source the Cangjie v1.2.0-beta.rc3 SDK's `envsetup.sh` to set `CANGJIE_HOME`.
This branch uses a C++ plugin and does not require a `stdx` path.

## Build and test

Build the compiler plugin and optionally run its tests:

```bash
python3 build.py build-plugin --run-tests normal
python3 build.py build-plugin --run-tests stub
```

Test modes are selected at build time on this branch. Rebuild the plugin
without `--run-tests` before installing it.

Build the patch generator and run its tests with the normal test-mode plugin:

```bash
python3 build.py build-plugin --run-tests normal
python3 build.py build-patch-gen --plugin <plugin_path> --build-type release --run-tests
```

`--build-type` is shared; patch-gen tests also accept `--filter-tests` and
`--test-compile-level`. Run `python3 build.py COMMAND --help` for details.

Build artifacts are written to `output/cjc-plugin` and `output/patch-gen`.
Remove all artifacts with:

```bash
python3 build.py clean
```

The resulting binaries are `output/cjc-plugin/libhotfix-plugin.so` and `output/patch-gen/chir-diff` on Linux (`libhotfix-plugin.dylib` on macOS).
