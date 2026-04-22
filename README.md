# Hotfix plugin for Cangjie compiler

Plugin for Cangjie compiler (`cjc`) that makes certain transformations during CHIR generation phase 
to prepare the source code for possible hot fixes in the future.  

## Build

Clean directory with artifacts (`output`) if it exists:

```bash
python3 build.py clean
```

Build `output/libcjc_hotfix_plugin.so` in corresponding build mode:

```bash
python3 build.py build --build_type <debug or release> <cangjie_toolchain_path>
```

## Test

Run tests in corresponding test mode:

```bash
python3 build.py build --run-tests <normal or stub> <cangjie_toolchain_path>
```

## Structure description

- `src` - plugin source code
- `test` - plugin unit tests and integration tests
- `libs` - external libraries
  - Now only `toml++.h` locates there as one-header file library.
    Probably should be removed if `cjc` provides proper implementation (TODO).
