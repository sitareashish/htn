#include "munit.h"
#include "htn/events.h"

#include <poll.h>
#include <unistd.h>

static int readable_within(int fd, int ms) {
    struct pollfd pfd = { .fd = fd, .events = POLLIN };
    return poll(&pfd, 1, ms) == 1 && (pfd.revents & POLLIN);
}

static MunitResult test_eventfd_wakeup(const MunitParameter p[], void *f) {
    (void)p; (void)f;
    int efd = ev_make_eventfd();
    munit_assert_int(efd, >=, 0);
    munit_assert_false(readable_within(efd, 50));  /* nothing yet */
    munit_assert_int(ev_eventfd_post(efd), ==, 0);
    munit_assert_true(readable_within(efd, 50));   /* now readable */
    munit_assert_int(ev_eventfd_drain(efd), ==, 0);
    munit_assert_false(readable_within(efd, 50));  /* drained */
    close(efd);
    return MUNIT_OK;
}

static MunitResult test_timerfd_ticks(const MunitParameter p[], void *f) {
    (void)p; (void)f;
    int tfd = ev_make_timerfd(50);   /* 50ms */
    munit_assert_int(tfd, >=, 0);
    munit_assert_true(readable_within(tfd, 200));  /* should fire */
    munit_assert_int(ev_timerfd_drain(tfd), ==, 0);
    close(tfd);
    return MUNIT_OK;
}

static MunitTest tests[] = {
    { "/eventfd_wakeup", test_eventfd_wakeup, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { "/timerfd_ticks",  test_timerfd_ticks,  NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
};
static const MunitSuite suite = { "/events", tests, NULL, 1, MUNIT_SUITE_OPTION_NONE };
int main(int argc, char *argv[]) { return munit_suite_main(&suite, NULL, argc, argv); }
