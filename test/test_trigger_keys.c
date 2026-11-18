/*
 * SPDX-FileCopyrightText: 2026 Weng Xuetian <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 *
 */

#include "imclient.h"
#include "test.h"
#include "test_xim_server.h"
#include "ximproto.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>

static bool opened;
static bool disconnected;

static void open_callback(xcb_xim_t *im, void *user_data) {
    uint32_t idx = UINT32_MAX;
    TEST_CHECK(
        xcb_xim_check_trigger_on_key(im, ' ', XCB_MOD_MASK_CONTROL, &idx));
    TEST_CHECK(idx == 0);

    idx = UINT32_MAX;
    TEST_CHECK(xcb_xim_check_trigger_off_key(im, 0xff1b, 0, &idx));
    TEST_CHECK(idx == 0);

    opened = true;
}

static void disconnected_callback(xcb_xim_t *im, void *user_data) {
    disconnected = true;
}

int main() {
    xcb_im_ximtriggerkey_fr_t on_key = {
        .keysym = ' ',
        .modifier = XCB_MOD_MASK_CONTROL,
        .modifier_mask = XCB_MOD_MASK_CONTROL,
    };
    xcb_im_ximtriggerkey_fr_t off_key = {
        .keysym = 0xff1b,
        .modifier = 0,
        .modifier_mask = 0,
    };
    xcb_im_trigger_keys_t on_keys = {
        .nKeys = 1,
        .keys = &on_key,
    };
    xcb_im_trigger_keys_t off_keys = {
        .nKeys = 1,
        .keys = &off_key,
    };

    test_xim_server_t *server = test_xim_server_new_with_keys(
        "test_trigger_keys_server", &on_keys, &off_keys, NULL, NULL);

    int screen_number;
    xcb_connection_t *client_connection = xcb_connect(NULL, &screen_number);
    TEST_CHECK(!xcb_connection_has_error(client_connection));

    xcb_xim_im_callback callbacks = {
        .disconnected = disconnected_callback,
    };
    xcb_xim_t *client_im = xcb_xim_create(client_connection, screen_number,
                                          "@im=test_trigger_keys_server");
    xcb_xim_set_im_callback(client_im, &callbacks, NULL);
    TEST_CHECK(xcb_xim_open(client_im, open_callback, true, NULL));

    xcb_generic_event_t *event;
    while (!opened && (event = xcb_wait_for_event(client_connection))) {
        xcb_xim_filter_event(client_im, event);
        free(event);
    }
    TEST_CHECK(opened);

    test_xim_server_destroy(server);

    while (!disconnected && (event = xcb_wait_for_event(client_connection))) {
        xcb_xim_filter_event(client_im, event);
        free(event);
    }
    TEST_CHECK(disconnected);

    xcb_xim_close(client_im);
    xcb_xim_destroy(client_im);
    xcb_disconnect(client_connection);
    return 0;
}

