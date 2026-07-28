#include "munit.h"
#include "htn/conn.h"

#include <unistd.h>

static MunitResult test_new_initial_state(const MunitParameter p[], void *f) {
    (void)p; (void)f;
    conn_t *c = conn_new(7);
    munit_assert_not_null(c);
    munit_assert_int(c->fd, ==, 7);
    munit_assert_int(c->state, ==, CONN_READING);
    munit_assert_uint32(c->rx_len, ==, 0);
    munit_assert_uint32(c->tx_len, ==, 0);
    munit_assert_uint32(c->tx_off, ==, 0);
    conn_free(c);
    return MUNIT_OK;
}

static MunitResult test_buffers_are_sized(const MunitParameter p[], void *f) {
    (void)p; (void)f;
    conn_t *c = conn_new(3);
    munit_assert_size(sizeof c->rx_buf, ==, CONN_BUF_SZ);
    munit_assert_size(sizeof c->tx_buf, ==, CONN_BUF_SZ);
    conn_free(c);
    return MUNIT_OK;
}

static MunitTest tests[] = {
    { "/new_initial_state", test_new_initial_state, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { "/buffers_are_sized", test_buffers_are_sized, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
};
static const MunitSuite suite = { "/conn", tests, NULL, 1, MUNIT_SUITE_OPTION_NONE };
int main(int argc, char *argv[]) { return munit_suite_main(&suite, NULL, argc, argv); }
