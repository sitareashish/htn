#include "htn/server.h"
#include "htn/log.h"
#include "htn/net.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define RX_BUF_SZ 4096

static void serve_one_client(int cfd) {
    net_set_nodelay(cfd);

    char buf[RX_BUF_SZ];
    for (;;) {
        ssize_t n = read(cfd, buf, sizeof buf);
        if (n == 0) {
            log_info("client closed fd=%d", cfd);
            return;
        }
        if (n < 0) {
            if (errno == EINTR) continue;
            log_error("read: %s", strerror(errno));
            return;
        }
        ssize_t off = 0;
        while (off < n) {
            ssize_t w = write(cfd, buf + off, (size_t)(n - off));
            if (w < 0) {
                if (errno == EINTR) continue;
                log_error("write: %s", strerror(errno));
                return;
            }
            off += w;
        }
    }
}

int server_run(int lfd, volatile sig_atomic_t *stop_flag) {
    while (!*stop_flag) {
        struct sockaddr_in peer;
        socklen_t plen = sizeof peer;
        int cfd = accept(lfd, (struct sockaddr *)&peer, &plen);
        if (cfd < 0) {
            if (errno == EINTR) continue;
            if (*stop_flag)      break;
            log_error("accept: %s", strerror(errno));
            continue;
        }
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &peer.sin_addr, ip, sizeof ip);
        log_info("accepted %s:%u fd=%d", ip, ntohs(peer.sin_port), cfd);
        serve_one_client(cfd);
        close(cfd);
    }
    return 0;
}
