#include "munit.h"
#include "htn/admin.h"
#include "htn/worker.h"
#include "htn/hdr.h"

#include <string.h>

/* render_metrics is static; expose a testable wrapper via a small shim, or
 * compile admin.c with a TEST macro. Simplest: declare it here. */
int htn_test_render_metrics(worker_t *ws, int n, char *buf, size_t cap);

static MunitResult test_metrics_format(const MunitParameter p[], void *f) {
    (void)p; (void)f;
    worker_t w; memset(&w, 0, sizeof w);
    hdr_reset(&w.m.latency);
    for (int i = 0; i < 100; ++i) hdr_record(&w.m.latency, 5000);  /* 5us */
    w.m.requests = 100; w.m.bytes_in = 800; w.m.bytes_out = 800;

    char buf[4096];
    int n = htn_test_render_metrics(&w, 1, buf, sizeof buf);
    munit_assert_int(n, >, 0);
    munit_assert_not_null(strstr(buf, "# TYPE htn_requests_total counter"));
    munit_assert_not_null(strstr(buf, "htn_requests_total 100"));
    munit_assert_not_null(strstr(buf, "quantile=\"0.99\""));
    munit_assert_not_null(strstr(buf, "htn_request_latency_seconds_count 100"));
    return MUNIT_OK;
}

static MunitTest tests[] = {
    { "/metrics_format", test_metrics_format, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
};
static const MunitSuite suite = { "/admin", tests, NULL, 1, MUNIT_SUITE_OPTION_NONE };
int main(int argc, char *argv[]) { return munit_suite_main(&suite, NULL, argc, argv); }
