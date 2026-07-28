#include "htn/pool.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

int pool_init(conn_pool_t *p, size_t capacity) {
    p->slots = calloc(capacity, sizeof *p->slots);
    if (!p->slots) return -1;
    p->capacity = capacity;
    p->in_use   = 0;

    /* Thread the free-list: slot[i].next = &slot[i+1], last -> NULL. */
    for (size_t i = 0; i + 1 < capacity; ++i)
        p->slots[i].next = &p->slots[i + 1];
    p->slots[capacity - 1].next = NULL;
    p->free_head = &p->slots[0];
    return 0;
}

void pool_destroy(conn_pool_t *p) {
    free(p->slots);
    p->slots = NULL;
    p->free_head = NULL;
    p->capacity = p->in_use = 0;
}

conn_t *pool_acquire(conn_pool_t *p, int fd) {
    conn_t *c = p->free_head;
    if (!c) return NULL;                 /* exhausted */
    p->free_head = c->next;

    /* Reset the slot to a clean live connection. */
    conn_t *saved_next = NULL;           /* not needed once in use */
    (void)saved_next;
    memset(c, 0, sizeof *c);
    c->in_use = true;
    c->fd     = fd;
    c->state  = CONN_READING;
    framer_init(&c->fr);

    p->in_use++;
    return c;
}

void pool_release(conn_pool_t *p, conn_t *c) {
    assert(c->in_use && "double free / release of idle conn");
    c->in_use = false;
    c->fd     = -1;
    c->next       = p->free_head;
    p->free_head  = c;
    p->in_use--;
}
