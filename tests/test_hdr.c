#include "munit.h"
#include "htn/hdr.h"

/* Known distribution: percentiles must land within HDR's error bound. */
static MunitResult test_percentiles(const MunitParameter p[], void *f) {
    (void)p; (void)f;
    hdr_t h; hdr_reset(&h);
    for (uint64_t v = 1; v <= 10000; ++v) hdr_record(&h, v);  /* uniform 1..10000 */
    munit_assert_uint64(h.total, ==, 10000);

    uint64_t p50 = hdr_percentile(&h, 50.0);
    uint64_t p99 = hdr_percentile(&h, 99.0);
    /* Within ~7% (sub-bucket granularity) of the true value. */
    munit_assert_uint64(p50, >, 4600); munit_assert_uint64(p50, <, 5400);
    munit_assert_uint64(p99, >, 9200); munit_assert_uint64(p99, <, 10001);
    return MUNIT_OK;
}

static MunitResult test_merge_additive(const MunitParameter p[], void *f) {
    (void)p; (void)f;
    hdr_t a, b, m; hdr_reset(&a); hdr_reset(&b); hdr_reset(&m);
    for (int i = 0; i < 1000; ++i) { hdr_record(&a, 100); hdr_record(&b, 200); }
    hdr_merge(&m, &a); hdr_merge(&m, &b);
    munit_assert_uint64(m.total, ==, 2000);
    /* median of {100 x1000, 200 x1000} sits at the boundary ~100..200 */
    uint64_t p50 = hdr_percentile(&m, 50.0);
    munit_assert_uint64(p50, >=, 90); munit_assert_uint64(p50, <=, 210);
    return MUNIT_OK;
}

static MunitResult test_empty(const MunitParameter p[], void *f) {
    (void)p; (void)f;
    hdr_t h; hdr_reset(&h);
    munit_assert_uint64(hdr_percentile(&h, 99.0), ==, 0);
    return MUNIT_OK;
}

static MunitTest tests[] = {
    { "/percentiles",    test_percentiles,   NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { "/merge_additive", test_merge_additive,NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { "/empty",          test_empty,         NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
};
static const MunitSuite suite = { "/hdr", tests, NULL, 1, MUNIT_SUITE_OPTION_NONE };
int main(int argc, char *argv[]) { return munit_suite_main(&suite, NULL, argc, argv); }
