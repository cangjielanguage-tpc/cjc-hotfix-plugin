#!/bin/bash

set -e

build_dir=$1
cangjie_home=$2
plugin=$3
cjc_opt_level=$4
shift 4
filter_tests=("$@")

diff_generator="$build_dir/chir-diff"
envsetup=$cangjie_home/"envsetup.sh"

if [[ ! -f $envsetup ]]; then
    echo "$cangjie_home is not a proper Cangjie toolchain dir."
    exit 1
fi

source "$envsetup"

echo "Building patchable hotfix lib"
cd lib
hotfix_lib="hotfix.a"
cjc -p hotfix --output-type=staticlib -o $hotfix_lib
cd -

for file in *.cj; do
  if [[ "$file" == *"_patched"* ]]; then
      continue
  fi

  file_no_ext="${file%.*}"
  ext="${file##*.}"

  if (( ${#filter_tests[@]} > 0 )); then
    test_idx=-1
    for i in "${!filter_tests[@]}"; do
        if [[ "${filter_tests[i]}" == "$file_no_ext" ]]; then
            test_idx=$i
            break
        fi
    done
    if (( $test_idx == - 1 )); then
      continue
    fi
  fi

  echo "Test file: $file"

  patched=`find . -name "${file_no_ext}_patched.cj"`
  expected=`find . -name "${file_no_ext}.expected_$cjc_opt_level"`
  if [[ ! -f "$expected" ]]; then
    echo "No opt specific file ${file_no_ext}.expected_$cjc_opt_level was found. Using $file_no_ext.expected file"
    expected=`find . -name "${file_no_ext}.expected"`
    if [[ ! -f "$expected" ]]; then
      echo "Fail. .expected file was not found"
      exit 1
    fi
  fi

  actual="$file_no_ext.actual"

  cjc -Woff unused "$file" "lib/$hotfix_lib" --import-path "lib" --output-type=staticlib --plugin "$plugin" "-$cjc_opt_level" -o "lib$file_no_ext.chir" --emit-chir
  if [ -f "$patched" ]; then
    cjc -Woff unused "$patched" "lib/$hotfix_lib" --import-path "lib" --output-type=staticlib --plugin "$plugin" "-$cjc_opt_level" -o "lib${file_no_ext}_patched.chir" --emit-chir
    $diff_generator "lib$file_no_ext.chir" "lib${file_no_ext}_patched.chir" &>"$actual" || true
    patch_file="difflib${file_no_ext}_patched.chir"
  else
    $diff_generator "lib$file_no_ext.chir" &>"$actual" || true
    patch_file="difflib${file_no_ext}.chir"
  fi
  expected_data=`cat "$expected"`
  actual_data=`cat "$actual"`

  if [ -f "$patch_file" ]; then
    echo "Checking patch (.chir) validity using chir-dis"
    # just another check whether CHIR is valid from chir-dis point of view
    chir-dis "$patch_file"
  fi

  echo "Comparing test results"
  # compare output while ignoring trailing whitespace
  if diff -u <(printf '%s\n' "$actual_data" | sed 's/[[:space:]]*$//') \
             <(printf '%s\n' "$expected_data" | sed 's/[[:space:]]*$//'); then
    echo "Equals"
  else
    echo "Differs"
    echo "Expected:"
    echo "$expected_data"
    echo "Actual:"
    echo "$actual_data"
    exit 1
  fi

  rm -f *.cjo
  rm -f "lib$file_no_ext.chir"
  rm -f "lib${file_no_ext}_patched.chir"
  rm -f "difflib${file_no_ext}_patched.chirtxt"
  rm -f "$actual"

  echo
done

echo "All tests passed"
