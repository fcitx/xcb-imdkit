/*
 * SPDX-FileCopyrightText: 2014 Weng Xuetian <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 *
 */
#include "encoding.h"
#include "test.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define TEST_STRING "hello world!你好世界켐ㅇㄹ貴方元気？☺"

void test_conversion(const char *str) {

    size_t len;
    char *result = xcb_utf8_to_compound_text(str, strlen(str), &len);
    TEST_CHECK(result != NULL);
    TEST_CHECK(result[len] == '\0');

    size_t utf8_len;
    char *utf8_result = xcb_compound_text_to_utf8(result, len, &utf8_len);
    TEST_CHECK(utf8_result != NULL);
    TEST_CHECK(utf8_len == strlen(str));
    TEST_CHECK(utf8_result[utf8_len] == '\0');

    TEST_CHECK(strcmp(utf8_result, str) == 0);

    free(result);
    free(utf8_result);
}

int main() {
    xcb_compound_text_init();
    test_conversion("");
    test_conversion(TEST_STRING);
    test_conversion("\xe2\x80\x93");

    // Overflowing length must fail cleanly without wrapping get_buf_size() to
    // 0.
    TEST_CHECK(xcb_utf8_to_compound_text("\x80", SIZE_MAX, NULL) == NULL);
    TEST_CHECK(xcb_utf8_to_compound_text("a", SIZE_MAX / 3 + 1, NULL) == NULL);
    TEST_CHECK(xcb_compound_text_to_utf8("\x1b", SIZE_MAX, NULL) == NULL);
    TEST_CHECK(xcb_compound_text_to_utf8("a", SIZE_MAX / 3 + 1, NULL) == NULL);

    // Valid Compound Text extended segment (\x1b%/2 + [0x80, 0x88] +
    // "gbk-0\x02"
    // + GBK "\xc4\xe3") decodes to UTF-8 "你" ("\xe4\xbd\xa0").
    const char valid_ext[] = "\x1b%/2\x80\x88gbk-0\x02\xc4\xe3";
    size_t ext_len = 0;
    char *ext_utf8 =
        xcb_compound_text_to_utf8(valid_ext, sizeof(valid_ext) - 1, &ext_len);
    TEST_CHECK(ext_utf8 != NULL);
    TEST_CHECK(strcmp(ext_utf8, "\xe4\xbd\xa0") == 0);
    free(ext_utf8);

    // Malformed extended segment whose second length byte lacks bit 7 (0x08
    // instead of 0x88) must NOT be accepted as a valid XctExtSeg header.
    const char bad_ext[] = "\x1b%/2\x80\x08gbk-0\x02\xc4\xe3";
    char *bad_utf8 =
        xcb_compound_text_to_utf8(bad_ext, sizeof(bad_ext) - 1, &ext_len);
    TEST_CHECK(bad_utf8 != NULL);
    TEST_CHECK(strcmp(bad_utf8, "\xe4\xbd\xa0") != 0);
    free(bad_utf8);

    return 0;
}
