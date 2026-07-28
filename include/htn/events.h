#pragma once

#include <stdint.h>

/* Block SIGINT+SIGTERM in the caller's thread and return a signalfd that
 * becomes readable when either is delivered. Returns fd or -1. */
int ev_make_signalfd(void);

/* Create an eventfd (semaphore-style counter). Returns fd or -1. */
int ev_make_eventfd(void);

/* Post to an eventfd to wake a blocked epoll_wait. */
int ev_eventfd_post(int efd);

/* Drain an eventfd's counter (call after it fires). */
int ev_eventfd_drain(int efd);

/* Create a periodic timerfd firing every interval_ms. Returns fd or -1. */
int ev_make_timerfd(unsigned interval_ms);

/* Drain a timerfd's expiration count (call after it fires). */
int ev_timerfd_drain(int tfd);
