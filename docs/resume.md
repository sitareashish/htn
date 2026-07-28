# HTN Server — resume bullets (each backed by a repo artifact)

- Built a multithreaded, share-nothing epoll TCP server in C11 sustaining
  ~46K req/s across 1200 concurrent clients at p99 < 2ms; results reproducible
  via `scripts/bench-matrix.sh` (coordinated-omission-corrected with wrk2).
- Designed a lock-free SPSC ring buffer (~1.8M ops/sec, `bench/bench_spsc`) and
  per-core share-nothing workers with SO_REUSEPORT, eliminating hot-path locks
  (verified race-free under ThreadSanitizer in CI).
- Implemented a zero-malloc connection pool (arena + intrusive free-list) and an
  edge-triggered, split-read-proof binary framer; validated with a chaos client
  under ASan/UBSan/TSan (zero leaks, races, or crashes).
- Shipped production-style observability: Prometheus /metrics with HDR-histogram
  p50/p99/p99.9, Grafana dashboards + alerts, Docker Compose, and a 4-hour soak
  test showing flat memory and latency.
