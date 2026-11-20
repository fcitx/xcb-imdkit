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
#include <string.h>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xproto.h>

static atomic_bool allow_open;
static bool opened;

static void server_callback(xcb_im_t *im, xcb_im_client_t *client,
                            xcb_im_input_context_t *ic,
                            const xcb_im_packet_header_fr_t *hdr, void *frame,
                            void *arg, void *user_data) {
    if (hdr->major_opcode == XCB_XIM_OPEN) {
        TEST_CHECK(atomic_load(&allow_open));
    }
}

static xcb_atom_t intern_atom(xcb_connection_t *conn, const char *name) {
    xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(
        conn, xcb_intern_atom(conn, false, strlen(name), name), NULL);
    TEST_CHECK(reply);
    xcb_atom_t atom = reply->atom;
    free(reply);
    return atom;
}

static xcb_window_t xconnect_to_server(xcb_connection_t *conn,
                                       xcb_window_t server_win,
                                       xcb_window_t client_win,
                                       xcb_atom_t xconnect_atom) {
    xcb_client_message_event_t ev;
    memset(&ev, 0, sizeof(ev));
    ev.response_type = XCB_CLIENT_MESSAGE;
    ev.format = 32;
    ev.window = server_win;
    ev.type = xconnect_atom;
    ev.data.data32[0] = client_win;
    xcb_send_event(conn, false, server_win, XCB_EVENT_MASK_NO_EVENT,
                   (const char *)&ev);
    xcb_flush(conn);

    xcb_generic_event_t *event;
    while ((event = xcb_wait_for_event(conn))) {
        if ((event->response_type & ~0x80) == XCB_CLIENT_MESSAGE) {
            xcb_client_message_event_t *cm =
                (xcb_client_message_event_t *)event;
            if (cm->type == xconnect_atom) {
                xcb_window_t accept_win = cm->data.data32[0];
                free(event);
                return accept_win;
            }
        }
        free(event);
    }
    return XCB_WINDOW_NONE;
}

static void send_inline_connect(xcb_connection_t *conn, xcb_window_t accept_win,
                                xcb_atom_t protocol_atom) {
    uint16_t endian = 1;
    uint8_t byte_order = *(char *)&endian ? 'l' : 'B';
    xcb_client_message_event_t ev;
    memset(&ev, 0, sizeof(ev));
    ev.response_type = XCB_CLIENT_MESSAGE;
    ev.format = 8;
    ev.window = accept_win;
    ev.type = protocol_atom;
    ev.data.data8[0] = XCB_XIM_CONNECT;
    ev.data.data8[1] = 0;
    uint16_t words = 2;
    memcpy(&ev.data.data8[2], &words, sizeof(words));
    ev.data.data8[4] = byte_order;
    xcb_send_event(conn, false, accept_win, XCB_EVENT_MASK_NO_EVENT,
                   (const char *)&ev);
    xcb_flush(conn);

    xcb_generic_event_t *event;
    while ((event = xcb_wait_for_event(conn))) {
        if ((event->response_type & ~0x80) == XCB_CLIENT_MESSAGE) {
            xcb_client_message_event_t *cm =
                (xcb_client_message_event_t *)event;
            if (cm->type == protocol_atom) {
                free(event);
                return;
            }
        }
        free(event);
    }
}

static void send_prop_cm(xcb_connection_t *conn, xcb_window_t target_win,
                         xcb_atom_t protocol_atom, uint32_t length,
                         xcb_atom_t prop_atom) {
    xcb_client_message_event_t ev;
    memset(&ev, 0, sizeof(ev));
    ev.response_type = XCB_CLIENT_MESSAGE;
    ev.format = 32;
    ev.window = target_win;
    ev.type = protocol_atom;
    ev.data.data32[0] = length;
    ev.data.data32[1] = prop_atom;
    xcb_send_event(conn, false, target_win, XCB_EVENT_MASK_NO_EVENT,
                   (const char *)&ev);
    xcb_flush(conn);
}

static void sync_x_roundtrip(xcb_connection_t *conn, xcb_window_t win) {
    free(xcb_get_window_attributes_reply(
        conn, xcb_get_window_attributes(conn, win), NULL));
}

static void open_callback(xcb_xim_t *im, void *user_data) { opened = true; }

static void read_error_reply(xcb_connection_t *conn, xcb_atom_t protocol_atom,
                             xcb_im_error_fr_t *out_err) {
    xcb_generic_event_t *event;
    while ((event = xcb_wait_for_event(conn))) {
        if ((event->response_type & ~0x80) == XCB_CLIENT_MESSAGE) {
            xcb_client_message_event_t *cm =
                (xcb_client_message_event_t *)event;
            if (cm->type == protocol_atom) {
                TEST_CHECK(cm->format == 8);
                TEST_CHECK(cm->data.data8[0] == XCB_XIM_ERROR);
                TEST_CHECK(cm->data.data8[1] == 0);
                uint16_t words = 0;
                memcpy(&words, &cm->data.data8[2], sizeof(words));
                TEST_CHECK(words == 3); // 12-byte XIM_ERROR body
                memset(out_err, 0, sizeof(*out_err));
                memcpy(&out_err->input_method_ID, &cm->data.data8[4], 2);
                memcpy(&out_err->input_context_ID, &cm->data.data8[6], 2);
                memcpy(&out_err->flag, &cm->data.data8[8], 2);
                memcpy(&out_err->Error_Code, &cm->data.data8[10], 2);
                memcpy(&out_err->length_of_error_detail, &cm->data.data8[12],
                       2);
                memcpy(&out_err->type_of_error_detail, &cm->data.data8[14], 2);
                free(event);
                return;
            }
        }
        free(event);
    }
    TEST_CHECK(false);
}

int main() {
    atomic_store(&allow_open, false);
    test_xim_server_t *server =
        test_xim_server_new("test_malformed_server", server_callback, NULL);

    int screen_number;
    xcb_connection_t *conn = xcb_connect(NULL, &screen_number);
    xcb_screen_t *screen = xcb_aux_get_screen(conn, screen_number);
    TEST_CHECK(screen);

    xcb_window_t raw_client_win = xcb_generate_id(conn);
    xcb_create_window(conn, XCB_COPY_FROM_PARENT, raw_client_win, screen->root,
                      0, 0, 1, 1, 1, XCB_WINDOW_CLASS_INPUT_OUTPUT,
                      screen->root_visual, 0, NULL);

    xcb_atom_t xconnect_atom = intern_atom(conn, "_XIM_XCONNECT");
    xcb_atom_t protocol_atom = intern_atom(conn, "_XIM_PROTOCOL");
    xcb_atom_t prop_atom = intern_atom(conn, "_TEST_MALFORMED_PROP");

    xcb_window_t accept_win =
        xconnect_to_server(conn, server->window, raw_client_win, xconnect_atom);
    TEST_CHECK(accept_win != XCB_WINDOW_NONE);
    send_inline_connect(conn, accept_win, protocol_atom);

    // 1. Truncated length (< XCB_IM_HEADER_SIZE) and XCB_ATOM_NONE.
    send_prop_cm(conn, accept_win, protocol_atom, 2, prop_atom);
    send_prop_cm(conn, accept_win, protocol_atom, 16, XCB_ATOM_NONE);

    // 2. Property has only 5 bytes, but ClientMessage claims 20 bytes and the
    // XIM_OPEN header claims 4 * 4 = 16 body bytes (heap OOB read).
    uint8_t short_open[5] = {XCB_XIM_OPEN, 0, 0, 0, 12};
    uint16_t body_words = 4;
    memcpy(&short_open[2], &body_words, sizeof(body_words));
    xcb_change_property(conn, XCB_PROP_MODE_REPLACE, accept_win, prop_atom,
                        XCB_ATOM_STRING, 8, sizeof(short_open), short_open);
    send_prop_cm(conn, accept_win, protocol_atom, 20, prop_atom);

    // 3. Non-zero property offset + 32-bit length wraparound (8 + 0xFFFFFFFC).
    xcb_atom_t wrap_atom = intern_atom(conn, "_TEST_WRAP_PROP");
    uint8_t two_msgs[16] = {0};
    two_msgs[0] = XCB_XIM_QUERY_EXTENSION;
    uint16_t words1 = 1;
    memcpy(&two_msgs[2], &words1, sizeof(words1));
    two_msgs[8] = XCB_XIM_OPEN;
    memcpy(&two_msgs[10], &words1, sizeof(words1));
    xcb_change_property(conn, XCB_PROP_MODE_REPLACE, accept_win, wrap_atom,
                        XCB_ATOM_STRING, 8, sizeof(two_msgs), two_msgs);
    send_prop_cm(conn, accept_win, protocol_atom, 8, wrap_atom);
    send_prop_cm(conn, accept_win, protocol_atom, 0xFFFFFFFCu, wrap_atom);

    // Ensure server has processed all queued requests on Xvfb before enabling
    // allow_open; we sync via a roundtrip to server->window after flushing.
    sync_x_roundtrip(conn, server->window);

    // Wait briefly by doing a clean xconnect roundtrip so the server thread
    // has definitely drained all prior ClientMessages before allow_open=true.
    xcb_window_t barrier_win = xcb_generate_id(conn);
    xcb_create_window(conn, XCB_COPY_FROM_PARENT, barrier_win, screen->root, 0,
                      0, 1, 1, 1, XCB_WINDOW_CLASS_INPUT_OUTPUT,
                      screen->root_visual, 0, NULL);
    xcb_window_t barrier_accept =
        xconnect_to_server(conn, server->window, barrier_win, xconnect_atom);
    TEST_CHECK(barrier_accept != XCB_WINDOW_NONE);
    send_inline_connect(conn, barrier_accept, protocol_atom);

    // 4. Malformed XIM_OPEN (string length 10 > 4-byte body) must elicit a
    // valid 12-byte XIM_ERROR frame with XIM_IMID_VALID (0x0001).
    {
        xcb_client_message_event_t bad_open = {
            .response_type = XCB_CLIENT_MESSAGE,
            .format = 8,
            .window = barrier_accept,
            .type = protocol_atom,
        };
        bad_open.data.data8[0] = XCB_XIM_OPEN;
        uint16_t words = 1;
        memcpy(&bad_open.data.data8[2], &words, sizeof(words));
        bad_open.data.data8[4] = 10; // length_of_string > 4 bytes
        xcb_send_event(conn, false, barrier_accept, XCB_EVENT_MASK_NO_EVENT,
                       (const char *)&bad_open);
        xcb_flush(conn);

        xcb_im_error_fr_t err_fr;
        read_error_reply(conn, protocol_atom, &err_fr);
        TEST_CHECK(err_fr.flag == 0x0001);
        TEST_CHECK(err_fr.input_method_ID != 0);
        TEST_CHECK(err_fr.Error_Code == 13);
        uint16_t imid = err_fr.input_method_ID;

        // IC-scoped request with unknown icid must elicit a 12-byte XIM_ERROR
        // with both XIM_IMID_VALID | XIM_ICID_VALID (0x0003).
        xcb_client_message_event_t bad_getic = {
            .response_type = XCB_CLIENT_MESSAGE,
            .format = 8,
            .window = barrier_accept,
            .type = protocol_atom,
        };
        bad_getic.data.data8[0] = XCB_XIM_GET_IC_VALUES;
        words = 2;
        memcpy(&bad_getic.data.data8[2], &words, sizeof(words));
        uint16_t icid = 0x4321;
        uint16_t byte_len = 0;
        memcpy(&bad_getic.data.data8[4], &imid, sizeof(imid));
        memcpy(&bad_getic.data.data8[6], &icid, sizeof(icid));
        memcpy(&bad_getic.data.data8[8], &byte_len, sizeof(byte_len));
        xcb_send_event(conn, false, barrier_accept, XCB_EVENT_MASK_NO_EVENT,
                       (const char *)&bad_getic);
        xcb_flush(conn);

        read_error_reply(conn, protocol_atom, &err_fr);
        TEST_CHECK(err_fr.flag == 0x0003);
        TEST_CHECK(err_fr.input_method_ID == imid);
        TEST_CHECK(err_fr.input_context_ID == icid);
        TEST_CHECK(err_fr.Error_Code == 13);
    }

    // Now allow legitimate XIM_OPEN and verify normal client works.
    atomic_store(&allow_open, true);
    xcb_xim_t *client_im =
        xcb_xim_create(conn, screen_number, "@im=test_malformed_server");
    TEST_CHECK(xcb_xim_open(client_im, open_callback, true, NULL));

    xcb_generic_event_t *event;
    while (!opened && (event = xcb_wait_for_event(conn))) {
        xcb_xim_filter_event(client_im, event);
        free(event);
    }
    TEST_CHECK(opened);

    xcb_xim_close(client_im);
    xcb_xim_destroy(client_im);
    xcb_destroy_window(conn, barrier_win);
    xcb_destroy_window(conn, raw_client_win);
    xcb_disconnect(conn);

    test_xim_server_destroy(server);
    return 0;
}
