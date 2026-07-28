#pragma once

#include "htn/framer.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CONN_BUF_SZ 65536u   /* now big enough for a full HTN payload */

typedef enum { CONN_READING, CONN_WRITING, CONN_CLOSING } conn_state_t;

typedef struct conn {
    struct conn *next;                 /* intrusive free-list link (free slots) */
    bool         in_use;
    int          fd;
    conn_state_t state;
    uint32_t     rx_len;
    uint32_t     tx_off;
    uint32_t     tx_len;
    framer_t     fr;
    uint8_t      rx_buf[CONN_BUF_SZ];
    uint8_t      tx_buf[CONN_BUF_SZ];
} conn_t;
