#!/usr/bin/env bash
set -euo pipefail
BUILD_TYPE="${1:-Debug}"
# Portable lowercase: bash 3.2 (macOS default) has no ${var,,}.
BUILD_DIR="build-$(printf '%s' "$BUILD_TYPE" | tr '[:upper:]' '[:lower:]')"
./scripts/build.sh "$BUILD_TYPE"
ctest --test-dir "$BUILD_DIR" --output-on-failure
