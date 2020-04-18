/*
 * (C) Copyright 2014 Weng Xuetian <wengxt@gmail.com>
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the GNU Lesser General Public License
 * (LGPL) version 2.1 which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/lgpl-2.1.html
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 */
#include "encoding.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define TEST_STRING "hello world!你好世界켐ㅇㄹ貴方元気？☺"

void test_conversion(const char *str) {

    size_t len;
    char *result = xcb_utf8_to_compound_text(str, strlen(str), &len);

    char *utf8_result = xcb_compound_text_to_utf8(result, len, NULL);

    assert(strcmp(utf8_result, str) == 0);

    free(result);
    free(utf8_result);
}

int main() {
    xcb_compound_text_init();
    test_conversion(TEST_STRING);
    test_conversion("\xe2\x80\x93");

    return 0;
}
