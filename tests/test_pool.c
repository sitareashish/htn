#include "munit.h"
#include "htn/pool.h"

static MunitResult test_acquire_release(const MunitParameter p[], void *f) {
    (void)p; (void)f;
    conn_pool_t pool;
    munit_assert_int(pool_init(&pool, 4), ==, 0);
    munit_assert_size(pool.in_use, ==, 0);

    conn_t *a = pool_acquire(&pool, 10);
    conn_t *b = pool_acquire(&pool, 11);
    munit_assert_not_null(a);
    munit_assert_not_null(b);
    munit_assert_int(a->fd, ==, 10);
    munit_assert_true(a->in_use);
    munit_assert_size(pool.in_use, ==, 2);

    pool_release(&pool, a);
    munit_assert_size(pool.in_use, ==, 1);
    munit_assert_false(a->in_use);

    /* Releasing then re-acquiring should reuse the freed slot. */
    conn_t *c = pool_acquire(&pool, 12);
    munit_assert_ptr_equal(c, a);   /* LIFO free-list returns the last freed */
    munit_assert_int(c->fd, ==, 12);

    pool_destroy(&pool);
    return MUNIT_OK;
}

static MunitResult test_exhaustion(const MunitParameter p[], void *f) {
    (void)p; (void)f;
    conn_pool_t pool;
    munit_assert_int(pool_init(&pool, 2), ==, 0);
    conn_t *a = pool_acquire(&pool, 1);
    conn_t *b = pool_acquire(&pool, 2);
    conn_t *c = pool_acquire(&pool, 3);   /* should fail */
    munit_assert_not_null(a);
    munit_assert_not_null(b);
    munit_assert_null(c);                 /* exhausted -> NULL, no crash */
    pool_release(&pool, a);
    conn_t *d = pool_acquire(&pool, 4);   /* now succeeds again */
    munit_assert_not_null(d);
    pool_destroy(&pool);
    return MUNIT_OK;
}

static MunitResult test_clean_slot(const MunitParameter p[], void *f) {
    (void)p; (void)f;
    conn_pool_t pool;
    munit_assert_int(pool_init(&pool, 2), ==, 0);
    conn_t *a = pool_acquire(&pool, 1);
    a->rx_len = 1234;
    a->tx_len = 5678;
    pool_release(&pool, a);
    conn_t *b = pool_acquire(&pool, 2);  /* same slot, must be zeroed */
    munit_assert_ptr_equal(a, b);
    munit_assert_uint32(b->rx_len, ==, 0);
    munit_assert_uint32(b->tx_len, ==, 0);
    pool_destroy(&pool);
    return MUNIT_OK;
}

static MunitTest tests[] = {
    { "/acquire_release", test_acquire_release, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { "/exhaustion",      test_exhaustion,      NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { "/clean_slot",      test_clean_slot,      NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
};
static const MunitSuite suite = { "/pool", tests, NULL, 1, MUNIT_SUITE_OPTION_NONE };
int main(int argc, char *argv[]) { return munit_suite_main(&suite, NULL, argc, argv); }
