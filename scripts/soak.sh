#!/usr/bin/env bash
# Steady moderate load for N hours; log RSS + fd count every 30s.
set -euo pipefail
HOURS="${1:-4}"
PID=$(pgrep -n htn)
end=$(( $(date +%s) + HOURS*3600 ))
echo "ts,rss_kb,open_fds" > bench/soak.csv
# background: constant load for the whole window
( while [ "$(date +%s)" -lt "$end" ]; do ./build/htn_load 127.0.0.1 9100 400 25000 60; done ) &
LOAD=$!
while [ "$(date +%s)" -lt "$end" ]; do
  RSS=$(awk '/VmRSS/{print $2}' /proc/"$PID"/status)
  FDS=$(ls /proc/"$PID"/fd | wc -l)
  echo "$(date +%s),$RSS,$FDS" >> bench/soak.csv
  sleep 30
done
kill $LOAD 2>/dev/null || true
echo "soak done -> bench/soak.csv"
