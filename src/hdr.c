#include "htn/hdr.h"

#include <string.h>

/* Bucket index: magnitude = floor(log2(value)); within a magnitude,
 * take the top HDR_SUB_BITS bits below the leading bit as the sub-bucket. */
static unsigned bucket_of(uint64_t v) {
    if (v == 0) return 0;
    unsigned magnitude = 63u - (unsigned)__builtin_clzll(v);
    unsigned sub;
    if (magnitude < HDR_SUB_BITS) {
        sub = (unsigned)v & (HDR_SUB - 1);
        return sub;                       /* first magnitudes are linear */
    }
    unsigned shift = magnitude - HDR_SUB_BITS;
    sub = (unsigned)((v >> shift) & (HDR_SUB - 1));
    unsigned idx = magnitude * HDR_SUB + sub;
    return (idx < HDR_BUCKETS) ? idx : HDR_BUCKETS - 1;
}

/* Representative (lower-bound) value for a bucket, for percentile output. */
static uint64_t value_of(unsigned idx) {
    unsigned magnitude = idx / HDR_SUB;
    unsigned sub       = idx % HDR_SUB;
    if (magnitude < HDR_SUB_BITS) return sub;
    unsigned shift = magnitude - HDR_SUB_BITS;
    return ((uint64_t)HDR_SUB + sub) << shift;
}

void hdr_reset(hdr_t *h) { memset(h, 0, sizeof *h); }

void hdr_record(hdr_t *h, uint64_t value) {
    h->counts[bucket_of(value)]++;
    h->total++;
    h->sum += value;
    if (value > h->max) h->max = value;
}

void hdr_merge(hdr_t *dst, const hdr_t *src) {
    for (unsigned i = 0; i < HDR_BUCKETS; ++i) dst->counts[i] += src->counts[i];
    dst->total += src->total;
    dst->sum   += src->sum;
    if (src->max > dst->max) dst->max = src->max;
}

uint64_t hdr_percentile(const hdr_t *h, double pct) {
    if (h->total == 0) return 0;
    uint64_t rank = (uint64_t)((pct / 100.0) * (double)h->total + 0.5);
    if (rank == 0) rank = 1;
    uint64_t cum = 0;
    for (unsigned i = 0; i < HDR_BUCKETS; ++i) {
        cum += h->counts[i];
        if (cum >= rank) return value_of(i);
    }
    return h->max;
}
