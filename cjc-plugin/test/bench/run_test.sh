#!/bin/bash

set -e

test_name=$1
runs=$2
arg1=$3

cd "$test_name"

baseline="$test_name"
baseline_temp="${baseline}_temp"
mkdir -p $baseline_temp
cjc "${test_name}.cj" -o "$baseline" -O2 -j 1 --save-temps $baseline_temp
hyperfine "./$baseline $arg1" --export-markdown "${baseline}.result" --warmup 5 -r "$runs"
echo "Binary size: $(stat -c %s $baseline)"

rm "cjpm.toml" || true
no_patchable="${test_name}_no_patchable"
no_patchable_temp="${no_patchable}_temp"
mkdir -p $no_patchable_temp
cjc "${test_name}.cj" -o "$no_patchable" --plugin ../libcjc_hotfix_plugin.so -O2 -j 1 --save-temps $no_patchable_temp
hyperfine "./$no_patchable $arg1" --export-markdown "${no_patchable}.result" --warmup 5 -r "$runs"
echo "Binary size: $(stat -c %s $no_patchable)"

cp "../cjpm.toml" .
patchable="${test_name}_patchable"
patchable_temp="${patchable}_temp"
mkdir -p $patchable_temp
cjc "${test_name}.cj" ../hotfix.a -o "$patchable" --import-path=.. --plugin ../libcjc_hotfix_plugin.so -O2 -j 1 --save-temps $patchable_temp
hyperfine "./$patchable $arg1" --export-markdown "${patchable}.result" --warmup 5 -r "$runs"
rm "cjpm.toml"
echo "Binary size: $(stat -c %s $patchable)"

if ! diff <(./$baseline $arg1) <(./$patchable $arg1) > /dev/null; then
    echo "results differ:"
    echo "baseline:"
    ./$baseline $arg1
    echo "patchable:"
    ./$patchable $arg1
    exit 1
fi

cd ..