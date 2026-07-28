#define _GNU_SOURCE
#include "htn/spsc.h"

#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define N   (50u * 1000u * 1000u)   /* 50M items */
#define CAP (1u << 14)              /* 16384 */

static spsc_t q;

static void pin(int core) {
    cpu_set_t s; CPU_ZERO(&s); CPU_SET(core, &s);
    pthread_setaffinity_np(pthread_self(), sizeof s, &s);
}

static void *producer(void *arg) {
    (void)arg; pin(0);
    for (uintptr_t i = 1; i <= N; ++i)
        while (!spsc_push(&q, (void *)i)) sched_yield();
    return NULL;
}

int main(void) {
    spsc_init(&q, CAP);
    pthread_t prod;
    struct timespec t0, t1;

    clock_gettime(CLOCK_MONOTONIC, &t0);
    pthread_create(&prod, NULL, producer, NULL);

    pin(1);
    uint64_t sum = 0, got = 0; void *item;
    while (got < N) {
        if (spsc_pop(&q, &item)) { sum += (uintptr_t)item; ++got; }
        else sched_yield();
    }
    pthread_join(prod, NULL);
    clock_gettime(CLOCK_MONOTONIC, &t1);

    double secs = (double)(t1.tv_sec - t0.tv_sec)
                + (double)(t1.tv_nsec - t0.tv_nsec) / 1e9;
    double ops  = (double)N / secs;
    /* checksum guards against the optimizer deleting the loop */
    printf("items=%u sum=%llu  %.2f M ops/sec  %.2f ns/op\n",
           N, (unsigned long long)sum, ops / 1e6, 1e9 / ops);
    spsc_destroy(&q);
    return 0;
}
