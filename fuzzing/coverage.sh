#!/usr/bin/env bash
# Generate code coverage reports from fuzzing results

set -e

SCRIPTDIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
BUILDDIR="$SCRIPTDIR/build"
CORPUSDIR="$SCRIPTDIR/corpus"
HTMLCOVDIR="$SCRIPTDIR/html-coverage"

# Compile the fuzzer with code coverage support
rm -rf "$BUILDDIR" "$HTMLCOVDIR"
cmake -DCMAKE_C_COMPILER=/usr/bin/clang -DCMAKE_CXX_COMPILER=/usr/bin/clang++ -DCODE_COVERAGE=1 -B"$BUILDDIR" -H.
cmake --build "$BUILDDIR" --target fuzz_tx_parser

# Run the fuzzer on the corpus files
export LLVM_PROFILE_FILE="$BUILDDIR/fuzz_tx_parser.profraw"
"$BUILDDIR/fuzz_tx_parser" "$CORPUSDIR" -runs=0
llvm-profdata merge --sparse "$LLVM_PROFILE_FILE" -o "$BUILDDIR/fuzz_tx_parser.profdata"
llvm-cov show --object "$BUILDDIR/fuzz_tx_parser" -instr-profile="$BUILDDIR/fuzz_tx_parser.profdata" -show-line-counts-or-regions -output-dir="$HTMLCOVDIR" -format=html
llvm-cov report --object "$BUILDDIR/fuzz_tx_parser" -instr-profile="$BUILDDIR/fuzz_tx_parser.profdata"
