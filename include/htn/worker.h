#pragma once

#include "htn/pool.h"
#include <pthread.h>
#include <stdint.h>

typedef struct {
    int          id;
    uint16_t     port;
    size_t       pool_cap;
    pthread_t    thread;

    /* Owned by the worker thread: */
    int          lfd;       /* SO_REUSEPORT listen socket */
    int          epfd;
    int          wakefd;    /* eventfd: main thread posts here to stop us */
    conn_pool_t  pool;

    /* Simple per-worker counters (merged on Day 10). */
    uint64_t     accepted;
    uint64_t     closed;
} worker_t;

/* Set up the worker's sockets/epoll/pool (called on the main thread). */
int  worker_init(worker_t *w, int id, uint16_t port, size_t pool_cap);

/* pthread entry point; runs the worker's epoll loop until wakefd fires. */
void *worker_main(void *arg);

/* Ask the worker to stop (post its eventfd). */
void worker_stop(worker_t *w);

/* Release the worker's resources after join. */
void worker_destroy(worker_t *w);
