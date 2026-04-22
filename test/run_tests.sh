#!/bin/bash

set -e

expected_ext=$1
build_dir=$2
cangjie_home=$3

plugin="$build_dir/libcjc_hotfix_plugin.so"

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
  ext="${file##*.}"
  echo "Test file: $file"
  expected=`find . -name "$file.$expected_ext"`

  if [ -f "$expected" ]; then
    echo "File to check results: $expected"
    rm -rf *_CHIR
    cjc "$file" "lib/$hotfix_lib" --import-path "lib" --plugin "$plugin" --dump-chir
    actual_data=`./main`
    expected_data=`cat "$expected"`

    rm -f *.cjo
    rm -f *.cjo.flag

    if diff -Z -u <(echo "$actual_data") <(echo "$expected_data"); then
      echo "Passed"
    else
      echo "Failed"
      echo "Expected:"
      echo "$expected_data"
      echo "Actual:"
      echo "$actual_data"
      exit 1
    fi

  else
     echo "Fail. File $file.$expected_ext to check results was not found"
     exit 1
  fi
done

echo "All tests passed"
