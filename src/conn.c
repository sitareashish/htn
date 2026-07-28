#include "htn/conn.h"

#include <stdlib.h>
#include <string.h>

conn_t *conn_new(int fd) {
    conn_t *c = calloc(1, sizeof *c);
    if (!c) return NULL;
    c->fd    = fd;
    c->state = CONN_READING;
    return c;
}

void conn_free(conn_t *c) {
    free(c);
}
