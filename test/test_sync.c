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
#include <string.h>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xproto.h>

static xcb_connection_t *client_connection;
static xcb_screen_t *client_screen;
static xcb_xim_t *client_im;
static xcb_window_t client_window;
static xcb_xic_t first_ic;
static xcb_xic_t second_ic;
static bool destroy_requested;
static bool completed;

static void server_callback(xcb_im_t *im, xcb_im_client_t *client,
                            xcb_im_input_context_t *ic,
                            const xcb_im_packet_header_fr_t *hdr, void *frame,
                            void *arg, void *user_data) {
    if (hdr->major_opcode == XCB_XIM_FORWARD_EVENT) {
        xcb_key_press_event_t *ev = arg;
        if (ev->detail == 99) {
            xcb_im_set_use_sync_event(im, true);
        }
        xcb_im_forward_event(im, ic, ev);
    }
}

static void send_event(xcb_xic_t ic) {
    xcb_key_press_event_t event = {
        .response_type = XCB_KEY_PRESS,
        .root = client_screen->root,
        .event = client_window,
        .detail = 38,
    };
    TEST_CHECK(xcb_xim_forward_event(client_im, ic, &event));
}

static void create_ic_callback(xcb_xim_t *im, xcb_xic_t ic, void *user_data) {
    TEST_CHECK(ic);
    if (!first_ic) {
        first_ic = ic;
    } else {
        second_ic = ic;
    }
    send_event(ic);
}

static void create_ic(void) {
    uint32_t input_style = XCB_IM_PreeditNothing | XCB_IM_StatusNothing;
    TEST_CHECK(xcb_xim_create_ic(client_im, create_ic_callback, NULL,
                                 XCB_XIM_XNInputStyle, &input_style,
                                 XCB_XIM_XNClientWindow, &client_window,
                                 XCB_XIM_XNFocusWindow, &client_window, NULL));
}

static void destroy_ic_callback(xcb_xim_t *im, xcb_xic_t ic, void *user_data) {
    TEST_CHECK(ic == first_ic);
    create_ic();
}

static void client_forward_event(xcb_xim_t *im, xcb_xic_t ic,
                                 xcb_key_press_event_t *event,
                                 void *user_data) {
    if (!destroy_requested) {
        destroy_requested = true;
        TEST_CHECK(
            xcb_xim_destroy_ic(client_im, ic, destroy_ic_callback, NULL));
        return;
    }

    TEST_CHECK(ic == second_ic);
    completed = true;
}

static void open_callback(xcb_xim_t *im, void *user_data) { create_ic(); }

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
        .forward_event = client_forward_event,
    };
    client_im = xcb_xim_create(client_connection, screen_number,
                               "@im=test_sync_server");
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

static xcb_atom_t intern_atom(xcb_connection_t *conn, const char *name) {
    xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(
        conn, xcb_intern_atom(conn, false, strlen(name), name), NULL);
    TEST_CHECK(reply);
    xcb_atom_t atom = reply->atom;
    free(reply);
    return atom;
}

static xcb_generic_event_t *wait_client_message(xcb_connection_t *conn) {
    xcb_generic_event_t *event;
    while ((event = xcb_wait_for_event(conn))) {
        if ((event->response_type & ~0x80) == XCB_CLIENT_MESSAGE) {
            return event;
        }
        free(event);
    }
    TEST_CHECK(false);
    return NULL;
}

static void send_native_inline(xcb_connection_t *conn, xcb_window_t accept_win,
                               xcb_atom_t protocol_atom, uint8_t opcode,
                               const void *body, size_t body_len) {
    TEST_CHECK(body_len % 4 == 0 && body_len <= 16);
    xcb_client_message_event_t ev;
    memset(&ev, 0, sizeof(ev));
    ev.response_type = XCB_CLIENT_MESSAGE;
    ev.format = 8;
    ev.window = accept_win;
    ev.type = protocol_atom;
    ev.data.data8[0] = opcode;
    ev.data.data8[1] = 0;
    uint16_t words = (uint16_t)(body_len / 4);
    memcpy(&ev.data.data8[2], &words, 2);
    if (body_len) {
        memcpy(&ev.data.data8[4], body, body_len);
    }
    xcb_send_event(conn, false, accept_win, XCB_EVENT_MASK_NO_EVENT,
                   (const char *)&ev);
    xcb_flush(conn);
}

static void send_native_property(xcb_connection_t *conn,
                                 xcb_window_t accept_win,
                                 xcb_atom_t protocol_atom, xcb_atom_t prop,
                                 uint8_t opcode, const void *body,
                                 size_t body_len) {
    TEST_CHECK(body_len % 4 == 0);
    size_t total = 4 + body_len;
    uint8_t *buf = calloc(1, total);
    TEST_CHECK(buf);
    buf[0] = opcode;
    buf[1] = 0;
    uint16_t words = (uint16_t)(body_len / 4);
    memcpy(&buf[2], &words, 2);
    if (body_len) {
        memcpy(&buf[4], body, body_len);
    }
    xcb_change_property(conn, XCB_PROP_MODE_APPEND, accept_win, prop,
                        XCB_ATOM_STRING, 8, total, buf);
    free(buf);

    xcb_client_message_event_t ev;
    memset(&ev, 0, sizeof(ev));
    ev.response_type = XCB_CLIENT_MESSAGE;
    ev.format = 32;
    ev.window = accept_win;
    ev.type = protocol_atom;
    ev.data.data32[0] = total;
    ev.data.data32[1] = prop;
    xcb_send_event(conn, false, accept_win, XCB_EVENT_MASK_NO_EVENT,
                   (const char *)&ev);
    xcb_flush(conn);
}

static uint8_t *read_native_reply(xcb_connection_t *conn, xcb_window_t comm_win,
                                  uint8_t *opcode_out, size_t *body_len_out) {
    xcb_generic_event_t *gev = wait_client_message(conn);
    xcb_client_message_event_t *ev = (xcb_client_message_event_t *)gev;
    uint8_t *body = NULL;
    if (ev->format == 8) {
        *opcode_out = ev->data.data8[0];
        uint16_t words;
        memcpy(&words, &ev->data.data8[2], 2);
        size_t len = (size_t)words * 4;
        TEST_CHECK(len <= 16);
        body = malloc(len ? len : 1);
        TEST_CHECK(body);
        if (len) {
            memcpy(body, &ev->data.data8[4], len);
        }
        *body_len_out = len;
    } else if (ev->format == 32) {
        size_t total = ev->data.data32[0];
        xcb_atom_t atom = ev->data.data32[1];
        xcb_get_property_reply_t *reply = xcb_get_property_reply(
            conn,
            xcb_get_property(conn, true, comm_win, atom, XCB_ATOM_ANY, 0,
                             (total + 3) / 4),
            NULL);
        TEST_CHECK(reply);
        uint8_t *val = xcb_get_property_value(reply);
        *opcode_out = val[0];
        uint16_t words;
        memcpy(&words, &val[2], 2);
        size_t len = (size_t)words * 4;
        TEST_CHECK(4 + len <= (size_t)xcb_get_property_value_length(reply));
        body = malloc(len ? len : 1);
        TEST_CHECK(body);
        if (len) {
            memcpy(body, val + 4, len);
        }
        *body_len_out = len;
        free(reply);
    } else {
        TEST_CHECK(false);
    }
    free(gev);
    return body;
}

static void send_forward_key(xcb_connection_t *conn, xcb_window_t accept_win,
                             xcb_atom_t protocol_atom, xcb_atom_t prop,
                             uint16_t imid, uint16_t icid, uint16_t flag,
                             uint16_t seq, uint8_t keycode,
                             xcb_window_t root_win, xcb_window_t event_win) {
    uint8_t body[8 + sizeof(xcb_key_press_event_t)];
    memset(body, 0, sizeof(body));
    memcpy(&body[0], &imid, 2);
    memcpy(&body[2], &icid, 2);
    memcpy(&body[4], &flag, 2);
    memcpy(&body[6], &seq, 2);

    xcb_key_press_event_t key;
    memset(&key, 0, sizeof(key));
    key.response_type = XCB_KEY_PRESS;
    key.detail = keycode;
    key.root = root_win;
    key.event = event_win;
    memcpy(&body[8], &key, sizeof(key));

    send_native_property(conn, accept_win, protocol_atom, prop,
                         XCB_XIM_FORWARD_EVENT, body, sizeof(body));
}

static void run_xim_sync_request_test(xcb_window_t server_win) {
    int screen_num;
    xcb_connection_t *conn = xcb_connect(NULL, &screen_num);
    xcb_screen_t *screen = xcb_aux_get_screen(conn, screen_num);
    TEST_CHECK(screen);

    xcb_window_t comm_win = xcb_generate_id(conn);
    xcb_create_window(conn, XCB_COPY_FROM_PARENT, comm_win, screen->root, 0, 0,
                      1, 1, 1, XCB_WINDOW_CLASS_INPUT_OUTPUT,
                      screen->root_visual, 0, NULL);

    xcb_atom_t xconnect_atom = intern_atom(conn, "_XIM_XCONNECT");
    xcb_atom_t protocol_atom = intern_atom(conn, "_XIM_PROTOCOL");
    xcb_atom_t client_prop = intern_atom(conn, "_CLIENT_SYNC_PROP");

    xcb_client_message_event_t xconn_ev;
    memset(&xconn_ev, 0, sizeof(xconn_ev));
    xconn_ev.response_type = XCB_CLIENT_MESSAGE;
    xconn_ev.format = 32;
    xconn_ev.window = server_win;
    xconn_ev.type = xconnect_atom;
    xconn_ev.data.data32[0] = comm_win;
    xcb_send_event(conn, false, server_win, XCB_EVENT_MASK_NO_EVENT,
                   (const char *)&xconn_ev);
    xcb_flush(conn);

    xcb_generic_event_t *gev = wait_client_message(conn);
    xcb_client_message_event_t *xconn_reply = (xcb_client_message_event_t *)gev;
    TEST_CHECK(xconn_reply->type == xconnect_atom);
    xcb_window_t accept_win = xconn_reply->data.data32[0];
    free(gev);

    uint16_t endian = 1;
    uint8_t native_order = (*(char *)&endian) ? 'l' : 'B';
    uint8_t connect_body[8] = {native_order, 0, 1, 0, 0, 0, 0, 0};
    send_native_inline(conn, accept_win, protocol_atom, XCB_XIM_CONNECT,
                       connect_body, sizeof(connect_body));

    uint8_t opcode = 0;
    size_t reply_len = 0;
    uint8_t *reply = read_native_reply(conn, comm_win, &opcode, &reply_len);
    TEST_CHECK(opcode == XCB_XIM_CONNECT_REPLY);
    free(reply);

    uint8_t open_body[4] = {1, 'C', 0, 0};
    send_native_inline(conn, accept_win, protocol_atom, XCB_XIM_OPEN, open_body,
                       sizeof(open_body));

    reply = read_native_reply(conn, comm_win, &opcode, &reply_len);
    TEST_CHECK(opcode == XCB_XIM_OPEN_REPLY);
    uint16_t imid;
    memcpy(&imid, reply, 2);
    uint16_t im_attr_bytes;
    memcpy(&im_attr_bytes, reply + 2, 2);
    size_t ic_list_off = 4 + im_attr_bytes;
    uint16_t ic_attr_bytes;
    memcpy(&ic_attr_bytes, reply + ic_list_off, 2);
    size_t pos = ic_list_off + 4;
    uint16_t input_style_id = UINT16_MAX;
    uint16_t client_win_id = UINT16_MAX;
    while (pos < ic_list_off + 4 + ic_attr_bytes) {
        uint16_t attr_id, name_len;
        memcpy(&attr_id, reply + pos, 2);
        memcpy(&name_len, reply + pos + 4, 2);
        const char *name = (const char *)(reply + pos + 6);
        if (name_len == strlen(XCB_XIM_XNInputStyle) &&
            memcmp(name, XCB_XIM_XNInputStyle, name_len) == 0) {
            input_style_id = attr_id;
        } else if (name_len == strlen(XCB_XIM_XNClientWindow) &&
                   memcmp(name, XCB_XIM_XNClientWindow, name_len) == 0) {
            client_win_id = attr_id;
        }
        pos += ((6 + name_len + 3) & ~3u);
    }
    free(reply);
    TEST_CHECK(input_style_id != UINT16_MAX && client_win_id != UINT16_MAX);

    reply = read_native_reply(conn, comm_win, &opcode, &reply_len);
    TEST_CHECK(opcode == XCB_XIM_SET_EVENT_MASK);
    free(reply);

    uint8_t create_ic_body[20];
    memset(create_ic_body, 0, sizeof(create_ic_body));
    uint16_t attr_bytes = 16;
    uint16_t val_len = 4;
    uint32_t style = XCB_IM_PreeditNothing | XCB_IM_StatusNothing;
    memcpy(&create_ic_body[0], &imid, 2);
    memcpy(&create_ic_body[2], &attr_bytes, 2);
    memcpy(&create_ic_body[4], &input_style_id, 2);
    memcpy(&create_ic_body[6], &val_len, 2);
    memcpy(&create_ic_body[8], &style, 4);
    memcpy(&create_ic_body[12], &client_win_id, 2);
    memcpy(&create_ic_body[14], &val_len, 2);
    memcpy(&create_ic_body[16], &comm_win, 4);
    send_native_property(conn, accept_win, protocol_atom, client_prop,
                         XCB_XIM_CREATE_IC, create_ic_body,
                         sizeof(create_ic_body));

    reply = read_native_reply(conn, comm_win, &opcode, &reply_len);
    TEST_CHECK(opcode == XCB_XIM_CREATE_IC_REPLY);
    uint16_t icid;
    memcpy(&icid, reply + 2, 2);
    free(reply);

    reply = read_native_reply(conn, comm_win, &opcode, &reply_len);
    TEST_CHECK(opcode == XCB_XIM_SET_EVENT_MASK);
    free(reply);

    // Valid XIM_SYNC from client -> XIM_SYNC_REPLY, pipelined with
    // XIM_QUERY_EXTENSION so an ignored XIM_SYNC fails immediately.
    uint8_t sync_body[4];
    memcpy(&sync_body[0], &imid, 2);
    memcpy(&sync_body[2], &icid, 2);
    send_native_inline(conn, accept_win, protocol_atom, XCB_XIM_SYNC, sync_body,
                       sizeof(sync_body));

    uint8_t query_ext_body[4] = {0, 0, 0, 0};
    memcpy(&query_ext_body[0], &imid, 2);
    send_native_inline(conn, accept_win, protocol_atom, XCB_XIM_QUERY_EXTENSION,
                       query_ext_body, sizeof(query_ext_body));

    reply = read_native_reply(conn, comm_win, &opcode, &reply_len);
    TEST_CHECK(opcode == XCB_XIM_SYNC_REPLY);
    TEST_CHECK(reply_len == 4);
    uint16_t sync_reply_imid, sync_reply_icid;
    memcpy(&sync_reply_imid, reply, 2);
    memcpy(&sync_reply_icid, reply + 2, 2);
    TEST_CHECK(sync_reply_imid == imid && sync_reply_icid == icid);
    free(reply);

    reply = read_native_reply(conn, comm_win, &opcode, &reply_len);
    TEST_CHECK(opcode == XCB_XIM_QUERY_EXTENSION_REPLY);
    free(reply);

    // Queue two FORWARD_EVENTs behind an in-flight synchronous FORWARD_EVENT:
    // - Event 1 (detail=99, seq=0x1111, flag=SYNCHRONOUS): enables
    //   use_sync_event and sets ic->sync = true.
    // - Event 2 (detail=41, seq=0x2222, flag=SYNCHRONOUS): queued; upon dequeue
    //   must preserve seq=0x2222 and emit XIM_SYNC_REPLY.
    // - Event 3 (detail=42, seq=0x3333, flag=0): queued; upon dequeue must
    //   preserve seq=0x3333 and NOT emit an unsolicited XIM_SYNC_REPLY.
    send_forward_key(conn, accept_win, protocol_atom, client_prop, imid, icid,
                     XCB_XIM_SYNCHRONOUS, 0x1111, 99, screen->root, comm_win);
    send_forward_key(conn, accept_win, protocol_atom, client_prop, imid, icid,
                     XCB_XIM_SYNCHRONOUS, 0x2222, 41, screen->root, comm_win);
    send_forward_key(conn, accept_win, protocol_atom, client_prop, imid, icid,
                     0, 0x3333, 42, screen->root, comm_win);

    uint16_t returned_seq = 0;
    reply = read_native_reply(conn, comm_win, &opcode, &reply_len);
    TEST_CHECK(opcode == XCB_XIM_FORWARD_EVENT);
    memcpy(&returned_seq, reply + 6, 2);
    TEST_CHECK(returned_seq == 0x1111);
    free(reply);
    reply = read_native_reply(conn, comm_win, &opcode, &reply_len);
    TEST_CHECK(opcode == XCB_XIM_SYNC_REPLY);
    free(reply);

    // Unblock Event 2 from ic->queue.
    send_native_inline(conn, accept_win, protocol_atom, XCB_XIM_SYNC_REPLY,
                       sync_body, sizeof(sync_body));
    reply = read_native_reply(conn, comm_win, &opcode, &reply_len);
    TEST_CHECK(opcode == XCB_XIM_FORWARD_EVENT);
    memcpy(&returned_seq, reply + 6, 2);
    TEST_CHECK(returned_seq == 0x2222);
    free(reply);
    reply = read_native_reply(conn, comm_win, &opcode, &reply_len);
    TEST_CHECK(opcode == XCB_XIM_SYNC_REPLY);
    free(reply);

    // Unblock Event 3 (flag == 0) from ic->queue.
    send_native_inline(conn, accept_win, protocol_atom, XCB_XIM_SYNC_REPLY,
                       sync_body, sizeof(sync_body));
    reply = read_native_reply(conn, comm_win, &opcode, &reply_len);
    TEST_CHECK(opcode == XCB_XIM_FORWARD_EVENT);
    memcpy(&returned_seq, reply + 6, 2);
    TEST_CHECK(returned_seq == 0x3333);
    free(reply);

    // Acknowledge Event 3's server->client forward_event.
    send_native_inline(conn, accept_win, protocol_atom, XCB_XIM_SYNC_REPLY,
                       sync_body, sizeof(sync_body));

    // Invalid ICID in XIM_SYNC -> XIM_ERROR (also proves Event 3 with flag == 0
    // did not leave a stray XIM_SYNC_REPLY in the stream).
    uint16_t bad_icid = icid + 99;
    memcpy(&sync_body[2], &bad_icid, 2);
    send_native_inline(conn, accept_win, protocol_atom, XCB_XIM_SYNC, sync_body,
                       sizeof(sync_body));

    reply = read_native_reply(conn, comm_win, &opcode, &reply_len);
    TEST_CHECK(opcode == XCB_XIM_ERROR);
    free(reply);

    xcb_disconnect(conn);
}

int main() {
    test_xim_server_t *server =
        test_xim_server_new("test_sync_server", server_callback, NULL);

    run_client();
    run_xim_sync_request_test(server->window);
    test_xim_server_destroy(server);
    return 0;
}
