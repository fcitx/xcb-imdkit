/*
 * SPDX-FileCopyrightText: 2026 Weng Xuetian <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 *
 */

#ifndef XCB_IMDKIT_TEST_XIM_SERVER_H
#define XCB_IMDKIT_TEST_XIM_SERVER_H

#include "imdkit.h"
#include <stdbool.h>
#include <threads.h>
#include <xcb/xproto.h>

typedef struct {
    const char *name;
    xcb_im_callback callback;
    void *user_data;
    thrd_t thread;
    cnd_t ready;
    mtx_t mutex;
    bool started;
    xcb_window_t window;
    xcb_atom_t stop_atom;
} test_xim_server_t;

test_xim_server_t *test_xim_server_new(const char *name,
                                       xcb_im_callback callback,
                                       void *user_data);
void test_xim_server_destroy(test_xim_server_t *server);

#endif
