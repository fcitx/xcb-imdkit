/*
 * SPDX-FileCopyrightText: 2026 Weng Xuetian <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 *
 */

#include "parser.h"
#include "test.h"
#include <stdalign.h>
#include <stdint.h>
#include <string.h>

int main() {
    alignas(8) uint8_t buf[32];
    memset(buf, 0, sizeof(buf));

    // 1. Unaligned (odd-offset) uint16_t read/write with and without byte swap.
    uint16_t v16 = 0x1234u;
    uint8_t *end = uint16_t_write(&v16, buf + 1, false);
    TEST_CHECK(end == buf + 3);
    uint8_t *p = buf + 1;
    size_t len = 2;
    uint16_t out16 = 0;
    uint16_t_read(&out16, &p, &len, false);
    TEST_CHECK(p == buf + 3 && len == 0 && out16 == 0x1234u);

    p = buf + 1;
    len = 2;
    uint16_t_read(&out16, &p, &len, true);
    TEST_CHECK(p == buf + 3 && len == 0 && out16 == 0x3412u);

    end = uint16_t_write(&v16, buf + 1, true);
    TEST_CHECK(end == buf + 3);
    p = buf + 1;
    len = 2;
    uint16_t_read(&out16, &p, &len, false);
    TEST_CHECK(out16 == 0x3412u);

    // 2. Unaligned (odd-offset and 2-byte-offset) uint32_t read/write.
    for (size_t off = 1; off <= 2; off++) {
        uint32_t v32 = 0x12345678u;
        end = uint32_t_write(&v32, buf + off, false);
        TEST_CHECK(end == buf + off + 4);
        p = buf + off;
        len = 4;
        uint32_t out32 = 0;
        uint32_t_read(&out32, &p, &len, false);
        TEST_CHECK(p == buf + off + 4 && len == 0 && out32 == 0x12345678u);

        p = buf + off;
        len = 4;
        uint32_t_read(&out32, &p, &len, true);
        TEST_CHECK(p == buf + off + 4 && len == 0 && out32 == 0x78563412u);

        end = uint32_t_write(&v32, buf + off, true);
        TEST_CHECK(end == buf + off + 4);
        p = buf + off;
        len = 4;
        uint32_t_read(&out32, &p, &len, false);
        TEST_CHECK(out32 == 0x78563412u);
    }

    // 3. Zero-length bytearray write with NULL pointer must not invoke
    // memcpy(data, NULL, 0) UB.
    xcb_im_bytearray_t empty = NULL;
    end = xcb_im_bytearray_t_write(&empty, 0, buf, false);
    TEST_CHECK(end == buf);

    const uint8_t payload[3] = {0xAA, 0xBB, 0xCC};
    xcb_im_bytearray_t non_empty = (xcb_im_bytearray_t)payload;
    end = xcb_im_bytearray_t_write(&non_empty, 3, buf + 1, false);
    TEST_CHECK(end == buf + 4);
    TEST_CHECK(memcmp(buf + 1, payload, 3) == 0);

    return 0;
}
