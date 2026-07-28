#include "munit.h"
#include "htn/spsc.h"

#include <pthread.h>
#include <stdint.h>

static MunitResult test_basic(const MunitParameter p[], void *f) {
    (void)p; (void)f;
    spsc_t q; munit_assert_int(spsc_init(&q, 4), ==, 0);
    void *out;
    munit_assert_false(spsc_pop(&q, &out));            /* empty */
    munit_assert_true(spsc_push(&q, (void *)1));
    munit_assert_true(spsc_push(&q, (void *)2));
    munit_assert_true(spsc_push(&q, (void *)3));
    munit_assert_true(spsc_push(&q, (void *)4));
    munit_assert_false(spsc_push(&q, (void *)5));       /* full (cap 4) */
    munit_assert_true(spsc_pop(&q, &out)); munit_assert_ptr_equal(out, (void*)1);
    munit_assert_true(spsc_push(&q, (void *)5));        /* room again */
    spsc_destroy(&q);
    return MUNIT_OK;
}

static MunitResult test_reject_non_pow2(const MunitParameter p[], void *f) {
    (void)p; (void)f;
    spsc_t q;
    munit_assert_int(spsc_init(&q, 6), ==, -1);   /* not a power of two */
    return MUNIT_OK;
}

#define STRESS (2u * 1000u * 1000u)
static spsc_t sq;
static void *stress_producer(void *arg) {
    (void)arg;
    for (uintptr_t i = 1; i <= STRESS; ++i)
        while (!spsc_push(&sq, (void *)i)) ;
    return NULL;
}
static MunitResult test_threaded(const MunitParameter p[], void *f) {
    (void)p; (void)f;
    munit_assert_int(spsc_init(&sq, 1024), ==, 0);
    pthread_t prod; pthread_create(&prod, NULL, stress_producer, NULL);
    uintptr_t expect = 1; void *item; uint64_t got = 0;
    while (got < STRESS) {
        if (spsc_pop(&sq, &item)) {
            munit_assert_ullong((unsigned long long)(uintptr_t)item, ==,
                                (unsigned long long)expect);  /* FIFO order */
            ++expect; ++got;
        }
    }
    pthread_join(prod, NULL);
    spsc_destroy(&sq);
    return MUNIT_OK;
}

static MunitTest tests[] = {
    { "/basic",          test_basic,          NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { "/reject_non_pow2",test_reject_non_pow2,NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { "/threaded",       test_threaded,       NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
};
static const MunitSuite suite = { "/spsc", tests, NULL, 1, MUNIT_SUITE_OPTION_NONE };
int main(int argc, char *argv[]) { return munit_suite_main(&suite, NULL, argc, argv); }
