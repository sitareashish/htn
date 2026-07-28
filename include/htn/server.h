#pragma once

#include <signal.h>

/* Level-triggered epoll event loop.
 * Serves many clients concurrently on lfd until *stop_flag becomes non-zero.
 * Returns 0 on clean shutdown, -1 on fatal setup error. */
int server_run(int lfd, volatile sig_atomic_t *stop_flag);
