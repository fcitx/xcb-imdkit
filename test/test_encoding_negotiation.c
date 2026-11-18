/*
 * SPDX-FileCopyrightText: 2026 Weng Xuetian <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 *
 */

#include "imclient.h"
#include "test.h"
#include "ximproto.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xproto.h>

static bool opened;

static void open_callback(xcb_xim_t *im, void *user_data) { opened = true; }

static xcb_atom_t intern_atom(xcb_connection_t *conn, const char *name) {
    xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(
        conn, xcb_intern_atom(conn, false, strlen(name), name), NULL);
    TEST_CHECK(reply);
    xcb_atom_t atom = reply->atom;
    free(reply);
    return atom;
}

static xcb_generic_event_t *wait_event_type(xcb_connection_t *conn,
                                            uint8_t type) {
    xcb_generic_event_t *event;
    while ((event = xcb_wait_for_event(conn))) {
        if ((event->response_type & ~0x80) == type) {
            return event;
        }
        free(event);
    }
    TEST_CHECK(false);
    return NULL;
}

static void send_inline_xim(xcb_connection_t *conn, xcb_window_t target_win,
                            xcb_atom_t protocol_atom, uint8_t major_opcode,
                            const void *body, size_t body_len) {
    TEST_CHECK(body_len % 4 == 0 && body_len <= 16);
    xcb_client_message_event_t ev;
    memset(&ev, 0, sizeof(ev));
    ev.response_type = XCB_CLIENT_MESSAGE;
    ev.format = 8;
    ev.window = target_win;
    ev.type = protocol_atom;
    ev.data.data8[0] = major_opcode;
    ev.data.data8[1] = 0;
    uint16_t words = body_len / 4;
    memcpy(&ev.data.data8[2], &words, sizeof(words));
    if (body_len) {
        memcpy(&ev.data.data8[4], body, body_len);
    }
    xcb_send_event(conn, false, target_win, XCB_EVENT_MASK_NO_EVENT,
                   (const char *)&ev);
    xcb_flush(conn);
}

static void pump_one_client_event(xcb_connection_t *client_conn,
                                  xcb_xim_t *client_im) {
    xcb_generic_event_t *ev = wait_event_type(client_conn, XCB_CLIENT_MESSAGE);
    xcb_xim_filter_event(client_im, ev);
    free(ev);
}

int main() {
    int screen_num;
    xcb_connection_t *server_conn = xcb_connect(NULL, &screen_num);
    xcb_screen_t *server_screen = xcb_aux_get_screen(server_conn, screen_num);
    TEST_CHECK(server_screen);

    xcb_window_t server_win = xcb_generate_id(server_conn);
    xcb_create_window(server_conn, XCB_COPY_FROM_PARENT, server_win,
                      server_screen->root, 0, 0, 1, 1, 1,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, server_screen->root_visual,
                      0, NULL);
    xcb_window_t accept_win = xcb_generate_id(server_conn);
    xcb_create_window(server_conn, XCB_COPY_FROM_PARENT, accept_win,
                      server_screen->root, 0, 0, 1, 1, 1,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, server_screen->root_visual,
                      0, NULL);

    xcb_atom_t server_atom = intern_atom(server_conn, "@server=test_enc_neg");
    xcb_atom_t servers_atom = intern_atom(server_conn, "XIM_SERVERS");
    xcb_atom_t xconnect_atom = intern_atom(server_conn, "_XIM_XCONNECT");
    xcb_atom_t protocol_atom = intern_atom(server_conn, "_XIM_PROTOCOL");

    xcb_set_selection_owner(server_conn, server_win, server_atom,
                            XCB_CURRENT_TIME);
    xcb_change_property(server_conn, XCB_PROP_MODE_REPLACE, server_screen->root,
                        servers_atom, XCB_ATOM_ATOM, 32, 1, &server_atom);
    xcb_flush(server_conn);
    free(xcb_get_selection_owner_reply(
        server_conn, xcb_get_selection_owner(server_conn, server_atom), NULL));

    int client_screen_num;
    xcb_connection_t *client_conn = xcb_connect(NULL, &client_screen_num);
    TEST_CHECK(!xcb_connection_has_error(client_conn));

    xcb_xim_t *client_im =
        xcb_xim_create(client_conn, client_screen_num, "@im=test_enc_neg");
    xcb_xim_set_use_compound_text(client_im, false);
    xcb_xim_set_use_utf8_string(client_im, true);
    TEST_CHECK(xcb_xim_open(client_im, open_callback, false, NULL));

    // 1. Handle TRANSPORT SelectionRequest -> SelectionNotify.
    xcb_generic_event_t *sev =
        wait_event_type(server_conn, XCB_SELECTION_REQUEST);
    xcb_selection_request_event_t *sreq = (xcb_selection_request_event_t *)sev;
    const char transport[] = "@transport=X/";
    xcb_change_property(server_conn, XCB_PROP_MODE_REPLACE, sreq->requestor,
                        sreq->property, sreq->target, 8, strlen(transport),
                        transport);
    xcb_selection_notify_event_t snotify;
    memset(&snotify, 0, sizeof(snotify));
    snotify.response_type = XCB_SELECTION_NOTIFY;
    snotify.requestor = sreq->requestor;
    snotify.selection = sreq->selection;
    snotify.target = sreq->target;
    snotify.property = sreq->property;
    snotify.time = sreq->time;
    xcb_send_event(server_conn, false, sreq->requestor, XCB_EVENT_MASK_NO_EVENT,
                   (const char *)&snotify);
    xcb_flush(server_conn);
    free(sev);

    xcb_generic_event_t *cev =
        wait_event_type(client_conn, XCB_SELECTION_NOTIFY);
    xcb_xim_filter_event(client_im, cev);
    free(cev);

    // 2. Handle _XIM_XCONNECT.
    sev = wait_event_type(server_conn, XCB_CLIENT_MESSAGE);
    xcb_client_message_event_t *cm = (xcb_client_message_event_t *)sev;
    TEST_CHECK(cm->type == xconnect_atom);
    xcb_window_t client_comm_win = cm->data.data32[0];
    free(sev);

    xcb_client_message_event_t xconn_reply;
    memset(&xconn_reply, 0, sizeof(xconn_reply));
    xconn_reply.response_type = XCB_CLIENT_MESSAGE;
    xconn_reply.format = 32;
    xconn_reply.window = client_comm_win;
    xconn_reply.type = xconnect_atom;
    xconn_reply.data.data32[0] = accept_win;
    xcb_send_event(server_conn, false, client_comm_win, XCB_EVENT_MASK_NO_EVENT,
                   (const char *)&xconn_reply);
    xcb_flush(server_conn);

    pump_one_client_event(client_conn, client_im);

    // 3. Receive XIM_CONNECT -> send XIM_CONNECT_REPLY.
    free(wait_event_type(server_conn, XCB_CLIENT_MESSAGE));
    uint8_t connect_reply_body[4] = {0};
    send_inline_xim(server_conn, client_comm_win, protocol_atom,
                    XCB_XIM_CONNECT_REPLY, connect_reply_body,
                    sizeof(connect_reply_body));
    pump_one_client_event(client_conn, client_im);

    // 4. Receive XIM_OPEN -> send minimal XIM_OPEN_REPLY (imid = 1).
    free(wait_event_type(server_conn, XCB_CLIENT_MESSAGE));
    uint16_t open_reply_body[4] = {1, 0, 0, 0};
    send_inline_xim(server_conn, client_comm_win, protocol_atom,
                    XCB_XIM_OPEN_REPLY, open_reply_body,
                    sizeof(open_reply_body));
    pump_one_client_event(client_conn, client_im);

    // 5. Receive XIM_QUERY_EXTENSION -> send minimal XIM_QUERY_EXTENSION_REPLY.
    free(wait_event_type(server_conn, XCB_CLIENT_MESSAGE));
    uint16_t ext_reply_body[2] = {1, 0};
    send_inline_xim(server_conn, client_comm_win, protocol_atom,
                    XCB_XIM_QUERY_EXTENSION_REPLY, ext_reply_body,
                    sizeof(ext_reply_body));
    pump_one_client_event(client_conn, client_im);

    // 6. Receive XIM_ENCODING_NEGOTIATION.
    free(wait_event_type(server_conn, XCB_CLIENT_MESSAGE));

    // 6a. Mismatched input_method_ID (999 != 1) with valid index 0 -> ignored.
    uint16_t bad_imid_body[4] = {999, 0, 0, 0};
    send_inline_xim(server_conn, client_comm_win, protocol_atom,
                    XCB_XIM_ENCODING_NEGOTIATION_REPLY, bad_imid_body,
                    sizeof(bad_imid_body));
    pump_one_client_event(client_conn, client_im);
    TEST_CHECK(!opened);

    // 6b. Matching input_method_ID (1) with out-of-bounds index 5000 ->
    // ignored (must not read encodings[5000] off stack).
    uint16_t bad_idx_body[4] = {1, 0, 5000, 0};
    send_inline_xim(server_conn, client_comm_win, protocol_atom,
                    XCB_XIM_ENCODING_NEGOTIATION_REPLY, bad_idx_body,
                    sizeof(bad_idx_body));
    pump_one_client_event(client_conn, client_im);
    TEST_CHECK(!opened);

    // 6c. Default encoding fallback index (-1 / UINT16_MAX) -> accepted and
    // falls back to XCB_XIM_COMPOUND_TEXT even though client only requested
    // UTF8_STRING.
    uint16_t fallback_body[4] = {1, 0, UINT16_MAX, 0};
    send_inline_xim(server_conn, client_comm_win, protocol_atom,
                    XCB_XIM_ENCODING_NEGOTIATION_REPLY, fallback_body,
                    sizeof(fallback_body));
    pump_one_client_event(client_conn, client_im);
    TEST_CHECK(opened);
    TEST_CHECK(xcb_xim_get_encoding(client_im) == XCB_XIM_COMPOUND_TEXT);

    xcb_xim_close(client_im);
    xcb_xim_destroy(client_im);
    xcb_disconnect(client_conn);
    xcb_disconnect(server_conn);
    return 0;
}
