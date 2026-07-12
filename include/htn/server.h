#pragma once

#include <signal.h>

/* Blocking single-connection accept loop.
 * Serves clients one at a time on lfd until *stop_flag becomes non-zero.
 * Returns 0 on clean shutdown. */
int server_run(int lfd, volatile sig_atomic_t *stop_flag);
