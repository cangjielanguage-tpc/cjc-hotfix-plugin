#!/usr/bin/env bash

set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PLUGIN_ROOT="$(cd "$HERE/.." && pwd)"
TEST_DATA="$HERE/test_data"

EXPECTED_EXT="${1:-expected}"

CANGJIE_ENVSETUP="${CANGJIE_ENVSETUP:-/home/s00827109/Projects/cangjie_compiler/output/envsetup.sh}"
CANGJIE_STDX_ROOT="${CANGJIE_STDX_ROOT:-/home/s00827109/Projects/cangjie_stdx/target/linux_x86_64_cjnative/dynamic}"

# shellcheck disable=SC1090
source "$CANGJIE_ENVSETUP"

export LD_LIBRARY_PATH="$PLUGIN_ROOT/output:$CANGJIE_STDX_ROOT/stdx:${LD_LIBRARY_PATH:-}"

"$PLUGIN_ROOT/build.sh"

if cjc -v | grep -qi "darwin"; then
    plugin="$PLUGIN_ROOT/output/libhotfix-plugin-cj.dylib"
else
    plugin="$PLUGIN_ROOT/output/libhotfix-plugin-cj.so"
fi

if [[ ! -f "$plugin" ]]; then
    echo "Plugin library was not built: $plugin"
    exit 1
fi

work_dir="$(mktemp -d)"
cleanup() {
    rm -rf "$work_dir"
}
trap cleanup EXIT

cp -R "$TEST_DATA"/. "$work_dir"/

echo "Building patchable hotfix lib"
(
    cd "$work_dir/lib"
    cjc -p hotfix --output-type=staticlib -o hotfix.a
)

for file in "$work_dir"/*.cj; do
    name="$(basename "$file")"
    expected="$work_dir/$name.$EXPECTED_EXT"

    echo "Test file: $name"
    if [[ ! -f "$expected" ]]; then
        echo "Fail. File $name.$EXPECTED_EXT to check results was not found"
        exit 1
    fi

    (
        cd "$work_dir"
        rm -rf *_CHIR
        cjc "$name" "lib/hotfix.a" --import-path "lib" --plugin "$plugin" --dump-chir
        actual_data="$(./main)"
        expected_data="$(cat "$expected")"

        rm -f ./*.cjo ./*.cjo.flag

        if diff -Z -u <(printf '%s\n' "$expected_data") <(printf '%s\n' "$actual_data"); then
            echo "Passed"
        else
            echo "Failed"
            echo "Expected:"
            echo "$expected_data"
            echo "Actual:"
            echo "$actual_data"
            exit 1
        fi
    )
done

echo "All tests passed"
