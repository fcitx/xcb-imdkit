/*
 * SPDX-FileCopyrightText: 2014 Weng Xuetian <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 *
 */
#include "encoding.h"
#include "xcbimdkit_export.h"
#include "xlibi18n/XlcPubI.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#define _CONVERT_BUFSIZE 2048

typedef int (*convert_to_cs_func)(State, void **, size_t *, void **, size_t *,
                                  XlcCharSet *);
typedef int (*convert_from_cs_func)(State, void **, size_t *, void **, size_t *,
                                    XlcCharSet);

static bool get_buf_size(size_t length, size_t *out) {
    if (length > (SIZE_MAX - _CONVERT_BUFSIZE - 1) / 3) {
        return false;
    }
    length *= 3;                                                 /* XXX */
    length = (length / _CONVERT_BUFSIZE + 1) * _CONVERT_BUFSIZE; /* XXX */

    *out = length;
    return true;
}

XCBIMDKIT_EXPORT
void xcb_compound_text_init() { _XlcInitCTInfo(); }

int indirect_convert(void **from, size_t *from_left, void **to, size_t *to_left,
                     convert_to_cs_func tocs, convert_from_cs_func csto) {
    char buf[_CONVERT_BUFSIZE];
    void *cs;
    size_t cs_left;
    XlcCharSet charset;

    int ret;
    int unconv_num = 0;
    StateRec state;
    init_state(&state);
    while (*from_left > 0) {
        cs = buf;
        cs_left = sizeof(buf);

        ret = tocs(&state, from, from_left, &cs, &cs_left, &charset);
        if (ret < 0) {
            break;
        }

        unconv_num += ret;

        size_t length = ((char *)cs) - buf;
        if (length > 0) {
            cs_left = length;
            cs = buf;

            ret = csto(&state, &cs, &cs_left, to, to_left, charset);
            if (ret < 0) {
                unconv_num +=
                    length / (charset->char_size > 0 ? charset->char_size : 1);
                continue;
            }

            unconv_num += ret;

            if (*to_left < 1) {
                break;
            }
        }
    }
    return unconv_num;
}

XCBIMDKIT_EXPORT
char *xcb_utf8_to_compound_text(const char *utf8, size_t len,
                                size_t *lenghtOut) {
    if (!utf8 && len > 0) {
        return NULL;
    }
    size_t buf_len;
    if (!get_buf_size(len, &buf_len)) {
        return NULL;
    }
    void *result;
    if ((result = malloc(buf_len + 1)) == NULL) {
        return NULL;
    }

    void *from = (void *)utf8;
    void *to = result;
    size_t from_left = len;
    size_t to_left = buf_len;
    int ret =
        indirect_convert(&from, &from_left, &to, &to_left, utf8tocs, cstoct);

    if (ret || from_left != 0) {
        free(result);
        return NULL;
    }

    if (lenghtOut) {
        *lenghtOut = (char *)to - (char *)result;
    }

    *((char *)to) = '\0';

    return result;
}

XCBIMDKIT_EXPORT
char *xcb_compound_text_to_utf8(const char *compound_text, size_t len,
                                size_t *lenghtOut) {
    if (!compound_text && len > 0) {
        return NULL;
    }
    size_t buf_len;
    if (!get_buf_size(len, &buf_len)) {
        return NULL;
    }
    void *result;
    if ((result = malloc(buf_len + 1)) == NULL) {
        return NULL;
    }

    void *from = (void *)compound_text;
    void *to = result;
    size_t from_left = len;
    size_t to_left = buf_len;
    int ret =
        indirect_convert(&from, &from_left, &to, &to_left, cttocs, cstoutf8);

    if (ret || from_left != 0) {
        free(result);
        return NULL;
    }

    if (lenghtOut) {
        *lenghtOut = (char *)to - (char *)result;
    }

    *((char *)to) = '\0';

    return result;
}
