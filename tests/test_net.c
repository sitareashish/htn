#include "munit.h"
#include "htn/net.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

/* Ask the kernel which port a listening fd was bound to. */
static uint16_t port_of(int fd) {
    struct sockaddr_in addr;
    socklen_t len = sizeof addr;
    int rc = getsockname(fd, (struct sockaddr *)&addr, &len);
    munit_assert_int(rc, ==, 0);
    return ntohs(addr.sin_port);
}

/* Port 0 = kernel picks a free ephemeral port. */
static MunitResult test_listen_ephemeral(const MunitParameter params[], void *fixture) {
    (void)params; (void)fixture;
    int fd = net_make_listen_socket(0);
    munit_assert_int(fd, >=, 0);
    munit_assert_uint16(port_of(fd), >, 0);
    close(fd);
    return MUNIT_OK;
}

/* Second bind on same port must fail. */
static MunitResult test_listen_conflict(const MunitParameter params[], void *fixture) {
    (void)params; (void)fixture;
    int a = net_make_listen_socket(0);
    munit_assert_int(a, >=, 0);
    uint16_t p = port_of(a);
    int b = net_make_listen_socket(p);
    munit_assert_int(b, ==, -1);
    close(a);
    return MUNIT_OK;
}

static MunitTest tests[] = {
    { "/listen_ephemeral", test_listen_ephemeral, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { "/listen_conflict",  test_listen_conflict,  NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
};

static const MunitSuite suite = {
    "/net", tests, NULL, 1, MUNIT_SUITE_OPTION_NONE
};

int main(int argc, char *argv[]) {
    return munit_suite_main(&suite, NULL, argc, argv);
}
