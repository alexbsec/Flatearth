#!/bin/bash
set -e

# Usage:
#   ./build-all.sh            -> Debug (default)
#   ./build-all.sh debug      -> Debug
#   ./build-all.sh release    -> Release
#
# Anything else -> error
mode="${1:-debug}"

if [[ ! "$mode" =~ ^([Dd]ebug|[Rr]elease)$ ]]; then
  echo "Usage: $0 [debug|release]"
  exit 1
fi
#
# Normalize
mode=$(echo "$mode" | tr '[:upper:]' '[:lower:]')

OUT_ROOT="$mode"

mkdir -p "$OUT_ROOT/bin"

pushd engine >/dev/null
./run.sh "$mode" "$OUT_ROOT"
popd >/dev/null

pushd testbed >/dev/null
./run.sh "$mode" "$OUT_ROOT"
popd >/dev/null
