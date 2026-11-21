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
#include <stdatomic.h>
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
static atomic_bool ext_move_notified;
static bool completed;

static void server_callback(xcb_im_t *im, xcb_im_client_t *client,
                            xcb_im_input_context_t *ic,
                            const xcb_im_packet_header_fr_t *hdr, void *frame,
                            void *arg, void *user_data) {
    if (hdr->major_opcode == XCB_XIM_EXTENSION &&
        hdr->minor_opcode == XCB_XIM_EXT_MOVE) {
        TEST_CHECK(xcb_im_input_context_get_preedit_attr_mask(ic) &
                   XCB_XIM_XNSpotLocation_MASK);
        const xcb_im_preedit_attr_t *attr =
            xcb_im_input_context_get_preedit_attr(ic);
        TEST_CHECK(attr->spot_location.x == 123);
        TEST_CHECK(attr->spot_location.y == 456);
        atomic_store(&ext_move_notified, true);
    }
}

static void
second_get_ic_values_callback(xcb_xim_t *im, xcb_xic_t ic,
                              xcb_im_get_ic_values_reply_fr_t *reply,
                              void *user_data) {
    TEST_CHECK(reply);
    TEST_CHECK(atomic_load(&ext_move_notified));
    completed = true;
}

static void second_create_ic_callback(xcb_xim_t *im, xcb_xic_t ic,
                                      void *user_data) {
    TEST_CHECK(ic);
    TEST_CHECK(xcb_xim_ext_move(client_im, ic, 123, 456));
    TEST_CHECK(xcb_xim_get_ic_values(client_im, ic,
                                     second_get_ic_values_callback, NULL,
                                     XCB_XIM_XNFilterEvents, NULL));
}

static void get_ic_values_callback(xcb_xim_t *im, xcb_xic_t ic,
                                   xcb_im_get_ic_values_reply_fr_t *reply,
                                   void *user_data) {
    TEST_CHECK(reply);
    // Deduplicated top-level: 1 XNFilterEvents + 1 nested XNPreeditAttributes
    // (containing 2 XNSpotLocation entries = 16 bytes).
    TEST_CHECK(reply->ic_attribute.size == 2);
    TEST_CHECK(reply->ic_attribute.items[0].value_length == 4);
    TEST_CHECK(reply->ic_attribute.items[1].value_length == 16);

    // Create a second IC without initial XNSpotLocation and update spot via
    // XIM_EXT_MOVE.
    uint32_t input_style = XCB_IM_PreeditNothing | XCB_IM_StatusNothing;
    TEST_CHECK(xcb_xim_create_ic(client_im, second_create_ic_callback, NULL,
                                 XCB_XIM_XNInputStyle, &input_style,
                                 XCB_XIM_XNClientWindow, &client_window, NULL));
}

static void create_ic_callback(xcb_xim_t *im, xcb_xic_t ic, void *user_data) {
    TEST_CHECK(ic);
    TEST_CHECK(!xcb_xim_set_ic_values(client_im, ic, NULL, NULL, NULL));
    TEST_CHECK(!xcb_xim_set_ic_values(client_im, ic, NULL, NULL,
                                      "unknownAttribute", &ic, NULL));

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
    uint32_t fg = 1;
    uint32_t bg = 2;

    // Empty or invalid IC attributes in xcb_xim_create_ic must fail upfront.
    TEST_CHECK(!xcb_xim_create_ic(client_im, create_ic_callback, NULL, NULL));
    TEST_CHECK(!xcb_xim_create_ic(client_im, create_ic_callback, NULL,
                                  XCB_XIM_XNInputStyle, &input_style,
                                  "unknownAttribute", &fg, NULL));
    xcb_xim_nested_list bad_nested = xcb_xim_create_nested_list(
        client_im, "unknownNestedAttribute", &fg, NULL);
    TEST_CHECK(!bad_nested.data && bad_nested.length == 0);
    TEST_CHECK(!xcb_xim_create_ic(
        client_im, create_ic_callback, NULL, XCB_XIM_XNInputStyle, &input_style,
        XCB_XIM_XNPreeditAttributes, &bad_nested, NULL));

    // 3 * 8 = 24 bytes (exact glibc 64-bit chunk usable size), plus an
    // unsupported-type XNFontSet attribute (size 0) that previously wrote 4
    // bytes past the end of the 24-byte heap buffer.
    xcb_xim_nested_list nested = xcb_xim_create_nested_list(
        client_im, XCB_XIM_XNSpotLocation, &spot, XCB_XIM_XNForeground, &fg,
        XCB_XIM_XNBackground, &bg, XCB_XIM_XNFontSet, "fixed", NULL);
    TEST_CHECK(nested.data);
    TEST_CHECK(nested.length == 24);
    TEST_CHECK(xcb_xim_create_ic(
        client_im, create_ic_callback, NULL, XCB_XIM_XNInputStyle, &input_style,
        XCB_XIM_XNClientWindow, &client_window, XCB_XIM_XNFocusWindow,
        &client_window, XCB_XIM_XNPreeditAttributes, &nested, NULL));
    free(nested.data);
}

int main() {
    test_xim_server_t *server =
        test_xim_server_new("test_ic_values_server", server_callback, NULL);

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
