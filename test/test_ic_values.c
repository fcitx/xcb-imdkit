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

static xcb_connection_t *client_connection;
static xcb_screen_t *client_screen;
static xcb_xim_t *client_im;
static xcb_window_t client_window;
static bool completed;

static void get_ic_values_callback(xcb_xim_t *im, xcb_xic_t ic,
                                   xcb_im_get_ic_values_reply_fr_t *reply,
                                   void *user_data) {
    TEST_CHECK(reply);
    // Deduplicated top-level: 1 XNFilterEvents + 1 nested XNPreeditAttributes
    // (containing 2 XNSpotLocation entries = 16 bytes).
    TEST_CHECK(reply->ic_attribute.size == 2);
    TEST_CHECK(reply->ic_attribute.items[0].value_length == 4);
    TEST_CHECK(reply->ic_attribute.items[1].value_length == 16);
    completed = true;
}

static void create_ic_callback(xcb_xim_t *im, xcb_xic_t ic, void *user_data) {
    TEST_CHECK(ic);

    // Request more than ARRAY_SIZE(Default_ICattr) (17) readable IC attributes
    // (including duplicate nested lists and duplicate nested sub-attributes)
    // in a single XIM_GET_IC_VALUES call.
    TEST_CHECK(xcb_xim_get_ic_values(
        client_im, ic, get_ic_values_callback, NULL, XCB_XIM_XNFilterEvents,
        XCB_XIM_XNFilterEvents, XCB_XIM_XNFilterEvents, XCB_XIM_XNFilterEvents,
        XCB_XIM_XNFilterEvents, XCB_XIM_XNFilterEvents, XCB_XIM_XNFilterEvents,
        XCB_XIM_XNFilterEvents, XCB_XIM_XNFilterEvents, XCB_XIM_XNFilterEvents,
        XCB_XIM_XNFilterEvents, XCB_XIM_XNFilterEvents, XCB_XIM_XNFilterEvents,
        XCB_XIM_XNFilterEvents, XCB_XIM_XNFilterEvents, XCB_XIM_XNFilterEvents,
        XCB_XIM_XNFilterEvents, XCB_XIM_XNFilterEvents, XCB_XIM_XNFilterEvents,
        XCB_XIM_XNFilterEvents, XCB_XIM_XNFilterEvents, XCB_XIM_XNFilterEvents,
        XCB_XIM_XNFilterEvents, XCB_XIM_XNFilterEvents,
        XCB_XIM_XNPreeditAttributes, XCB_XIM_XNSpotLocation,
        XCB_XIM_XNSpotLocation, XCB_XIM_XNSeparatorofNestedList,
        XCB_XIM_XNPreeditAttributes, XCB_XIM_XNSpotLocation,
        XCB_XIM_XNSeparatorofNestedList, NULL));
}

static void open_callback(xcb_xim_t *im, void *user_data) {
    uint32_t input_style = XCB_IM_PreeditPosition | XCB_IM_StatusNothing;
    xcb_point_t spot = {.x = 12, .y = 34};
    xcb_xim_nested_list nested = xcb_xim_create_nested_list(
        client_im, XCB_XIM_XNSpotLocation, &spot, NULL);
    TEST_CHECK(nested.data);
    TEST_CHECK(xcb_xim_create_ic(
        client_im, create_ic_callback, NULL, XCB_XIM_XNInputStyle, &input_style,
        XCB_XIM_XNClientWindow, &client_window, XCB_XIM_XNFocusWindow,
        &client_window, XCB_XIM_XNPreeditAttributes, &nested, NULL));
    free(nested.data);
}

int main() {
    test_xim_server_t *server =
        test_xim_server_new("test_ic_values_server", NULL, NULL);

    int screen_number;
    client_connection = xcb_connect(NULL, &screen_number);
    client_screen = xcb_aux_get_screen(client_connection, screen_number);
    TEST_CHECK(client_screen);

    client_window = xcb_generate_id(client_connection);
    xcb_create_window(client_connection, XCB_COPY_FROM_PARENT, client_window,
                      client_screen->root, 0, 0, 1, 1, 1,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, client_screen->root_visual,
                      0, NULL);

    client_im = xcb_xim_create(client_connection, screen_number,
                               "@im=test_ic_values_server");
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

    test_xim_server_destroy(server);
    return 0;
}
