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

    return 0;
}
