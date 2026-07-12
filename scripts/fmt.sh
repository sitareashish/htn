#!/usr/bin/env bash
set -euo pipefail
find src include tests -type f \( -name '*.c' -o -name '*.h' \) \
    -not -path '*/vendor/*' -print0 \
    | xargs -0 -r clang-format-17 -i
echo "OK: formatted src/, include/, tests/"
