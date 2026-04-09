#!/bin/bash

set -e

cangjie_home=$1
envsetup=$cangjie_home/"envsetup.sh"

if [[ ! -f $envsetup ]]; then
    echo "$cangjie_home is not a proper Cangjie toolchain dir."
    exit 1
fi

lsp=`find "$cangjie_home/tools" -name 'libcangjie-lsp.so'`;

if [ -z "$lsp" ]; then
  echo "libcangjie-lsp.so was not found under toolchain dir.";
  exit 1
fi

source "$envsetup"

echo "Building plugin in release mode"
cd ../src
cmake . -DLSP_PATH=`dirname "$lsp"` -DCMAKE_BUILD_TYPE="Release"
make -j
plugin=`pwd`"/libcjc_hotfix_plugin.so"

echo "Building unit tests"
cd ../test
cmake .
make -j

echo "Running unit tests"
ctest --extra-verbose

echo "Building patchable hotfix lib"
cd test_data/lib
hotfix_lib="hotfix.a"
cjc -p hotfix --output-type=staticlib -o $hotfix_lib

run_tests () {
  expected_ext=$1
  for file in *.cj; do
    ext="${file##*.}"
    echo "Test file: $file"
    expected=`find . -name "$file.$expected_ext"`

    if [ -f "$expected" ]; then
      echo "File to check results: $expected"
      rm -r *_CHIR || true
      cjc "$file" "lib/$hotfix_lib" --import-path "lib" --plugin "$plugin" --dump-chir
      actual_data=`./main`
      expected_data=`cat "$expected"`

      rm *.cjo
      rm *.cjo.flag

      if diff -Z <(echo "$actual_data") <(echo "$expected_data"); then
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
}

echo "Running functional tests in default mode"
cd ..
run_tests "expected"

echo "Building plugin in stub mode"
cd ../../src
cmake . -DLSP_PATH=`dirname "$lsp"` -DCMAKE_BUILD_TYPE=Test
make -j

echo "Running functional tests in stub mode"
cd ../test/test_data
run_tests "expected.stub"


echo "All tests passed"
