#include "htn/spsc.h"

#include <stdlib.h>

static bool is_pow2(size_t x) { return x && ((x & (x - 1)) == 0); }

int spsc_init(spsc_t *q, size_t capacity) {
    if (!is_pow2(capacity)) return -1;
    q->buf = calloc(capacity, sizeof *q->buf);
    if (!q->buf) return -1;
    q->mask = capacity - 1;
    atomic_store_explicit(&q->head, 0, memory_order_relaxed);
    atomic_store_explicit(&q->tail, 0, memory_order_relaxed);
    return 0;
}

void spsc_destroy(spsc_t *q) {
    free(q->buf);
    q->buf = NULL;
}
