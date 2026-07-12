#include "htn/log.h"
#include "htn/net.h"
#include "htn/server.h"

#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#define PORT 9100

static volatile sig_atomic_t g_stop = 0;
static void on_sigint(int _s) { (void)_s; g_stop = 1; }

int main(void) {
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT,  on_sigint);

    int lfd = net_make_listen_socket(PORT);
    if (lfd < 0) die("net_make_listen_socket");

    log_info("HTN listening on 0.0.0.0:%d (Ctrl-C to stop)", PORT);
    int rc = server_run(lfd, &g_stop);
    close(lfd);
    log_info("HTN shutdown clean (rc=%d)", rc);
    return rc;
}
