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
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xproto.h>

static xcb_connection_t *client_connection;
static xcb_screen_t *client_screen;
static xcb_xim_t *client_im;
static xcb_window_t client_window;
static bool completed;

static void server_callback(xcb_im_t *im, xcb_im_client_t *client,
                            xcb_im_input_context_t *ic,
                            const xcb_im_packet_header_fr_t *hdr, void *frame,
                            void *arg, void *user_data) {
    if (hdr->major_opcode == XCB_XIM_FORWARD_EVENT) {
        xcb_im_forward_event(im, ic, arg);
    }
}

static void forward_event(xcb_xim_t *im, xcb_xic_t ic,
                          xcb_key_press_event_t *event, void *user_data) {
    TEST_CHECK(event->response_type == XCB_KEY_PRESS);
    TEST_CHECK(event->detail == 38);
    completed = true;
}

static void create_ic_callback(xcb_xim_t *im, xcb_xic_t ic, void *user_data) {
    TEST_CHECK(ic);

    xcb_key_press_event_t event = {
        .response_type = XCB_KEY_PRESS,
        .root = client_screen->root,
        .event = client_window,
        .detail = 38,
    };
    TEST_CHECK(xcb_xim_forward_event(client_im, ic, &event));
}

static void open_callback(xcb_xim_t *im, void *user_data) {
    uint32_t input_style = XCB_IM_PreeditNothing | XCB_IM_StatusNothing;
    TEST_CHECK(xcb_xim_create_ic(client_im, create_ic_callback, NULL,
                                 XCB_XIM_XNInputStyle, &input_style,
                                 XCB_XIM_XNClientWindow, &client_window,
                                 XCB_XIM_XNFocusWindow, &client_window, NULL));
}

static void run_client(void) {
    int screen_number;
    client_connection = xcb_connect(NULL, &screen_number);
    client_screen = xcb_aux_get_screen(client_connection, screen_number);
    TEST_CHECK(client_screen);

    client_window = xcb_generate_id(client_connection);
    xcb_create_window(client_connection, XCB_COPY_FROM_PARENT, client_window,
                      client_screen->root, 0, 0, 1, 1, 1,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, client_screen->root_visual,
                      0, NULL);

    xcb_xim_im_callback callbacks = {
        .forward_event = forward_event,
    };
    client_im = xcb_xim_create(client_connection, screen_number,
                               "@im=test_basic_server");
    xcb_xim_set_im_callback(client_im, &callbacks, NULL);
    TEST_CHECK(xcb_xim_open(client_im, open_callback, true, NULL));

    xcb_generic_event_t *event;
    while (!completed && (event = xcb_wait_for_event(client_connection))) {
        xcb_xim_filter_event(client_im, event);
        free(event);
    }

    TEST_CHECK(completed);
    xcb_xim_close(client_im);
    xcb_xim_destroy(client_im);
    xcb_disconnect(client_connection);
}

int main() {
    test_xim_server_t *server =
        test_xim_server_new("test_basic_server", server_callback, NULL);

    run_client();
    test_xim_server_destroy(server);
    return 0;
}
