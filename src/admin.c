#include "htn/admin.h"
#include "htn/hdr.h"
#include "htn/log.h"
#include "htn/net.h"

#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* Forward decls from worker.c */
void workers_snapshot(worker_t *ws, int n, worker_metrics *out);

static int render_metrics(admin_t *a, char *buf, size_t cap) {
    worker_metrics m;
    workers_snapshot(a->workers, a->nworkers, &m);
    double p50  = (double)hdr_percentile(&m.latency, 50.0)  / 1e9;
    double p99  = (double)hdr_percentile(&m.latency, 99.0)  / 1e9;
    double p999 = (double)hdr_percentile(&m.latency, 99.9)  / 1e9;
    double sum  = (double)m.latency.sum / 1e9;
    return snprintf(buf, cap,
        "# HELP htn_requests_total Total requests processed.\n"
        "# TYPE htn_requests_total counter\n"
        "htn_requests_total %llu\n"
        "# HELP htn_bytes_in_total Bytes received.\n"
        "# TYPE htn_bytes_in_total counter\n"
        "htn_bytes_in_total %llu\n"
        "# HELP htn_bytes_out_total Bytes sent.\n"
        "# TYPE htn_bytes_out_total counter\n"
        "htn_bytes_out_total %llu\n"
        "# HELP htn_connections_total Connections accepted / closed.\n"
        "# TYPE htn_connections_total counter\n"
        "htn_connections_accepted_total %llu\n"
        "htn_connections_closed_total %llu\n"
        "# HELP htn_request_latency_seconds Request handling latency.\n"
        "# TYPE htn_request_latency_seconds summary\n"
        "htn_request_latency_seconds{quantile=\"0.5\"} %.9f\n"
        "htn_request_latency_seconds{quantile=\"0.99\"} %.9f\n"
        "htn_request_latency_seconds{quantile=\"0.999\"} %.9f\n"
        "htn_request_latency_seconds_sum %.9f\n"
        "htn_request_latency_seconds_count %llu\n",
        (unsigned long long)m.requests,
        (unsigned long long)m.bytes_in,
        (unsigned long long)m.bytes_out,
        (unsigned long long)m.accepts,
        (unsigned long long)m.closes,
        p50, p99, p999, sum,
        (unsigned long long)m.latency.total);
}

static void write_all(int fd, const char *p, size_t n) {
    while (n) { ssize_t w = write(fd, p, n); if (w <= 0) return; p += w; n -= (size_t)w; }
}

static void respond(int fd, int code, const char *status,
                    const char *ctype, const char *body, size_t blen) {
    char hdr[256];
    int hn = snprintf(hdr, sizeof hdr,
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n", code, status, ctype, blen);
    write_all(fd, hdr, (size_t)hn);
    if (body && blen) write_all(fd, body, blen);
}

static void handle_conn(admin_t *a, int cfd) {
    char req[2048];
    ssize_t n = read(cfd, req, sizeof req - 1);
    if (n <= 0) { close(cfd); return; }
    req[n] = '\0';

    /* Parse "METHOD PATH VERSION" from the first line. */
    char method[8] = {0}, path[128] = {0};
    sscanf(req, "%7s %127s", method, path);

    if (strcmp(path, "/healthz") == 0) {
        respond(cfd, 200, "OK", "text/plain", "ok\n", 3);
    } else if (strcmp(path, "/ready") == 0) {
        if (atomic_load(&a->ready))
            respond(cfd, 200, "OK", "text/plain", "ready\n", 6);
        else
            respond(cfd, 503, "Service Unavailable", "text/plain", "not ready\n", 10);
    } else if (strcmp(path, "/metrics") == 0) {
        char body[4096];
        int bn = render_metrics(a, body, sizeof body);
        respond(cfd, 200, "OK", "text/plain; version=0.0.4", body, (size_t)bn);
    } else {
        respond(cfd, 404, "Not Found", "text/plain", "not found\n", 10);
    }
    close(cfd);
}

static void *admin_main(void *arg) {
    admin_t *a = arg;
    log_info("admin http on :%u (/metrics /healthz /ready)", a->port);
    while (!atomic_load(&a->stop)) {
        int cfd = accept(a->lfd, NULL, NULL);
        if (cfd < 0) continue;
        handle_conn(a, cfd);
    }
    return NULL;
}

int admin_start(admin_t *a, uint16_t port, worker_t *ws, int n) {
    memset(a, 0, sizeof *a);
    a->port = port; a->workers = ws; a->nworkers = n;
    atomic_store(&a->ready, 0);
    atomic_store(&a->stop, 0);
    a->lfd = net_make_listen_socket(port);   /* blocking listener is fine */
    if (a->lfd < 0) return -1;
    return pthread_create(&a->thread, NULL, admin_main, a);
}

void admin_set_ready(admin_t *a, int ready) { atomic_store(&a->ready, ready); }

void admin_stop(admin_t *a) {
    atomic_store(&a->stop, 1);
    shutdown(a->lfd, SHUT_RDWR);   /* unblock accept */
    close(a->lfd);
    pthread_join(a->thread, NULL);
}
