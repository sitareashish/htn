#pragma once

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifndef HTN_CACHELINE
#define HTN_CACHELINE 64
#endif

typedef struct {
    void   **buf;         /* capacity slots */
    size_t   mask;        /* capacity - 1 (capacity is power of two) */

    /* Producer's index and consumer's index on separate cache lines. */
    alignas(HTN_CACHELINE) _Atomic size_t head;   /* written by producer */
    alignas(HTN_CACHELINE) _Atomic size_t tail;   /* written by consumer */
    alignas(HTN_CACHELINE) char _pad;             /* isolate from neighbors */
} spsc_t;

/* capacity MUST be a power of two. Returns 0 / -1. */
int  spsc_init(spsc_t *q, size_t capacity);
void spsc_destroy(spsc_t *q);

/* Producer side. Returns false if full. */
static inline bool spsc_push(spsc_t *q, void *item) {
    size_t head = atomic_load_explicit(&q->head, memory_order_relaxed);
    size_t next = head + 1;
    size_t tail = atomic_load_explicit(&q->tail, memory_order_acquire);
    if (next - tail > q->mask + 1) return false;   /* full */
    q->buf[head & q->mask] = item;
    atomic_store_explicit(&q->head, next, memory_order_release);
    return true;
}

/* Consumer side. Returns false if empty. */
static inline bool spsc_pop(spsc_t *q, void **out) {
    size_t tail = atomic_load_explicit(&q->tail, memory_order_relaxed);
    size_t head = atomic_load_explicit(&q->head, memory_order_acquire);
    if (head == tail) return false;                /* empty */
    *out = q->buf[tail & q->mask];
    atomic_store_explicit(&q->tail, tail + 1, memory_order_release);
    return true;
}
