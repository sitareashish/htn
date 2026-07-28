#include "htn/worker.h"
#include "htn/conn.h"
#include "htn/events.h"
#include "htn/log.h"
#include "htn/net.h"

#include <errno.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

#define MAX_EVENTS 512

/* Tag infra fds vs client conns on the same epoll set. */
enum fd_kind { KIND_LISTEN = 1, KIND_WAKE = 2 };
typedef struct { enum fd_kind kind; } infra_tag;

static infra_tag TAG_LISTEN = { KIND_LISTEN };
static infra_tag TAG_WAKE   = { KIND_WAKE };

static int ep_add(int epfd, int fd, uint32_t ev, void *ptr) {
    struct epoll_event e = { .events = ev, .data = { .ptr = ptr } };
    return epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &e);
}

int worker_init(worker_t *w, int id, uint16_t port, size_t pool_cap) {
    memset(w, 0, sizeof *w);
    w->id = id; w->port = port; w->pool_cap = pool_cap;

    w->lfd = net_make_reuseport_socket(port);
    if (w->lfd < 0) return -1;
    w->epfd = epoll_create1(EPOLL_CLOEXEC);
    if (w->epfd < 0) return -1;
    w->wakefd = ev_make_eventfd();
    if (w->wakefd < 0) return -1;
    if (pool_init(&w->pool, pool_cap) < 0) return -1;

    /* Listen socket is level-triggered here for simplicity; clients are ET. */
    if (ep_add(w->epfd, w->lfd,    EPOLLIN, &TAG_LISTEN) < 0) return -1;
    if (ep_add(w->epfd, w->wakefd, EPOLLIN, &TAG_WAKE)   < 0) return -1;
    return 0;
}

void *worker_main(void *arg) {
    worker_t *w = arg;
    struct epoll_event events[MAX_EVENTS];
    log_info("worker %d up on port %u", w->id, w->port);

    for (;;) {
        int n = epoll_wait(w->epfd, events, MAX_EVENTS, -1);
        if (n < 0) { if (errno == EINTR) continue; break; }
        for (int i = 0; i < n; ++i) {
            void *ptr = events[i].data.ptr;
            if (ptr == &TAG_WAKE) {
                ev_eventfd_drain(w->wakefd);
                goto done;
            }
            if (ptr == &TAG_LISTEN) {
                /* accept_new: drain the accept queue, pool_acquire each,
                   arm client fd EPOLLIN|EPOLLET|EPOLLONESHOT. */
                worker_accept_all(w);
                continue;
            }
            /* else ptr is a conn_t* client */
            worker_handle_client(w, (conn_t *)ptr, events[i].events);
        }
    }
done:
    log_info("worker %d stopping (accepted=%lu closed=%lu)",
             w->id, (unsigned long)w->accepted, (unsigned long)w->closed);
    return NULL;
}

void worker_stop(worker_t *w)    { ev_eventfd_post(w->wakefd); }
void worker_destroy(worker_t *w) {
    pool_destroy(&w->pool);
    if (w->lfd    >= 0) close(w->lfd);
    if (w->wakefd >= 0) close(w->wakefd);
    if (w->epfd   >= 0) close(w->epfd);
}
