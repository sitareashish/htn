#pragma once

#include <stdint.h>

/* Create an IPv4 TCP listening socket on 0.0.0.0:port.
 * Sets SO_REUSEADDR. listen() backlog = 128.
 * Returns fd on success, -1 on failure (errno preserved). */
int net_make_listen_socket(uint16_t port);

/* Disable Nagle on an accepted connection fd. Best-effort. */
void net_set_nodelay(int fd);

/* Set O_NONBLOCK on fd. Returns 0 on success, -1 on failure (errno preserved). */
int net_set_nonblocking(int fd);
