#include "htn/events.h"
#include "htn/log.h"
#include "htn/worker.h"

#include <signal.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <unistd.h>

#define PORT       9100
#define POOL_CAP   8192

int main(int argc, char **argv) {
    signal(SIGPIPE, SIG_IGN);

    int nworkers = (argc > 1) ? atoi(argv[1]) : (int)sysconf(_SC_NPROCESSORS_ONLN);
    if (nworkers < 1) nworkers = 1;

    /* Block signals in the MAIN thread BEFORE spawning workers so the mask
       is inherited; only the main thread handles the signalfd. */
    int sfd = ev_make_signalfd();
    if (sfd < 0) { log_error("signalfd failed"); return 1; }

    worker_t *ws = calloc((size_t)nworkers, sizeof *ws);
    for (int i = 0; i < nworkers; ++i) {
        if (worker_init(&ws[i], i, PORT, POOL_CAP) < 0) {
            log_error("worker_init %d failed", i);
            return 1;
        }
    }
    for (int i = 0; i < nworkers; ++i)
        pthread_create(&ws[i].thread, NULL, worker_main, &ws[i]);

    log_info("HTN listening on :%d with %d workers", PORT, nworkers);

    /* Main thread blocks until a signal arrives on the signalfd. */
    struct epoll_event ev;
    int epfd = epoll_create1(EPOLL_CLOEXEC);
    struct epoll_event se = { .events = EPOLLIN, .data = { .fd = sfd } };
    epoll_ctl(epfd, EPOLL_CTL_ADD, sfd, &se);
    epoll_wait(epfd, &ev, 1, -1);          /* wakes on SIGINT/SIGTERM */
    log_info("signal received, stopping workers");

    for (int i = 0; i < nworkers; ++i) worker_stop(&ws[i]);
    for (int i = 0; i < nworkers; ++i) pthread_join(ws[i].thread, NULL);
    for (int i = 0; i < nworkers; ++i) worker_destroy(&ws[i]);

    close(epfd); close(sfd); free(ws);
    log_info("HTN shutdown clean");
    return 0;
}
