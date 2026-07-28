#pragma once

#include "htn/worker.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>

typedef struct {
    uint16_t   port;         /* admin port, e.g. 9101 */
    worker_t  *workers;      /* borrowed: for snapshot */
    int        nworkers;
    atomic_int ready;        /* 0 = not ready, 1 = ready */
    atomic_int stop;         /* set to stop the admin loop */
    pthread_t  thread;
    int        lfd;          /* admin listen socket */
} admin_t;

int  admin_start(admin_t *a, uint16_t port, worker_t *ws, int n);
void admin_set_ready(admin_t *a, int ready);
void admin_stop(admin_t *a);
