# Changelog

## v1.0.0
First complete release. 15-day build:
- Days 1–2: toolchain, logging, blocking echo baseline
- Days 3–4: level- then edge-triggered epoll, non-blocking I/O
- Day 5: binary wire protocol + split-read-proof framer
- Day 6: zero-malloc connection pool (arena + free-list)
- Day 7: signalfd/eventfd/timerfd, race-free shutdown
- Days 8–9: SO_REUSEPORT share-nothing workers, CPU pinning, lock-free SPSC ring
- Day 10: per-worker HDR-histogram metrics
- Day 11: admin HTTP /metrics, /healthz, /ready (Prometheus format)
- Day 12: chaos client + ASan/UBSan/TSan hardening
- Day 13: honest benchmarking (wrk2), perf flamegraph, tuning
- Day 14: Prometheus + Grafana + Docker Compose + soak test
- Day 15: docs, demo, v1.0.0 release
