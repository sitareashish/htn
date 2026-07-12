#!/usr/bin/env bash
set -euo pipefail
BUILD_TYPE="${1:-Debug}"
BUILD_DIR="build-${BUILD_TYPE,,}"
cmake -B "$BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
cmake --build "$BUILD_DIR"
echo "OK: $BUILD_DIR/htn"
