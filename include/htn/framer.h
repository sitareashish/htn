#pragma once

#include "htn/proto.h"
#include <stddef.h>
#include <stdint.h>

typedef enum { FR_NEED_HEADER, FR_NEED_PAYLOAD } framer_state_t;

typedef struct {
    framer_state_t state;
    uint8_t        hdr[HTN_HDR_SIZE];
    uint32_t       hdr_have;                 /* header bytes accumulated */
    htn_header_t   cur;                       /* decoded header for current frame */
    uint8_t        payload[HTN_MAX_PAYLOAD];
    uint32_t       pay_have;                  /* payload bytes accumulated */
} framer_t;

typedef enum {
    FRAME_NONE  = 0,   /* need more bytes */
    FRAME_READY = 1,   /* a complete frame is available in *out_hdr/out_payload */
    FRAME_ERR   = -1   /* protocol violation; caller should close conn */
} frame_result_t;

void framer_init(framer_t *f);

/* Feed up to *inout_len bytes from buf. On return *inout_len holds the number
 * of bytes consumed. If a complete frame is ready, returns FRAME_READY and
 * fills out_hdr/out_payload (payload points into f->payload, valid until the
 * next framer_push). Call again to drain multiple frames from one buffer. */
frame_result_t framer_push(framer_t *f, const uint8_t *buf, size_t *inout_len,
                           htn_header_t *out_hdr, const uint8_t **out_payload);
