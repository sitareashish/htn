#include "htn/framer.h"

#include <string.h>

void framer_init(framer_t *f) {
    f->state    = FR_NEED_HEADER;
    f->hdr_have = 0;
    f->pay_have = 0;
}

static uint32_t take(uint8_t *dst, uint32_t have, uint32_t need,
                     const uint8_t *src, size_t avail, uint32_t *consumed) {
    uint32_t want = need - have;
    uint32_t n    = (avail < want) ? (uint32_t)avail : want;
    memcpy(dst + have, src, n);
    *consumed = n;
    return have + n;
}

frame_result_t framer_push(framer_t *f, const uint8_t *buf, size_t *inout_len,
                           htn_header_t *out_hdr, const uint8_t **out_payload) {
    size_t off = 0, len = *inout_len;

    while (off < len) {
        if (f->state == FR_NEED_HEADER) {
            uint32_t consumed = 0;
            f->hdr_have = take(f->hdr, f->hdr_have, HTN_HDR_SIZE,
                               buf + off, len - off, &consumed);
            off += consumed;
            if (f->hdr_have < HTN_HDR_SIZE) break;   /* need more */

            if (htn_header_decode(f->hdr, &f->cur) < 0) {
                *inout_len = off;
                return FRAME_ERR;
            }
            f->pay_have = 0;
            f->state = FR_NEED_PAYLOAD;
        }

        if (f->state == FR_NEED_PAYLOAD) {
            if (f->cur.payload_len == 0) {
                /* zero-length payload frame (e.g. PING) is complete now */
                *out_hdr     = f->cur;
                *out_payload = f->payload;
                f->state     = FR_NEED_HEADER;
                f->hdr_have  = 0;
                *inout_len   = off;
                return FRAME_READY;
            }
            uint32_t consumed = 0;
            f->pay_have = take(f->payload, f->pay_have, f->cur.payload_len,
                               buf + off, len - off, &consumed);
            off += consumed;
            if (f->pay_have < f->cur.payload_len) break; /* need more */

            *out_hdr     = f->cur;
            *out_payload = f->payload;
            f->state     = FR_NEED_HEADER;
            f->hdr_have  = 0;
            *inout_len   = off;
            return FRAME_READY;
        }
    }

    *inout_len = off;
    return FRAME_NONE;
}
