#!/usr/bin/env bash
set -euo pipefail
BUILD_TYPE="${1:-Debug}"
# Portable lowercase: bash 3.2 (macOS default) has no ${var,,}.
BUILD_DIR="build-$(printf '%s' "$BUILD_TYPE" | tr '[:upper:]' '[:lower:]')"
cmake -B "$BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
cmake --build "$BUILD_DIR"
echo "OK: $BUILD_DIR/htn"
