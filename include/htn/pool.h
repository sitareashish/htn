#pragma once

#include "htn/conn.h"
#include <stddef.h>

typedef struct {
    conn_t *slots;        /* arena: capacity contiguous conn_t */
    conn_t *free_head;    /* head of intrusive free-list */
    size_t  capacity;
    size_t  in_use;       /* live connection count (for metrics) */
} conn_pool_t;

/* Allocate the arena and thread the free-list. Returns 0 / -1. */
int  pool_init(conn_pool_t *p, size_t capacity);

/* Free the whole arena at once. */
void pool_destroy(conn_pool_t *p);

/* O(1) pop. Returns an initialized conn_t for fd, or NULL if exhausted. */
conn_t *pool_acquire(conn_pool_t *p, int fd);

/* O(1) push back onto the free-list. */
void pool_release(conn_pool_t *p, conn_t *c);
