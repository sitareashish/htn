#pragma once

#include <stdint.h>
#include <stddef.h>

/* Records uint64 values (nanoseconds) from 1 to ~ 2^HDR_MAGNITUDES.
 * SUB_BITS controls precision: 2^SUB_BITS sub-buckets per magnitude. */
#define HDR_SUB_BITS    4u                    /* 16 sub-buckets/magnitude */
#define HDR_MAGNITUDES  40u                   /* up to ~2^40 ns ~ 18 min */
#define HDR_SUB         (1u << HDR_SUB_BITS)
#define HDR_BUCKETS     (HDR_MAGNITUDES * HDR_SUB)

typedef struct {
    uint64_t counts[HDR_BUCKETS];
    uint64_t total;      /* number of samples */
    uint64_t sum;        /* sum of values (for mean) */
    uint64_t max;        /* max value seen */
} hdr_t;

void     hdr_reset(hdr_t *h);
void     hdr_record(hdr_t *h, uint64_t value);
void     hdr_merge(hdr_t *dst, const hdr_t *src);   /* dst += src */
uint64_t hdr_percentile(const hdr_t *h, double pct); /* e.g. 99.0 */
