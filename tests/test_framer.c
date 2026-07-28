#include "munit.h"
#include "htn/framer.h"
#include "htn/proto.h"

#include <string.h>

static size_t build_echo(uint8_t *out, const char *msg) {
    htn_header_t h = { HTN_MAGIC, HTN_VERSION, HTN_ECHO, (uint32_t)strlen(msg) };
    htn_header_encode(&h, out);
    memcpy(out + HTN_HDR_SIZE, msg, strlen(msg));
    return HTN_HDR_SIZE + strlen(msg);
}

/* Feed a whole frame in one shot. */
static MunitResult test_whole_frame(const MunitParameter p[], void *f) {
    (void)p; (void)f;
    uint8_t wire[128];
    size_t total = build_echo(wire, "hello");
    framer_t fr; framer_init(&fr);
    size_t len = total; htn_header_t h; const uint8_t *pay;
    frame_result_t r = framer_push(&fr, wire, &len, &h, &pay);
    munit_assert_int(r, ==, FRAME_READY);
    munit_assert_uint32(h.payload_len, ==, 5);
    munit_assert_memory_equal(5, pay, "hello");
    return MUNIT_OK;
}

/* Feed the SAME frame one byte at a time — the split-read torture test. */
static MunitResult test_byte_by_byte(const MunitParameter p[], void *f) {
    (void)p; (void)f;
    uint8_t wire[128];
    size_t total = build_echo(wire, "dribble");
    framer_t fr; framer_init(&fr);
    int got = 0;
    for (size_t i = 0; i < total; ++i) {
        size_t len = 1; htn_header_t h; const uint8_t *pay;
        frame_result_t r = framer_push(&fr, wire + i, &len, &h, &pay);
        if (r == FRAME_READY) {
            munit_assert_memory_equal(7, pay, "dribble");
            got = 1;
        } else {
            munit_assert_int(r, ==, FRAME_NONE);
        }
    }
    munit_assert_int(got, ==, 1);
    return MUNIT_OK;
}

/* Two frames concatenated in one buffer must both be drained. */
static MunitResult test_two_frames(const MunitParameter p[], void *f) {
    (void)p; (void)f;
    uint8_t wire[256];
    size_t a = build_echo(wire, "one");
    size_t b = build_echo(wire + a, "two");
    size_t total = a + b;
    framer_t fr; framer_init(&fr);
    size_t off = 0; int frames = 0;
    while (off < total) {
        size_t len = total - off; htn_header_t h; const uint8_t *pay;
        frame_result_t r = framer_push(&fr, wire + off, &len, &h, &pay);
        off += len;
        if (r == FRAME_READY) frames++;
        else if (r == FRAME_NONE) break;
    }
    munit_assert_int(frames, ==, 2);
    return MUNIT_OK;
}

/* Bad magic must be rejected. */
static MunitResult test_bad_magic(const MunitParameter p[], void *f) {
    (void)p; (void)f;
    uint8_t wire[8] = {0xDE,0xAD,1,HTN_ECHO,0,0,0,0};
    framer_t fr; framer_init(&fr);
    size_t len = 8; htn_header_t h; const uint8_t *pay;
    frame_result_t r = framer_push(&fr, wire, &len, &h, &pay);
    munit_assert_int(r, ==, FRAME_ERR);
    return MUNIT_OK;
}

/* Oversized payload_len must be rejected. */
static MunitResult test_oversized(const MunitParameter p[], void *f) {
    (void)p; (void)f;
    htn_header_t big = { HTN_MAGIC, HTN_VERSION, HTN_ECHO, HTN_MAX_PAYLOAD + 1 };
    uint8_t wire[8]; htn_header_encode(&big, wire);
    framer_t fr; framer_init(&fr);
    size_t len = 8; htn_header_t h; const uint8_t *pay;
    frame_result_t r = framer_push(&fr, wire, &len, &h, &pay);
    munit_assert_int(r, ==, FRAME_ERR);
    return MUNIT_OK;
}

static MunitTest tests[] = {
    { "/whole_frame",  test_whole_frame,  NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { "/byte_by_byte", test_byte_by_byte, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { "/two_frames",   test_two_frames,   NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { "/bad_magic",    test_bad_magic,    NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { "/oversized",    test_oversized,    NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
};
static const MunitSuite suite = { "/framer", tests, NULL, 1, MUNIT_SUITE_OPTION_NONE };
int main(int argc, char *argv[]) { return munit_suite_main(&suite, NULL, argc, argv); }
