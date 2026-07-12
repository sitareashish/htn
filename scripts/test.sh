#!/usr/bin/env bash
set -euo pipefail
BUILD_TYPE="${1:-Debug}"
BUILD_DIR="build-${BUILD_TYPE,,}"
./scripts/build.sh "$BUILD_TYPE"
ctest --test-dir "$BUILD_DIR" --output-on-failure
