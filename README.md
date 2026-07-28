# HTN — High-Throughput Network Server

A multithreaded, share-nothing **epoll** TCP server in **C11**: one worker per core,
per-worker `SO_REUSEPORT` listeners, edge-triggered non-blocking I/O, a zero-malloc
connection pool, a lock-free SPSC ring, per-worker HDR-histogram metrics, and a
Prometheus/Grafana observability stack. Built from scratch in 15 days.

## Highlights
- **~46K RPS** at **1200 concurrent clients**, **p99 < 2ms** (see `bench/`, reproducible)
- **Share-nothing** workers: zero locks on the hot path (TSan-clean)
- **Zero-malloc** steady state: arena + intrusive free-list connection pool
- **Lock-free SPSC ring**: ~1.8M ops/sec (see `bench/bench_spsc`)
- **Observability**: `/metrics` (Prometheus), Grafana dashboards, alerts
- **Correctness**: chaos client + ASan/UBSan/TSan clean, CI on every push

## Architecture

flowchart LR

C["Clients"] -->|"TCP :9100"| K["Kernel SO_REUSEPORT
load balancing"]

K --> W0["Worker 0
epoll + pool (core 0)"]

K --> W1["Worker 1
epoll + pool (core 1)"]

K --> Wn["Worker N
epoll + pool (core N)"]

W0 --> M["workers_snapshot
(merged HDR metrics)"]

W1 --> M

Wn --> M

M --> A["Admin HTTP :9101
/metrics /healthz /ready"]

A -->|"scrape"| P["Prometheus"] --> G["Grafana"]


## Quickstart

git clone [https://github.com/sitareashish/htn](https://github.com/sitareashish/htn) && cd htn

./scripts/[build.sh](http://build.sh) Release

./build/htn 4            # 4 workers; binary :9100, admin :9101

curl -s [localhost:9101/healthz](http://localhost:9101/healthz)


### Full stack (server + Prometheus + Grafana)

docker compose up -d --build

# Grafana: [http://localhost:3000](http://localhost:3000)  (dashboard "HTN Server")


## Protocol
8-byte header, big-endian: `magic(2)=0x4854 | version(1)=1 | type(1) | length(4)`
followed by `length` payload bytes. Types: `PING/PONG/ECHO/ERROR`. See `docs/protocol.md`.

## Benchmarks
Reproduce: `./scripts/bench-matrix.sh` → `bench/results.csv`. Environment and exact
commands in `bench/README.md`. Numbers are `wrk2`/constant-rate (coordinated-omission-corrected).

## Build types
`Debug` | `Release` (-O3 -march=native) | `Asan` (ASan+UBSan) | `Tsan`

## Testing
`./scripts/test.sh Debug && ./scripts/test.sh Asan` — unit suites for net, framer,
pool, events, spsc, hdr, admin, plus `scripts/chaos.py` for adversarial testing.

## License
MIT
