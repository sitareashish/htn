#!/usr/bin/env bash
set -euo pipefail
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release >/dev/null
cmake --build build --target htn htn_load >/dev/null
echo "workers,conns,rate,achieved_rps,p50_ms,p99_ms,p999_ms"
for W in 1 2 4 8; do
  ./build/htn "$W" & SRV=$!; sleep 0.4
  for C in 200 600 1200; do
    for R in 20000 40000 60000; do
      OUT=$(./build/htn_load 127.0.0.1 9100 "$C" "$R" 15)
      RPS=$(echo "$OUT" | sed -n 's/.*achieved=\([0-9]*\).*/\1/p')
      P50=$(echo "$OUT" | sed -n 's/.*p50=\([0-9.]*\)ms.*/\1/p')
      P99=$(echo "$OUT" | sed -n 's/.*p99=\([0-9.]*\)ms.*/\1/p')
      P999=$(echo "$OUT"| sed -n 's/.*p99.9=\([0-9.]*\)ms.*/\1/p')
      echo "$W,$C,$R,$RPS,$P50,$P99,$P999"
    done
  done
  kill -INT $SRV; wait $SRV 2>/dev/null || true
done
