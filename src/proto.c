#include "htn/proto.h"

#include <string.h>

static void put_u16(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)(v >> 8);
    p[1] = (uint8_t)(v & 0xFF);
}
static void put_u32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)(v & 0xFF);
}
static uint16_t get_u16(const uint8_t *p) {
    return (uint16_t)((uint16_t)p[0] << 8 | (uint16_t)p[1]);
}
static uint32_t get_u32(const uint8_t *p) {
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 |
           (uint32_t)p[2] << 8  | (uint32_t)p[3];
}

void htn_header_encode(const htn_header_t *h, uint8_t out[HTN_HDR_SIZE]) {
    put_u16(out,     h->magic);
    out[2] = h->ver;
    out[3] = h->type;
    put_u32(out + 4, h->payload_len);
}

int htn_header_decode(const uint8_t in[HTN_HDR_SIZE], htn_header_t *out) {
    out->magic       = get_u16(in);
    out->ver         = in[2];
    out->type        = in[3];
    out->payload_len = get_u32(in + 4);

    if (out->magic != HTN_MAGIC)          return -1;
    if (out->ver   != HTN_VERSION)        return -1;
    if (out->payload_len > HTN_MAX_PAYLOAD) return -1;
    return 0;
}
