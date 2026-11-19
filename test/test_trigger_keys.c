/*
 * SPDX-FileCopyrightText: 2026 Weng Xuetian <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 *
 */

#include "imclient.h"
#include "imdkit.h"
#include "test.h"
#include "test_xim_server.h"
#include "ximproto.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xproto.h>

static bool opened;
static bool disconnected;
static int trigger_notify_count;
static int set_event_mask_count;
static xcb_window_t client_win;
static xcb_window_t focus_win1;
static xcb_window_t focus_win2;
static xcb_xic_t created_ic;

static void server_callback(xcb_im_t *im, xcb_im_client_t *client,
                            xcb_im_input_context_t *ic,
                            const xcb_im_packet_header_fr_t *hdr, void *frame,
                            void *arg, void *user_data) {
    if (hdr->major_opcode == XCB_XIM_TRIGGER_NOTIFY) {
        xcb_im_trigger_notify_fr_t *tn = frame;
        if (trigger_notify_count == 0) {
            TEST_CHECK(tn->flag == 0);
            TEST_CHECK(
                tn->client_select_event_mask ==
                (XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_FOCUS_CHANGE));
        } else if (trigger_notify_count == 1) {
            TEST_CHECK(tn->flag == 1);
            TEST_CHECK(tn->client_select_event_mask ==
                       (XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE));
        }
        trigger_notify_count++;
    }
}

static void set_ic_callback(xcb_xim_t *im, xcb_xic_t ic, void *user_data) {
    TEST_CHECK(xcb_xim_trigger_notify(im, ic, 0, true));
}

static void set_event_mask_callback(xcb_xim_t *im, xcb_xic_t ic,
                                    uint32_t forward_event_mask,
                                    uint32_t synchronous_event_mask,
                                    void *user_data) {
    set_event_mask_count++;
    if (set_event_mask_count == 1) {
        TEST_CHECK(xcb_xim_set_ic_values(im, created_ic, set_ic_callback, NULL,
                                         XCB_XIM_XNFocusWindow, &focus_win2,
                                         NULL));
    } else if (set_event_mask_count == 2) {
        opened = true;
    }
}

static void create_ic_callback(xcb_xim_t *im, xcb_xic_t ic, void *user_data) {
    TEST_CHECK(ic);
    created_ic = ic;
    TEST_CHECK(xcb_xim_trigger_notify(im, ic, 0, false));
}

static void open_callback(xcb_xim_t *im, void *user_data) {
    uint32_t idx = UINT32_MAX;
    TEST_CHECK(
        xcb_xim_check_trigger_on_key(im, ' ', XCB_MOD_MASK_CONTROL, &idx));
    TEST_CHECK(idx == 0);
    TEST_CHECK(
        xcb_xim_check_trigger_on_key(im, ' ', XCB_MOD_MASK_CONTROL, NULL));
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    TEST_CHECK(xcb_xim_check_trigger_key(im, ' ', XCB_MOD_MASK_CONTROL, NULL) ==
               XCB_XIM_TRIGGER_ON_KEY);
#pragma GCC diagnostic pop

    idx = UINT32_MAX;
    TEST_CHECK(xcb_xim_check_trigger_off_key(im, 0xff1b, 0, &idx));
    TEST_CHECK(idx == 0);
    TEST_CHECK(xcb_xim_check_trigger_off_key(im, 0xff1b, 0, NULL));
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    TEST_CHECK(xcb_xim_check_trigger_key(im, 0xff1b, 0, NULL) ==
               XCB_XIM_TRIGGER_OFF_KEY);
#pragma GCC diagnostic pop

    uint32_t input_style = XCB_IM_PreeditNothing | XCB_IM_StatusNothing;
    TEST_CHECK(xcb_xim_create_ic(im, create_ic_callback, NULL,
                                 XCB_XIM_XNInputStyle, &input_style,
                                 XCB_XIM_XNClientWindow, &client_win,
                                 XCB_XIM_XNFocusWindow, &focus_win1, NULL));
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
        "test_trigger_keys_server", &on_keys, &off_keys, server_callback, NULL);

    int screen_number;
    xcb_connection_t *client_connection = xcb_connect(NULL, &screen_number);
    TEST_CHECK(!xcb_connection_has_error(client_connection));
    xcb_screen_t *screen = xcb_aux_get_screen(client_connection, screen_number);
    TEST_CHECK(screen);

    client_win = xcb_generate_id(client_connection);
    xcb_create_window(
        client_connection, XCB_COPY_FROM_PARENT, client_win, screen->root, 0, 0,
        1, 1, 1, XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, 0, NULL);

    uint32_t mask1 = XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_FOCUS_CHANGE;
    focus_win1 = xcb_generate_id(client_connection);
    xcb_create_window(client_connection, XCB_COPY_FROM_PARENT, focus_win1,
                      screen->root, 0, 0, 1, 1, 1,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual,
                      XCB_CW_EVENT_MASK, &mask1);

    uint32_t mask2 = XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE;
    focus_win2 = xcb_generate_id(client_connection);
    xcb_create_window(client_connection, XCB_COPY_FROM_PARENT, focus_win2,
                      screen->root, 0, 0, 1, 1, 1,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual,
                      XCB_CW_EVENT_MASK, &mask2);
    xcb_flush(client_connection);

    xcb_xim_im_callback callbacks = {
        .set_event_mask = set_event_mask_callback,
        .disconnected = disconnected_callback,
    };
    xcb_xim_t *client_im = xcb_xim_create(client_connection, screen_number,
                                          "@im=test_trigger_keys_server");
    xcb_xim_set_im_callback(client_im, &callbacks, NULL);
    TEST_CHECK(xcb_xim_open(client_im, open_callback, true, NULL));
    TEST_CHECK(!xcb_xim_filter_event(client_im, NULL));

    xcb_generic_event_t *event;
    while (!opened && (event = xcb_wait_for_event(client_connection))) {
        xcb_xim_filter_event(client_im, event);
        free(event);
    }
    TEST_CHECK(opened);
    TEST_CHECK(trigger_notify_count == 2);

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
