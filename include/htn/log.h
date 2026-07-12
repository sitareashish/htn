#pragma once

#include <stdnoreturn.h>

void log_info(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void log_error(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
noreturn void die(const char *what);
