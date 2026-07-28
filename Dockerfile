# ---- build stage ----
FROM ubuntu:24.04 AS build
RUN apt-get update && apt-get install -y --no-install-recommends \
    cmake ninja-build gcc g++ libc6-dev && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY . .
RUN cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
 && cmake --build build --target htn

# ---- runtime stage ----
FROM ubuntu:24.04
RUN apt-get update && apt-get install -y --no-install-recommends \
    libgcc-s1 curl && rm -rf /var/lib/apt/lists/*
COPY --from=build /src/build/htn /usr/local/bin/htn
EXPOSE 9100 9101
# healthcheck hits the Day 11 liveness endpoint
HEALTHCHECK --interval=10s --timeout=2s --retries=3 \
  CMD curl -fs http://localhost:9101/healthz || exit 1
ENTRYPOINT ["/usr/local/bin/htn"]
CMD ["4"]
