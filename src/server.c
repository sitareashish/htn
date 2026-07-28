#include "htn/server.h"
#include "htn/log.h"
#include "htn/net.h"

#include <errno.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_EVENTS 64
#define RX_BUF_SZ  4096

/* Accept every pending connection (drain the accept queue), register each
 * with the epoll set for read readiness. */
static void accept_new(int epfd, int lfd) {
    for (;;) {
        int cfd = accept(lfd, NULL, NULL);
        if (cfd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) return; /* drained */
            if (errno == EINTR) continue;
            log_error("accept: %s", strerror(errno));
            return;
        }
        if (net_set_nonblocking(cfd) < 0) {
            log_error("set_nonblocking: %s", strerror(errno));
            close(cfd);
            continue;
        }
        net_set_nodelay(cfd);

        struct epoll_event ev = { .events = EPOLLIN, .data.fd = cfd };
        if (epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev) < 0) {
            log_error("epoll_ctl ADD: %s", strerror(errno));
            close(cfd);
            continue;
        }
        log_info("accepted fd=%d", cfd);
    }
}

/* Read whatever is available and echo it back. Returns 0 to keep the
 * connection, -1 if it should be closed. */
static int serve_client(int cfd) {
    char buf[RX_BUF_SZ];
    ssize_t n = read(cfd, buf, sizeof buf);
    if (n == 0) {
        log_info("client closed fd=%d", cfd);
        return -1;
    }
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) return 0; /* spurious */
        if (errno == EINTR) return 0;
        log_error("read fd=%d: %s", cfd, strerror(errno));
        return -1;
    }
    /* Echo. In LT mode a short write leaves data unsent, but for a local
     * echo of <=4KB the socket buffer almost never fills; we harden this
     * with a proper TX buffer on Day 4. */
    ssize_t off = 0;
    while (off < n) {
        ssize_t w = write(cfd, buf + off, (size_t)(n - off));
        if (w < 0) {
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) break; /* deferred */
            log_error("write fd=%d: %s", cfd, strerror(errno));
            return -1;
        }
        off += w;
    }
    return 0;
}

static void close_client(int epfd, int cfd) {
    epoll_ctl(epfd, EPOLL_CTL_DEL, cfd, NULL); /* DEL before close */
    close(cfd);
}

int server_run(int lfd, volatile sig_atomic_t *stop_flag) {
    if (net_set_nonblocking(lfd) < 0) {
        log_error("listen fd nonblocking: %s", strerror(errno));
        return -1;
    }

    int epfd = epoll_create1(EPOLL_CLOEXEC);
    if (epfd < 0) {
        log_error("epoll_create1: %s", strerror(errno));
        return -1;
    }

    struct epoll_event lev = { .events = EPOLLIN, .data.fd = lfd };
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &lev) < 0) {
        log_error("epoll_ctl ADD listen: %s", strerror(errno));
        close(epfd);
        return -1;
    }

    struct epoll_event events[MAX_EVENTS];
    while (!*stop_flag) {
        int n = epoll_wait(epfd, events, MAX_EVENTS, 250 /*ms*/);
        if (n < 0) {
            if (errno == EINTR) continue; /* signal (e.g. SIGINT) */
            log_error("epoll_wait: %s", strerror(errno));
            break;
        }
        for (int i = 0; i < n; ++i) {
            int fd = events[i].data.fd;
            uint32_t ev = events[i].events;

            if (fd == lfd) {
                accept_new(epfd, lfd);
                continue;
            }
            if (ev & (EPOLLERR | EPOLLHUP)) {
                close_client(epfd, fd);
                continue;
            }
            if (ev & EPOLLIN) {
                if (serve_client(fd) < 0) close_client(epfd, fd);
            }
        }
    }

    close(epfd);
    return 0;
}
