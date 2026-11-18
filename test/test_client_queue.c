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
static xcb_xic_t client_ic;
static unsigned int set_ic_values_replies;
static unsigned int forwarded_events;
static bool reset_replied;
static bool completed;

static atomic_uint server_set_ic_values_count;
static atomic_uint server_forwarded_events;

static void server_callback(xcb_im_t *im, xcb_im_client_t *client,
                            xcb_im_input_context_t *ic,
                            const xcb_im_packet_header_fr_t *hdr, void *frame,
                            void *arg, void *user_data) {
    if (hdr->major_opcode == XCB_XIM_SET_IC_VALUES) {
        unsigned int idx = atomic_fetch_add(&server_set_ic_values_count, 1);
        TEST_CHECK(xcb_im_input_context_get_preedit_attr_mask(ic) &
                   XCB_XIM_XNSpotLocation_MASK);
        const xcb_im_preedit_attr_t *attr =
            xcb_im_input_context_get_preedit_attr(ic);
        if (idx == 0) {
            TEST_CHECK(attr->spot_location.x == 10);
            TEST_CHECK(attr->spot_location.y == 20);
        } else if (idx == 1) {
            TEST_CHECK(attr->spot_location.x == 30);
            TEST_CHECK(attr->spot_location.y == 40);
        } else {
            TEST_CHECK(false);
        }
    } else if (hdr->major_opcode == XCB_XIM_FORWARD_EVENT) {
        TEST_CHECK(atomic_load(&server_set_ic_values_count) == 2);
        atomic_fetch_add(&server_forwarded_events, 1);
        xcb_im_forward_event(im, ic, arg);
    }
}

static void update_spot(int16_t x, int16_t y,
                        xcb_xim_set_ic_values_callback cb) {
    xcb_point_t spot = {.x = x, .y = y};
    xcb_xim_nested_list nested = xcb_xim_create_nested_list(
        client_im, XCB_XIM_XNSpotLocation, &spot, NULL);
    TEST_CHECK(nested.data);
    TEST_CHECK(xcb_xim_set_ic_values(client_im, client_ic, cb, NULL,
                                     XCB_XIM_XNPreeditAttributes, &nested,
                                     NULL));
    free(nested.data);
}

static void set_ic_values_callback(xcb_xim_t *im, xcb_xic_t ic,
                                   void *user_data) {
    TEST_CHECK(ic == client_ic);
    set_ic_values_replies++;
}

static void reset_ic_callback(xcb_xim_t *im, xcb_xic_t ic,
                              xcb_im_reset_ic_reply_fr_t *reply,
                              void *user_data) {
    TEST_CHECK(ic == client_ic);
    TEST_CHECK(reply != NULL);
    TEST_CHECK(!reset_replied);
    reset_replied = true;
}

static void destroy_ic_callback(xcb_xim_t *im, xcb_xic_t ic, void *user_data) {
    TEST_CHECK(ic == client_ic);
    TEST_CHECK(reset_replied);
    completed = true;
}

static void forward_event(xcb_xim_t *im, xcb_xic_t ic,
                          xcb_key_press_event_t *event, void *user_data) {
    TEST_CHECK(ic == client_ic);
    TEST_CHECK(set_ic_values_replies == 2);
    TEST_CHECK(event->response_type == XCB_KEY_PRESS);
    TEST_CHECK(event->detail == 38 + forwarded_events);
    forwarded_events++;

    if (forwarded_events == 2) {
        // Queue two different IC requests back-to-back with no follow-up call
        // inside reset_ic_callback.
        TEST_CHECK(
            xcb_xim_reset_ic(client_im, client_ic, reset_ic_callback, NULL));
        TEST_CHECK(xcb_xim_destroy_ic(client_im, client_ic, destroy_ic_callback,
                                      NULL));
    }
}

static void create_ic_callback(xcb_xim_t *im, xcb_xic_t ic, void *user_data) {
    TEST_CHECK(ic);
    client_ic = ic;

    // Issue #21: two back-to-back XIM_SET_IC_VALUES spot updates followed by
    // forwarded key events, with no further client requests triggered from
    // set_ic_values_callback.
    update_spot(10, 20, set_ic_values_callback);
    update_spot(30, 40, set_ic_values_callback);

    for (uint8_t i = 0; i < 2; i++) {
        xcb_key_press_event_t event = {
            .response_type = XCB_KEY_PRESS,
            .root = client_screen->root,
            .event = client_window,
            .detail = 38 + i,
        };
        TEST_CHECK(xcb_xim_forward_event(client_im, ic, &event));
    }
}

static void open_callback(xcb_xim_t *im, void *user_data) {
    uint32_t input_style = XCB_IM_PreeditPosition | XCB_IM_StatusNothing;
    xcb_point_t spot = {.x = 0, .y = 0};
    xcb_xim_nested_list nested = xcb_xim_create_nested_list(
        client_im, XCB_XIM_XNSpotLocation, &spot, NULL);
    TEST_CHECK(nested.data);
    TEST_CHECK(xcb_xim_create_ic(
        client_im, create_ic_callback, NULL, XCB_XIM_XNInputStyle, &input_style,
        XCB_XIM_XNClientWindow, &client_window, XCB_XIM_XNFocusWindow,
        &client_window, XCB_XIM_XNPreeditAttributes, &nested, NULL));
    free(nested.data);
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
                               "@im=test_client_queue_server");
    xcb_xim_set_im_callback(client_im, &callbacks, NULL);
    TEST_CHECK(xcb_xim_open(client_im, open_callback, true, NULL));

    xcb_generic_event_t *event;
    while (!completed && (event = xcb_wait_for_event(client_connection))) {
        xcb_xim_filter_event(client_im, event);
        free(event);
    }

    TEST_CHECK(completed);
    TEST_CHECK(set_ic_values_replies == 2);
    TEST_CHECK(forwarded_events == 2);
    TEST_CHECK(reset_replied);
    xcb_xim_close(client_im);
    xcb_xim_destroy(client_im);
    xcb_disconnect(client_connection);
}

int main() {
    test_xim_server_t *server =
        test_xim_server_new("test_client_queue_server", server_callback, NULL);

    run_client();
    test_xim_server_destroy(server);
    TEST_CHECK(atomic_load(&server_set_ic_values_count) == 2);
    TEST_CHECK(atomic_load(&server_forwarded_events) == 2);
    return 0;
}
