#!/bin/bash
set -e

SCRIPTDIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
BUILDDIR="$SCRIPTDIR/build"

# Compile fuzzer
rm -rf "$BUILDDIR"
mkdir "$BUILDDIR"
cd "$BUILDDIR"

cmake -DCMAKE_C_COMPILER=/usr/bin/clang -DCMAKE_CXX_COMPILER=/usr/bin/clang++ -DSANITIZER=memory ..
make clean
make fuzz_tx_parser
