#!/usr/bin/env bash
#
# Build the Cangjie hotfix CHIR plugin into a shared library that `cjc` can load
# via `--plugin <path>`. Mirrors how stdx ships its own plugins
# (libstdx.collect_aspects.so / libstdx.weave_aspects.so): the resulting .so has
# NEEDED deps on libstdx.plugin.manager.so (which exports the @C
# `executeCHIRPlugins` entry point) and libstdx.chir.so, and the plugin class
# registers itself via a static initializer.
#
# Usage: ./build.sh
#
# Environment overrides:
#   CANGJIE_ENVSETUP  path to the cjc toolchain envsetup.sh
#   CANGJIE_STDX_ROOT directory containing the built `stdx/` module (with .cjo + .so)
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

CANGJIE_ENVSETUP="${CANGJIE_ENVSETUP:-/home/s00827109/Projects/cangjie_compiler/output/envsetup.sh}"
CANGJIE_STDX_ROOT="${CANGJIE_STDX_ROOT:-/home/s00827109/Projects/cangjie_stdx/target/linux_x86_64_cjnative/dynamic}"

# shellcheck disable=SC1090
source "$CANGJIE_ENVSETUP"

mkdir -p "$HERE/output"

cjc --output-type=dylib \
    -p "$HERE/src" \
    --import-path "$CANGJIE_STDX_ROOT" \
    -L "$CANGJIE_STDX_ROOT/stdx" -lstdx.chir -lstdx.plugin.manager \
    --output "$HERE/output/libhotfix-plugin-cj.so"

echo "Built: $HERE/output/libhotfix-plugin-cj.so"
