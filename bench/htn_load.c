#define _GNU_SOURCE
#include "htn/hdr.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

/* usage: htn_load <host> <port> <conns> <rate_per_sec> <seconds> */
static const char *HOST; static int PORT, CONNS; static long RATE, SECS;

static uint64_t now_ns(void) {
    struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
    return (uint64_t)t.tv_sec * 1000000000ull + (uint64_t)t.tv_nsec;
}

static int dial(void) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in a = {0};
    a.sin_family = AF_INET; a.sin_port = htons((uint16_t)PORT);
    inet_pton(AF_INET, HOST, &a.sin_addr);
    if (connect(fd, (struct sockaddr *)&a, sizeof a) < 0) { perror("connect"); exit(1); }
    int one = 1; setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof one);
    return fd;
}

typedef struct { int fd; hdr_t h; uint64_t sent, recv; } lc_t;

static void ping(lc_t *c) {
    unsigned char req[8], rsp[64];
    /* PING frame: magic 0x4854, ver 1, type 1, len 0 */
    req[0]=0x48; req[1]=0x54; req[2]=1; req[3]=1; req[4]=req[5]=req[6]=req[7]=0;
    uint64_t t0 = now_ns();
    if (write(c->fd, req, 8) != 8) return;
    ssize_t n = read(c->fd, rsp, sizeof rsp);      /* wait for PONG */
    if (n <= 0) return;
    hdr_record(&c->h, now_ns() - t0);
    c->sent++; c->recv++;
}

static void *runner(void *arg) {
    lc_t *c = arg; c->fd = dial(); hdr_reset(&c->h);
    /* per-connection target rate; open-loop pacing by wall clock */
    double per_conn = (double)RATE / (double)CONNS;
    uint64_t interval = per_conn > 0 ? (uint64_t)(1e9 / per_conn) : 0;
    uint64_t start = now_ns(), deadline = start + (uint64_t)SECS * 1000000000ull;
    uint64_t next = start;
    while (now_ns() < deadline) {
        ping(c);
        next += interval;
        uint64_t t = now_ns();
        if (t < next) { struct timespec s = { .tv_nsec = (long)(next - t) % 1000000000L,
                                               .tv_sec  = (time_t)((next - t) / 1000000000ull) };
                        nanosleep(&s, NULL); }
    }
    return NULL;
}

int main(int argc, char **argv) {
    if (argc < 6) { fprintf(stderr, "usage: %s host port conns rate secs\n", argv[0]); return 2; }
    HOST = argv[1]; PORT = atoi(argv[2]); CONNS = atoi(argv[3]);
    RATE = atol(argv[4]); SECS = atol(argv[5]);

    pthread_t *th = calloc((size_t)CONNS, sizeof *th);
    lc_t *cs = calloc((size_t)CONNS, sizeof *cs);
    uint64_t t0 = now_ns();
    for (int i = 0; i < CONNS; ++i) pthread_create(&th[i], NULL, runner, &cs[i]);
    for (int i = 0; i < CONNS; ++i) pthread_join(th[i], NULL);
    double secs = (double)(now_ns() - t0) / 1e9;

    hdr_t all; hdr_reset(&all); uint64_t total = 0;
    for (int i = 0; i < CONNS; ++i) { hdr_merge(&all, &cs[i].h); total += cs[i].recv; }
    printf("conns=%d target_rate=%ld  achieved=%.0f RPS over %.1fs\n",
           CONNS, RATE, (double)total / secs, secs);
    printf("p50=%.3fms p99=%.3fms p99.9=%.3fms max=%.3fms\n",
           hdr_percentile(&all,50.0)/1e6, hdr_percentile(&all,99.0)/1e6,
           hdr_percentile(&all,99.9)/1e6, (double)all.max/1e6);
    return 0;
}
