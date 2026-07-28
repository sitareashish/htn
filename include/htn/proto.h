#pragma once

#include <stddef.h>
#include <stdint.h>

#define HTN_MAGIC       0x4854u   /* 'H','T' */
#define HTN_VERSION     1u
#define HTN_HDR_SIZE    8u
#define HTN_MAX_PAYLOAD 65536u    /* 64 KiB cap; reject anything larger */

typedef enum {
    HTN_PING  = 1,
    HTN_PONG  = 2,
    HTN_ECHO  = 3,
    HTN_ERROR = 255
} htn_type_t;

typedef struct {
    uint16_t magic;
    uint8_t  ver;
    uint8_t  type;
    uint32_t payload_len;
} htn_header_t;

/* Serialize a header into an 8-byte big-endian buffer. */
void htn_header_encode(const htn_header_t *h, uint8_t out[HTN_HDR_SIZE]);

/* Parse an 8-byte big-endian buffer into a header.
 * Returns 0 on success, -1 if magic/ver/len are invalid. */
int  htn_header_decode(const uint8_t in[HTN_HDR_SIZE], htn_header_t *out);
