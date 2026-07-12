#include "htn/log.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static void vlog(FILE *out, const char *level, const char *fmt, va_list ap) {
    char ts[32];
    time_t now = time(NULL);
    struct tm tm;
    localtime_r(&now, &tm);
    strftime(ts, sizeof ts, "%Y-%m-%d %H:%M:%S", &tm);
    fprintf(out, "%s %s ", ts, level);
    vfprintf(out, fmt, ap);
    fputc('\n', out);
}

void log_info(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vlog(stdout, "INFO ", fmt, ap);
    va_end(ap);
}

void log_error(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vlog(stderr, "ERROR", fmt, ap);
    va_end(ap);
}

noreturn void die(const char *what) {
    log_error("fatal: %s: %s", what, strerror(errno));
    exit(EXIT_FAILURE);
}
