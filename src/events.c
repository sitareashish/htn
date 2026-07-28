#include "htn/events.h"

#include <signal.h>
#include <stdint.h>
#include <sys/eventfd.h>
#include <sys/signalfd.h>
#include <sys/timerfd.h>
#include <time.h>
#include <unistd.h>

int ev_make_signalfd(void) {
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    /* Block so signals are delivered via the fd, not the default handler. */
    if (pthread_sigmask(SIG_BLOCK, &mask, NULL) != 0) return -1;
    return signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
}

int ev_make_eventfd(void) {
    return eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
}

int ev_eventfd_post(int efd) {
    uint64_t one = 1;
    return (write(efd, &one, sizeof one) == (ssize_t)sizeof one) ? 0 : -1;
}

int ev_eventfd_drain(int efd) {
    uint64_t val;
    return (read(efd, &val, sizeof val) == (ssize_t)sizeof val) ? 0 : -1;
}

int ev_make_timerfd(unsigned interval_ms) {
    int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (tfd < 0) return -1;
    struct itimerspec its = {
        .it_interval = { .tv_sec = interval_ms / 1000,
                         .tv_nsec = (long)(interval_ms % 1000) * 1000000L },
        .it_value    = { .tv_sec = interval_ms / 1000,
                         .tv_nsec = (long)(interval_ms % 1000) * 1000000L },
    };
    if (timerfd_settime(tfd, 0, &its, NULL) < 0) { close(tfd); return -1; }
    return tfd;
}

int ev_timerfd_drain(int tfd) {
    uint64_t expirations;
    return (read(tfd, &expirations, sizeof expirations) > 0) ? 0 : -1;
}
