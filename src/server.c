#include "htn/server.h"
#include "htn/conn.h"
#include "htn/log.h"
#include "htn/net.h"

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_EVENTS 64

/* Base flags for a client: edge-triggered, oneshot, watch read + peer-close. */
#define CLIENT_BASE (EPOLLET | EPOLLONESHOT | EPOLLRDHUP)

static int arm(int epfd, int op, conn_t *c, uint32_t rw) {
    struct epoll_event ev = { .events = CLIENT_BASE | rw, .data.ptr = c };
    return epoll_ctl(epfd, op, c->fd, &ev);
}

static void close_conn(int epfd, conn_t *c) {
    epoll_ctl(epfd, EPOLL_CTL_DEL, c->fd, NULL);
    close(c->fd);
    conn_free(c);
}

/* Drain the accept queue completely — mandatory with edge-triggered. */
static void accept_new(int epfd, int lfd) {
    for (;;) {
        int cfd = accept(lfd, NULL, NULL);
        if (cfd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) return;
            if (errno == EINTR) continue;
            log_error("accept: %s", strerror(errno));
            return;
        }
        if (net_set_nonblocking(cfd) < 0) { close(cfd); continue; }
        net_set_nodelay(cfd);

        conn_t *c = conn_new(cfd);
        if (!c) { close(cfd); continue; }

        if (arm(epfd, EPOLL_CTL_ADD, c, EPOLLIN) < 0) {
            log_error("epoll_ctl ADD: %s", strerror(errno));
            close(cfd); conn_free(c); continue;
        }
        log_info("accepted fd=%d", cfd);
    }
}

/* Read until EAGAIN into rx_buf. Returns -1 on close, 0 otherwise. */
static int do_read(conn_t *c) {
    for (;;) {
        if (c->rx_len == CONN_BUF_SZ) return 0; /* buffer full; flush first */
        ssize_t n = read(c->fd, c->rx_buf + c->rx_len, CONN_BUF_SZ - c->rx_len);
        if (n == 0) return -1;                  /* orderly close */
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) return 0; /* drained */
            if (errno == EINTR) continue;
            log_error("read fd=%d: %s", c->fd, strerror(errno));
            return -1;
        }
        c->rx_len += (uint32_t)n;
    }
}

/* Move rx bytes into tx (the "echo" handler). */
static void do_process(conn_t *c) {
    if (c->rx_len == 0) return;
    memcpy(c->tx_buf, c->rx_buf, c->rx_len);
    c->tx_len = c->rx_len;
    c->tx_off = 0;
    c->rx_len = 0;
    c->state  = CONN_WRITING;
}

/* Write until EAGAIN. Returns -1 on error, 0 otherwise.
 * Sets *want_out if there are still bytes to flush. */
static int do_write(conn_t *c, int *want_out) {
    *want_out = 0;
    while (c->tx_off < c->tx_len) {
        ssize_t w = write(c->fd, c->tx_buf + c->tx_off, c->tx_len - c->tx_off);
        if (w < 0) {
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) { *want_out = 1; return 0; }
            log_error("write fd=%d: %s", c->fd, strerror(errno));
            return -1;
        }
        c->tx_off += (uint32_t)w;
    }
    c->tx_len = c->tx_off = 0;
    c->state  = CONN_READING;
    return 0;
}

static void handle_client(int epfd, conn_t *c, uint32_t ev) {
    if (ev & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) { close_conn(epfd, c); return; }

    if (ev & EPOLLIN) {
        if (do_read(c) < 0) { close_conn(epfd, c); return; }
        do_process(c);
    }

    int want_out = 0;
    if (c->tx_len > c->tx_off) {
        if (do_write(c, &want_out) < 0) { close_conn(epfd, c); return; }
    }

    /* Re-arm EPOLLONESHOT — the non-negotiable step. */
    uint32_t rw = EPOLLIN;
    if (want_out) rw |= EPOLLOUT;
    if (arm(epfd, EPOLL_CTL_MOD, c, rw) < 0) {
        log_error("epoll_ctl MOD: %s", strerror(errno));
        close_conn(epfd, c);
    }
}

int server_run(int lfd, volatile sig_atomic_t *stop_flag) {
    if (net_set_nonblocking(lfd) < 0) { log_error("listen nonblock: %s", strerror(errno)); return -1; }

    int epfd = epoll_create1(EPOLL_CLOEXEC);
    if (epfd < 0) { log_error("epoll_create1: %s", strerror(errno)); return -1; }

    /* Listen fd: level-triggered is fine here and simpler, tagged data.ptr=NULL. */
    struct epoll_event lev = { .events = EPOLLIN, .data.ptr = NULL };
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &lev) < 0) {
        log_error("epoll_ctl ADD listen: %s", strerror(errno));
        close(epfd); return -1;
    }

    struct epoll_event events[MAX_EVENTS];
    while (!*stop_flag) {
        int n = epoll_wait(epfd, events, MAX_EVENTS, 250);
        if (n < 0) { if (errno == EINTR) continue; log_error("epoll_wait: %s", strerror(errno)); break; }
        for (int i = 0; i < n; ++i) {
            conn_t *c = events[i].data.ptr;
            if (c == NULL) { accept_new(epfd, lfd); continue; }
            handle_client(epfd, c, events[i].events);
        }
    }

    close(epfd);
    return 0;
}
