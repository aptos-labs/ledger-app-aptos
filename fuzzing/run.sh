#!/usr/bin/env bash

set -e

SCRIPTDIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
BUILDDIR="$SCRIPTDIR/build"
CORPUSDIR="$SCRIPTDIR/corpus"

# Create corpus directory if it doesn't exist
mkdir -p "$CORPUSDIR"

"$BUILDDIR"/fuzz_tx_parser "$CORPUSDIR" "$@" >/dev/null
