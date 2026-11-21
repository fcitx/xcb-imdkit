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
#include <string.h>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xproto.h>

static xcb_connection_t *client_connection;
static xcb_screen_t *client_screen;
static xcb_xim_t *client_im;
static xcb_window_t client_window;
static bool completed;
static bool encoding_checked;
static bool disconnect_checked;

static void server_callback(xcb_im_t *im, xcb_im_client_t *client,
                            xcb_im_input_context_t *ic,
                            const xcb_im_packet_header_fr_t *hdr, void *frame,
                            void *arg, void *user_data) {
    if (hdr->major_opcode == XCB_XIM_ENCODING_NEGOTIATION) {
        xcb_im_encoding_negotiation_fr_t *enc_frame = frame;
        TEST_CHECK(enc_frame->supported_list_of_encoding_in_IM_library.size ==
                   1);
        TEST_CHECK(enc_frame->supported_list_of_encoding_in_IM_library.items[0]
                       .length_of_string == strlen("COMPOUND_TEXT"));
        TEST_CHECK(
            memcmp(enc_frame->supported_list_of_encoding_in_IM_library.items[0]
                       .string,
                   "COMPOUND_TEXT", strlen("COMPOUND_TEXT")) == 0);
        encoding_checked = true;
    } else if (hdr->major_opcode == XCB_XIM_FORWARD_EVENT) {
        xcb_im_forward_event(im, ic, arg);
    } else if (hdr->major_opcode == XCB_XIM_DISCONNECT) {
        disconnect_checked = true;
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

static bool atom_exists(xcb_connection_t *conn, const char *name) {
    xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(
        conn, xcb_intern_atom(conn, true, strlen(name), name), NULL);
    xcb_atom_t atom = reply ? reply->atom : XCB_ATOM_NONE;
    free(reply);
    return atom != XCB_ATOM_NONE;
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
    // Client sent 4 property-backed XIM messages (QUERY_EXTENSION,
    // ENCODING_NEGOTIATION, CREATE_IC, FORWARD_EVENT) -> _client0.._client3;
    // Server sent 3 property-backed XIM messages (OPEN_REPLY,
    // QUERY_EXTENSION_REPLY, FORWARD_EVENT) -> _server1_0.._server1_2.
    TEST_CHECK(atom_exists(client_connection, "_client0"));
    TEST_CHECK(atom_exists(client_connection, "_client1"));
    TEST_CHECK(atom_exists(client_connection, "_client2"));
    TEST_CHECK(atom_exists(client_connection, "_client3"));
    TEST_CHECK(!atom_exists(client_connection, "_client4"));
    TEST_CHECK(atom_exists(client_connection, "_server1_0"));
    TEST_CHECK(atom_exists(client_connection, "_server1_1"));
    TEST_CHECK(atom_exists(client_connection, "_server1_2"));
    TEST_CHECK(!atom_exists(client_connection, "_server1_3"));
    xcb_xim_close(client_im);
    free(xcb_get_input_focus_reply(
        client_connection, xcb_get_input_focus(client_connection), NULL));
    xcb_xim_destroy(client_im);
    xcb_disconnect(client_connection);
}

int main() {
    test_xim_server_t *server =
        test_xim_server_new("test_basic_server", server_callback, NULL);

    run_client();
    test_xim_server_destroy(server);
    TEST_CHECK(encoding_checked);
    TEST_CHECK(disconnect_checked);
    return 0;
}
