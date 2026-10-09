/*
 * SPDX-FileCopyrightText: 2026 Weng Xuetian <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 *
 */

#ifndef XCB_IMDKIT_TEST_H
#define XCB_IMDKIT_TEST_H

#include <stdio.h> // IWYU pragma: keep

#include <stdlib.h> // IWYU pragma: keep

#define TEST_CHECK(condition)                                                  \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition);    \
            abort();                                                           \
        }                                                                      \
    } while (0)

#endif
