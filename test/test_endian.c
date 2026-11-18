/*
 * SPDX-FileCopyrightText: 2026 Weng Xuetian <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 *
 */

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

static uint16_t bswap16_val(uint16_t x) {
    return (uint16_t)((x << 8) | (x >> 8));
}

static uint32_t bswap32_val(uint32_t x) {
    return ((x << 24) & 0xFF000000u) | ((x << 8) & 0x00FF0000u) |
           ((x >> 8) & 0x0000FF00u) | ((x >> 24) & 0x000000FFu);
}

static bool server_saw_key;
static xcb_window_t expected_client_win;

static void server_callback(xcb_im_t *im, xcb_im_client_t *client,
                            xcb_im_input_context_t *ic,
                            const xcb_im_packet_header_fr_t *hdr, void *frame,
                            void *arg, void *user_data) {
    if (hdr->major_opcode == XCB_XIM_FORWARD_EVENT) {
        xcb_key_press_event_t *ev = arg;
        TEST_CHECK(ev->response_type == XCB_KEY_PRESS);
        TEST_CHECK(ev->detail == 38);
        TEST_CHECK(ev->sequence == 0x1234);
        TEST_CHECK(ev->time == 0x12345678u);
        TEST_CHECK(ev->event == expected_client_win);
        TEST_CHECK(ev->root_x == 123);
        TEST_CHECK(ev->state == XCB_MOD_MASK_CONTROL);
        server_saw_key = true;
        xcb_im_forward_event(im, ic, ev);
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

static void send_swapped_inline(xcb_connection_t *conn, xcb_window_t accept_win,
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
    uint16_t swapped_words = bswap16_val((uint16_t)(body_len / 4));
    memcpy(&ev.data.data8[2], &swapped_words, 2);
    if (body_len) {
        memcpy(&ev.data.data8[4], body, body_len);
    }
    xcb_send_event(conn, false, accept_win, XCB_EVENT_MASK_NO_EVENT,
                   (const char *)&ev);
    xcb_flush(conn);
}

static void send_swapped_property(xcb_connection_t *conn,
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
    uint16_t swapped_words = bswap16_val((uint16_t)(body_len / 4));
    memcpy(&buf[2], &swapped_words, 2);
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

static uint8_t *read_swapped_reply(xcb_connection_t *conn,
                                   xcb_window_t comm_win, uint8_t *opcode_out,
                                   size_t *body_len_out) {
    xcb_generic_event_t *gev = wait_client_message(conn);
    xcb_client_message_event_t *ev = (xcb_client_message_event_t *)gev;
    uint8_t *body = NULL;
    if (ev->format == 8) {
        *opcode_out = ev->data.data8[0];
        uint16_t raw_words;
        memcpy(&raw_words, &ev->data.data8[2], 2);
        size_t len = (size_t)bswap16_val(raw_words) * 4;
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
        uint16_t raw_words;
        memcpy(&raw_words, &val[2], 2);
        size_t len = (size_t)bswap16_val(raw_words) * 4;
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

static xcb_window_t xconnect(xcb_connection_t *conn, xcb_window_t server_win,
                             xcb_window_t comm_win, xcb_atom_t xconnect_atom) {
    xcb_client_message_event_t ev;
    memset(&ev, 0, sizeof(ev));
    ev.response_type = XCB_CLIENT_MESSAGE;
    ev.format = 32;
    ev.window = server_win;
    ev.type = xconnect_atom;
    ev.data.data32[0] = comm_win;
    xcb_send_event(conn, false, server_win, XCB_EVENT_MASK_NO_EVENT,
                   (const char *)&ev);
    xcb_flush(conn);

    xcb_generic_event_t *gev = wait_client_message(conn);
    xcb_client_message_event_t *reply = (xcb_client_message_event_t *)gev;
    TEST_CHECK(reply->type == xconnect_atom);
    xcb_window_t accept_win = reply->data.data32[0];
    free(gev);
    return accept_win;
}

int main() {
    test_xim_server_t *server =
        test_xim_server_new("test_endian_server", server_callback, NULL);

    int screen_num;
    xcb_connection_t *conn = xcb_connect(NULL, &screen_num);
    xcb_screen_t *screen = xcb_aux_get_screen(conn, screen_num);
    TEST_CHECK(screen);

    uint16_t endian = 1;
    uint8_t opposite_order = (*(char *)&endian) ? 'B' : 'l';

    xcb_atom_t server_atom = intern_atom(conn, "@server=test_endian_server");
    xcb_atom_t xconnect_atom = intern_atom(conn, "_XIM_XCONNECT");
    xcb_atom_t protocol_atom = intern_atom(conn, "_XIM_PROTOCOL");
    xcb_atom_t client_prop = intern_atom(conn, "_CLIENT_ENDIAN_PROP");

    xcb_get_selection_owner_reply_t *owner_reply =
        xcb_get_selection_owner_reply(
            conn, xcb_get_selection_owner(conn, server_atom), NULL);
    TEST_CHECK(owner_reply && owner_reply->owner != XCB_NONE);
    xcb_window_t server_win = owner_reply->owner;
    free(owner_reply);

    // 1. Invalid byte_order ('X') in XIM_CONNECT must be ignored cleanly.
    xcb_window_t bad_comm_win = xcb_generate_id(conn);
    xcb_create_window(conn, XCB_COPY_FROM_PARENT, bad_comm_win, screen->root, 0,
                      0, 1, 1, 1, XCB_WINDOW_CLASS_INPUT_OUTPUT,
                      screen->root_visual, 0, NULL);
    xcb_window_t bad_accept_win =
        xconnect(conn, server_win, bad_comm_win, xconnect_atom);
    uint8_t bad_connect[8] = {'X', 0, 0, 0, 0, 0, 0, 0};
    send_swapped_inline(conn, bad_accept_win, protocol_atom, XCB_XIM_CONNECT,
                        bad_connect, sizeof(bad_connect));

    // 2. Opposite-endian client sending XIM_CONNECT via format=32 property.
    xcb_window_t comm_win = xcb_generate_id(conn);
    xcb_create_window(conn, XCB_COPY_FROM_PARENT, comm_win, screen->root, 0, 0,
                      1, 1, 1, XCB_WINDOW_CLASS_INPUT_OUTPUT,
                      screen->root_visual, 0, NULL);
    expected_client_win = comm_win;
    xcb_window_t accept_win =
        xconnect(conn, server_win, comm_win, xconnect_atom);

    uint8_t connect_body[8] = {opposite_order, 0, 0, 1, 0, 0, 0, 0};
    send_swapped_property(conn, accept_win, protocol_atom, client_prop,
                          XCB_XIM_CONNECT, connect_body, sizeof(connect_body));

    uint8_t opcode = 0;
    size_t reply_len = 0;
    uint8_t *reply = read_swapped_reply(conn, comm_win, &opcode, &reply_len);
    TEST_CHECK(opcode == XCB_XIM_CONNECT_REPLY);
    TEST_CHECK(reply_len == 4);
    uint16_t major_ver;
    memcpy(&major_ver, reply, 2);
    TEST_CHECK(bswap16_val(major_ver) == 1);
    free(reply);

    // 3. Opposite-endian inline format=8 XIM_OPEN ("C" locale: 1 byte + 3 pad),
    // pipelined with a format=32 XIM_QUERY_EXTENSION so that if XIM_OPEN is
    // dropped due to unswapped format=8 hdr->length, read_swapped_reply fails
    // immediately instead of hanging.
    uint8_t open_body[4] = {1, 'C', 0, 0};
    send_swapped_inline(conn, accept_win, protocol_atom, XCB_XIM_OPEN,
                        open_body, sizeof(open_body));
    uint8_t query_ext_body[4] = {0, 0, 0, 0};
    send_swapped_property(conn, accept_win, protocol_atom, client_prop,
                          XCB_XIM_QUERY_EXTENSION, query_ext_body,
                          sizeof(query_ext_body));

    reply = read_swapped_reply(conn, comm_win, &opcode, &reply_len);
    TEST_CHECK(opcode == XCB_XIM_OPEN_REPLY);
    TEST_CHECK(reply_len >= 8);
    uint16_t swapped_imid;
    memcpy(&swapped_imid, reply, 2);
    // Find "inputStyle" and "clientWindow" attribute IDs from XIM_OPEN_REPLY.
    uint16_t im_attr_bytes_raw;
    memcpy(&im_attr_bytes_raw, reply + 2, 2);
    size_t ic_list_off = 4 + bswap16_val(im_attr_bytes_raw);
    uint16_t ic_attr_bytes_raw;
    memcpy(&ic_attr_bytes_raw, reply + ic_list_off, 2);
    size_t ic_bytes = bswap16_val(ic_attr_bytes_raw);
    size_t pos = ic_list_off + 4;
    uint16_t input_style_id = UINT16_MAX;
    uint16_t client_win_id = UINT16_MAX;
    while (pos < ic_list_off + 4 + ic_bytes) {
        uint16_t attr_id_raw, name_len_raw;
        memcpy(&attr_id_raw, reply + pos, 2);
        memcpy(&name_len_raw, reply + pos + 4, 2);
        uint16_t name_len = bswap16_val(name_len_raw);
        const char *name = (const char *)(reply + pos + 6);
        if (name_len == strlen(XCB_XIM_XNInputStyle) &&
            memcmp(name, XCB_XIM_XNInputStyle, name_len) == 0) {
            input_style_id = attr_id_raw; // keep swapped wire representation
        } else if (name_len == strlen(XCB_XIM_XNClientWindow) &&
                   memcmp(name, XCB_XIM_XNClientWindow, name_len) == 0) {
            client_win_id = attr_id_raw; // keep swapped wire representation
        }
        size_t item_len = 6 + name_len;
        item_len = (item_len + 3) & ~3u;
        pos += item_len;
    }
    free(reply);
    TEST_CHECK(input_style_id != UINT16_MAX && client_win_id != UINT16_MAX);

    // Server also sends XIM_SET_EVENT_MASK after XIM_OPEN_REPLY when no trigger
    // keys are configured; consume it and the pipelined QUERY_EXTENSION_REPLY.
    reply = read_swapped_reply(conn, comm_win, &opcode, &reply_len);
    TEST_CHECK(opcode == XCB_XIM_SET_EVENT_MASK);
    free(reply);
    reply = read_swapped_reply(conn, comm_win, &opcode, &reply_len);
    TEST_CHECK(opcode == XCB_XIM_QUERY_EXTENSION_REPLY);
    free(reply);

    // 4. Opposite-endian XIM_CREATE_IC.
    uint8_t create_ic_body[20];
    memset(create_ic_body, 0, sizeof(create_ic_body));
    memcpy(&create_ic_body[0], &swapped_imid, 2);
    uint16_t attr_bytes_swapped = bswap16_val(16);
    memcpy(&create_ic_body[2], &attr_bytes_swapped, 2);
    uint16_t val_len_swapped = bswap16_val(4);
    uint32_t style_swapped =
        bswap32_val(XCB_IM_PreeditNothing | XCB_IM_StatusNothing);
    memcpy(&create_ic_body[4], &input_style_id, 2);
    memcpy(&create_ic_body[6], &val_len_swapped, 2);
    memcpy(&create_ic_body[8], &style_swapped, 4);
    uint32_t win_swapped = bswap32_val(comm_win);
    memcpy(&create_ic_body[12], &client_win_id, 2);
    memcpy(&create_ic_body[14], &val_len_swapped, 2);
    memcpy(&create_ic_body[16], &win_swapped, 4);
    send_swapped_property(conn, accept_win, protocol_atom, client_prop,
                          XCB_XIM_CREATE_IC, create_ic_body,
                          sizeof(create_ic_body));

    reply = read_swapped_reply(conn, comm_win, &opcode, &reply_len);
    TEST_CHECK(opcode == XCB_XIM_CREATE_IC_REPLY);
    TEST_CHECK(reply_len == 4);
    uint16_t swapped_icid;
    memcpy(&swapped_icid, reply + 2, 2);
    free(reply);

    // Consume XIM_SET_EVENT_MASK sent on create_ic.
    reply = read_swapped_reply(conn, comm_win, &opcode, &reply_len);
    TEST_CHECK(opcode == XCB_XIM_SET_EVENT_MASK);
    free(reply);

    // 5. Opposite-endian XIM_FORWARD_EVENT round-trip: verify server swaps
    // incoming xcb_key_press_event_t to host endian and swaps outgoing event
    // back to client endian.
    uint8_t fwd_body[8 + sizeof(xcb_key_press_event_t)];
    memset(fwd_body, 0, sizeof(fwd_body));
    memcpy(&fwd_body[0], &swapped_imid, 2);
    memcpy(&fwd_body[2], &swapped_icid, 2);
    uint16_t flag_swapped = 0;
    uint16_t seq_swapped = bswap16_val(0x4321);
    memcpy(&fwd_body[4], &flag_swapped, 2);
    memcpy(&fwd_body[6], &seq_swapped, 2);

    xcb_key_press_event_t wire_key;
    memset(&wire_key, 0, sizeof(wire_key));
    wire_key.response_type = XCB_KEY_PRESS;
    wire_key.detail = 38;
    wire_key.sequence = bswap16_val(0x1234);
    wire_key.time = bswap32_val(0x12345678u);
    wire_key.root = bswap32_val(screen->root);
    wire_key.event = bswap32_val(comm_win);
    wire_key.root_x = (int16_t)bswap16_val(123);
    wire_key.state = bswap16_val(XCB_MOD_MASK_CONTROL);
    memcpy(&fwd_body[8], &wire_key, sizeof(wire_key));

    send_swapped_property(conn, accept_win, protocol_atom, client_prop,
                          XCB_XIM_FORWARD_EVENT, fwd_body, sizeof(fwd_body));

    reply = read_swapped_reply(conn, comm_win, &opcode, &reply_len);
    TEST_CHECK(opcode == XCB_XIM_FORWARD_EVENT);
    TEST_CHECK(reply_len == 8 + sizeof(xcb_key_press_event_t));
    xcb_key_press_event_t returned_key;
    memcpy(&returned_key, reply + 8, sizeof(returned_key));
    TEST_CHECK(returned_key.response_type == XCB_KEY_PRESS);
    TEST_CHECK(returned_key.detail == 38);
    TEST_CHECK(bswap16_val(returned_key.sequence) == 0x1234);
    TEST_CHECK(bswap32_val(returned_key.time) == 0x12345678u);
    TEST_CHECK(bswap32_val(returned_key.event) == comm_win);
    TEST_CHECK(bswap16_val((uint16_t)returned_key.root_x) == 123);
    TEST_CHECK(bswap16_val(returned_key.state) == XCB_MOD_MASK_CONTROL);
    free(reply);

    TEST_CHECK(server_saw_key);

    // 6. Host-endian client sending XIM_CONNECT via format=32 property must
    // detect local byte_order instead of leaving byte_order == '?' (swap=true).
    uint8_t local_order = (*(char *)&endian) ? 'l' : 'B';
    xcb_window_t local_comm_win = xcb_generate_id(conn);
    xcb_create_window(conn, XCB_COPY_FROM_PARENT, local_comm_win, screen->root,
                      0, 0, 1, 1, 1, XCB_WINDOW_CLASS_INPUT_OUTPUT,
                      screen->root_visual, 0, NULL);
    xcb_window_t local_accept_win =
        xconnect(conn, server_win, local_comm_win, xconnect_atom);
    uint8_t local_connect_hdr_and_body[12] = {
        XCB_XIM_CONNECT, 0, 0, 0, local_order, 0, 1, 0, 0, 0, 0, 0};
    uint16_t local_words = 2;
    memcpy(&local_connect_hdr_and_body[2], &local_words, 2);
    uint16_t local_major = 1;
    memcpy(&local_connect_hdr_and_body[6], &local_major, 2);
    xcb_change_property(conn, XCB_PROP_MODE_APPEND, local_accept_win,
                        client_prop, XCB_ATOM_STRING, 8,
                        sizeof(local_connect_hdr_and_body),
                        local_connect_hdr_and_body);
    xcb_client_message_event_t cm32;
    memset(&cm32, 0, sizeof(cm32));
    cm32.response_type = XCB_CLIENT_MESSAGE;
    cm32.format = 32;
    cm32.window = local_accept_win;
    cm32.type = protocol_atom;
    cm32.data.data32[0] = sizeof(local_connect_hdr_and_body);
    cm32.data.data32[1] = client_prop;
    xcb_send_event(conn, false, local_accept_win, XCB_EVENT_MASK_NO_EVENT,
                   (const char *)&cm32);
    xcb_flush(conn);

    xcb_generic_event_t *local_gev = wait_client_message(conn);
    xcb_client_message_event_t *local_reply =
        (xcb_client_message_event_t *)local_gev;
    TEST_CHECK(local_reply->format == 8);
    TEST_CHECK(local_reply->data.data8[0] == XCB_XIM_CONNECT_REPLY);
    uint16_t local_reply_words;
    memcpy(&local_reply_words, &local_reply->data.data8[2], 2);
    TEST_CHECK(local_reply_words == 1);
    free(local_gev);

    xcb_disconnect(conn);
    test_xim_server_destroy(server);
    return 0;
}
