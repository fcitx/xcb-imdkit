/*
 * SPDX-FileCopyrightText: 2014 Weng Xuetian <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 *
 */

#include "parser.h"
#include "ximproto.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static inline uint16_t _xcb_im_cond_bswap16(uint16_t n, bool swap) {
    return swap ? _xcb_im_bswap16(n) : n;
}

static inline uint32_t _xcb_im_cond_bswap32(uint32_t n, bool swap) {
    return swap ? _xcb_im_bswap32(n) : n;
}

void uint8_t_read(uint8_t *p, uint8_t **data, size_t *len, bool swap) {
    if (*len < 1) {
        *data = NULL;
        return;
    }

    *p = **data;

    *data += 1;
    *len -= 1;
}

uint8_t *uint8_t_write(uint8_t *p, uint8_t *data, bool swap) {
    *data = *p;
    data += 1;
    return data;
}

void uint16_t_read(uint16_t *p, uint8_t **data, size_t *len, bool swap) {
    if (*len < 2) {
        *data = NULL;
        return;
    }

    uint16_t val;
    memcpy(&val, *data, sizeof(val));
    *p = _xcb_im_cond_bswap16(val, swap);
    *data += 2;
    *len -= 2;
}

uint8_t *uint16_t_write(uint16_t *p, uint8_t *data, bool swap) {
    uint16_t val = _xcb_im_cond_bswap16(*p, swap);
    memcpy(data, &val, sizeof(val));
    data += 2;
    return data;
}

void uint32_t_read(uint32_t *p, uint8_t **data, size_t *len, bool swap) {
    if (*len < 4) {
        *data = NULL;
        return;
    }

    uint32_t val;
    memcpy(&val, *data, sizeof(val));
    *p = _xcb_im_cond_bswap32(val, swap);
    *data += 4;
    *len -= 4;
}

uint8_t *uint32_t_write(uint32_t *p, uint8_t *data, bool swap) {
    uint32_t val = _xcb_im_cond_bswap32(*p, swap);
    memcpy(data, &val, sizeof(val));
    data += 4;
    return data;
}

void xcb_im_bytearray_t_read(xcb_im_bytearray_t *p, uint32_t arraylen,
                             uint8_t **data, size_t *len, bool swap) {
    if (*len < arraylen) {
        *data = NULL;
        return;
    }

    *p = *data;
    *data += arraylen;
    *len -= arraylen;
}

uint8_t *xcb_im_bytearray_t_write(xcb_im_bytearray_t *p, uint32_t arraylen,
                                  uint8_t *data, bool swap) {
    if (arraylen > 0) {
        memcpy(data, *p, arraylen);
    }
    return data + arraylen;
}

uintptr_t align_to_4(uintptr_t ptr, uintptr_t len, size_t *remain) {
    uintptr_t diff = (len % 4) ? 4 - (len % 4) : 0;
    if (remain) {
        if (*remain < diff) {
            return 0;
        }
        *remain -= diff;
    }
    return ptr + diff;
}

uintptr_t align_to_2(uintptr_t ptr, uintptr_t len, size_t *remain) {
    uintptr_t diff = (len % 2) ? 2 - (len % 2) : 0;
    if (remain) {
        if (*remain < diff) {
            return 0;
        }
        *remain -= diff;
    }
    return ptr + diff;
}
