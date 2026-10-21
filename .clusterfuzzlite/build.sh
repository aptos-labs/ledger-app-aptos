#!/bin/bash -eu

# build fuzzers

pushd fuzzing
cmake -DBOLOS_SDK=../BOLOS_SDK -Bbuild -H. -DCODE_COVERAGE=1
cmake --build build --target fuzz_tx_parser
mv ./build/fuzz_tx_parser "${OUT}/fuzz_tx_parser"
popd
