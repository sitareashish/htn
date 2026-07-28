#pragma once

#include <stddef.h>
#include <stdint.h>

#define CONN_BUF_SZ 4096

typedef enum {
    CONN_READING,
    CONN_WRITING,
    CONN_CLOSING
} conn_state_t;

typedef struct conn {
    int          fd;
    conn_state_t state;
    uint32_t     rx_len;              /* bytes waiting to be echoed */
    uint32_t     tx_off;             /* bytes of tx already written */
    uint32_t     tx_len;             /* total bytes to write */
    uint8_t      rx_buf[CONN_BUF_SZ];
    uint8_t      tx_buf[CONN_BUF_SZ];
} conn_t;

conn_t *conn_new(int fd);
void    conn_free(conn_t *c);
