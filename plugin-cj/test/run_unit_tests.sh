#!/usr/bin/env bash

set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PLUGIN_ROOT="$(cd "$HERE/.." && pwd)"

CANGJIE_ENVSETUP="${CANGJIE_ENVSETUP:-/home/s00827109/Projects/cangjie_compiler/output/envsetup.sh}"
CANGJIE_STDX_ROOT="${CANGJIE_STDX_ROOT:-/home/s00827109/Projects/cangjie_stdx/target/linux_x86_64_cjnative/dynamic}"

# shellcheck disable=SC1090
source "$CANGJIE_ENVSETUP"

mkdir -p "$PLUGIN_ROOT/output/test"

export LD_LIBRARY_PATH="$CANGJIE_STDX_ROOT/stdx:${LD_LIBRARY_PATH:-}"

cjc "$PLUGIN_ROOT/src/type_filter.cj" "$HERE/unit/type_filter_test.cj" \
    --import-path "$CANGJIE_STDX_ROOT" \
    -L "$CANGJIE_STDX_ROOT/stdx" -lstdx.chir -lstdx.unittest \
    -o "$PLUGIN_ROOT/output/test/type_filter_test"

"$PLUGIN_ROOT/output/test/type_filter_test"
