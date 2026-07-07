// src/main.c — Day 1: blocking, single-connection TCP echo server.
// Deliberately serves one client at a time. Fix that from Day 3.

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT           9100
#define BACKLOG        128       // kernel accept queue depth (see listen(2))
#define RX_BUF_SZ      4096

static volatile sig_atomic_t g_stop = 0;
static void on_sigint(int _s) { (void)_s; g_stop = 1; }

static void die(const char *what) {
    fprintf(stderr, "fatal: %s: %s\\n", what, strerror(errno));
    exit(EXIT_FAILURE);
}

static int make_listen_socket(uint16_t port) {
    // 1. socket()   — ask the kernel for a TCP socket handle.
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd < 0) die("socket");

    // Allow immediate rebind after restart (TIME_WAIT stragglers).
    int one = 1;
    if (setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one) < 0)
        die("setsockopt SO_REUSEADDR");

    // 2. bind()  — attach the socket to (0.0.0.0, port).
    struct sockaddr_in addr = {
        .sin_family      = AF_INET,
        .sin_port        = htons(port),          // host → network byte order
        .sin_addr.s_addr = htonl(INADDR_ANY),    // 0.0.0.0 (all interfaces)
    };
    if (bind(lfd, (struct sockaddr *)&addr, sizeof addr) < 0)
        die("bind");

    // 3. listen() — start accepting into a kernel queue of size BACKLOG.
    if (listen(lfd, BACKLOG) < 0) die("listen");

    return lfd;
}

static void serve_one_client(int cfd) {
    // Optional: disable Nagle so tiny replies flush immediately.
    int one = 1;
    setsockopt(cfd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof one);

    char buf[RX_BUF_SZ];
    for (;;) {
        // 4. read() — blocks the thread until bytes arrive or peer closes.
        ssize_t n = read(cfd, buf, sizeof buf);
        if (n == 0) {                 // peer performed an orderly shutdown
            fprintf(stderr, "client closed\\n");
            return;
        }
        if (n < 0) {
            if (errno == EINTR) continue;    // signal interrupted — retry
            fprintf(stderr, "read: %s\\n", strerror(errno));
            return;
        }

        // Echo back — write() can be short, so loop until we drain.
        ssize_t off = 0;
        while (off < n) {
            ssize_t w = write(cfd, buf + off, (size_t)(n - off));
            if (w < 0) {
                if (errno == EINTR) continue;
                fprintf(stderr, "write: %s\\n", strerror(errno));
                return;
            }
            off += w;
        }
    }
}

int main(void) {
    // Never take SIGPIPE on a dead socket — handle EPIPE from write() instead.
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT,  on_sigint);

    int lfd = make_listen_socket(PORT);
    fprintf(stderr, "HTN listening on 0.0.0.0:%d (Ctrl-C to stop)\\n", PORT);

    while (!g_stop) {
        struct sockaddr_in peer;
        socklen_t plen = sizeof peer;

        // 5. accept() — blocks until a client completes the 3-way handshake.
        int cfd = accept(lfd, (struct sockaddr *)&peer, &plen);
        if (cfd < 0) {
            if (errno == EINTR) continue;
            if (g_stop)         break;
            fprintf(stderr, "accept: %s\\n", strerror(errno));
            continue;
        }

        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &peer.sin_addr, ip, sizeof ip);
        fprintf(stderr, "accepted %s:%u\\n", ip, ntohs(peer.sin_port));

        serve_one_client(cfd);       // <-- ENTIRE server is stuck here until it returns
        close(cfd);
    }

    close(lfd);
    fprintf(stderr, "HTN shutdown clean\\n");
    return 0;
}
