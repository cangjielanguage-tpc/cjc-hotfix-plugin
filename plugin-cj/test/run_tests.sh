#!/bin/bash

set -e

expected_ext=$1
build_dir=$2
cangjie_home=$3

envsetup=$cangjie_home/"envsetup.sh"

if [[ ! -f $envsetup ]]; then
    echo "$cangjie_home is not a proper Cangjie toolchain dir."
    exit 1
fi

# shellcheck disable=SC1090
source "$envsetup"

export HOTFIX_TEST_MODE=1
if [[ "$expected_ext" == "expected.stub" ]]; then
  export HOTFIX_STUB_TEST=1
else
  unset HOTFIX_STUB_TEST
fi

if cjc -v | grep -qi "darwin"; then
  plugin="$build_dir/libhotfix-plugin-cj.dylib"
else
  plugin="$build_dir/libhotfix-plugin-cj.so"
fi

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

    # Difference from the C++ PatcherStub: C++ resolves
    # `builder.GetChirContext().GetSourceFileName(1) + ".stub.map"` inside the
    # plugin. The public stdx.chir API used by plugin-cj does not expose that
    # source-file lookup, so the test runner passes the equivalent map path.
    export HOTFIX_STUB_MAP_FILE="$file.stub.map"

    cjc "$file" "lib/$hotfix_lib" --import-path "lib" --plugin "$plugin" --dump-chir
    # TODO if macos, should be run on simulator
    actual_data=`./main`
    expected_data=`cat "$expected"`

    rm -f *.cjo
    rm -f *.cjo.flag

    # TODO -Z, -u are not supported on macos
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
