#pragma once

/* Run the epoll event loop until SIGINT/SIGTERM is received.
 * Returns 0 on clean shutdown, -1 on fatal setup error. */
int server_run(int lfd);
