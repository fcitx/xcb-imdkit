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

typedef struct {
    xcb_connection_t *conn;
    xcb_screen_t *screen;
    xcb_atom_t xconnect_atom;
    xcb_atom_t protocol_atom;
    xcb_window_t client_win;
    xcb_window_t accept_win;
    uint16_t imid;
} raw_session_t;

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

static void send_inline_xim(const raw_session_t *s, uint8_t major_opcode,
                            const void *body, size_t body_len) {
    TEST_CHECK(body_len <= 16 && (body_len % 4) == 0);
    xcb_client_message_event_t ev;
    memset(&ev, 0, sizeof(ev));
    ev.response_type = XCB_CLIENT_MESSAGE;
    ev.format = 8;
    ev.window = s->accept_win;
    ev.type = s->protocol_atom;
    ev.data.data8[0] = major_opcode;
    uint16_t words = (uint16_t)(body_len / 4);
    memcpy(&ev.data.data8[2], &words, sizeof(words));
    if (body_len > 0) {
        memcpy(&ev.data.data8[4], body, body_len);
    }
    xcb_send_event(s->conn, false, s->accept_win, XCB_EVENT_MASK_NO_EVENT,
                   (const char *)&ev);
    xcb_flush(s->conn);
}

static void send_prop_cm(const raw_session_t *s, uint32_t length,
                         xcb_atom_t prop_atom) {
    xcb_client_message_event_t ev;
    memset(&ev, 0, sizeof(ev));
    ev.response_type = XCB_CLIENT_MESSAGE;
    ev.format = 32;
    ev.window = s->accept_win;
    ev.type = s->protocol_atom;
    ev.data.data32[0] = length;
    ev.data.data32[1] = prop_atom;
    xcb_send_event(s->conn, false, s->accept_win, XCB_EVENT_MASK_NO_EVENT,
                   (const char *)&ev);
    xcb_flush(s->conn);
}

static void raw_session_init(raw_session_t *s, xcb_connection_t *conn,
                             xcb_screen_t *screen, xcb_window_t server_win) {
    memset(s, 0, sizeof(*s));
    s->conn = conn;
    s->screen = screen;
    s->xconnect_atom = intern_atom(conn, "_XIM_XCONNECT");
    s->protocol_atom = intern_atom(conn, "_XIM_PROTOCOL");
    s->client_win = xcb_generate_id(conn);
    xcb_create_window(conn, XCB_COPY_FROM_PARENT, s->client_win, screen->root,
                      0, 0, 1, 1, 1, XCB_WINDOW_CLASS_INPUT_OUTPUT,
                      screen->root_visual, 0, NULL);

    xcb_client_message_event_t ev;
    memset(&ev, 0, sizeof(ev));
    ev.response_type = XCB_CLIENT_MESSAGE;
    ev.format = 32;
    ev.window = server_win;
    ev.type = s->xconnect_atom;
    ev.data.data32[0] = s->client_win;
    xcb_send_event(conn, false, server_win, XCB_EVENT_MASK_NO_EVENT,
                   (const char *)&ev);
    xcb_flush(conn);

    xcb_generic_event_t *event;
    while ((event = xcb_wait_for_event(conn))) {
        if ((event->response_type & ~0x80) == XCB_CLIENT_MESSAGE) {
            xcb_client_message_event_t *cm =
                (xcb_client_message_event_t *)event;
            if (cm->type == s->xconnect_atom) {
                s->accept_win = cm->data.data32[0];
                free(event);
                break;
            }
        }
        free(event);
    }
    TEST_CHECK(s->accept_win != XCB_WINDOW_NONE);

    uint16_t endian = 1;
    uint8_t connect_body[8] = {
        *(char *)&endian ? 'l' : 'B', 0, 0, 0, 0, 0, 0, 0};
    send_inline_xim(s, XCB_XIM_CONNECT, connect_body, sizeof(connect_body));

    while ((event = xcb_wait_for_event(conn))) {
        if ((event->response_type & ~0x80) == XCB_CLIENT_MESSAGE) {
            xcb_client_message_event_t *cm =
                (xcb_client_message_event_t *)event;
            if (cm->type == s->protocol_atom) {
                TEST_CHECK(cm->data.data8[0] == XCB_XIM_CONNECT_REPLY);
                free(event);
                return;
            }
        }
        free(event);
    }
    TEST_CHECK(false);
}

static void raw_session_cleanup(raw_session_t *s) {
    xcb_destroy_window(s->conn, s->client_win);
    xcb_flush(s->conn);
}

static void read_error_reply(const raw_session_t *s,
                             xcb_im_error_fr_t *out_err) {
    xcb_generic_event_t *event;
    while ((event = xcb_wait_for_event(s->conn))) {
        if ((event->response_type & ~0x80) == XCB_CLIENT_MESSAGE) {
            xcb_client_message_event_t *cm =
                (xcb_client_message_event_t *)event;
            if (cm->type == s->protocol_atom) {
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

static uint16_t raw_create_ic(const raw_session_t *s) {
    uint16_t body[2] = {s->imid, 0};
    send_inline_xim(s, XCB_XIM_CREATE_IC, body, sizeof(body));

    uint16_t valid_icid = 0;
    bool got_set_event_mask = false;
    xcb_generic_event_t *ev;
    while ((!valid_icid || !got_set_event_mask) &&
           (ev = xcb_wait_for_event(s->conn))) {
        if ((ev->response_type & ~0x80) == XCB_CLIENT_MESSAGE) {
            xcb_client_message_event_t *cm = (xcb_client_message_event_t *)ev;
            if (cm->type == s->protocol_atom) {
                if (cm->data.data8[0] == XCB_XIM_CREATE_IC_REPLY) {
                    memcpy(&valid_icid, &cm->data.data8[6], sizeof(valid_icid));
                } else if (cm->data.data8[0] == XCB_XIM_SET_EVENT_MASK) {
                    got_set_event_mask = true;
                }
            }
        }
        free(ev);
    }
    TEST_CHECK(valid_icid != 0);
    return valid_icid;
}

static void test_malformed_property_frames(xcb_connection_t *conn,
                                           xcb_screen_t *screen,
                                           xcb_window_t server_win) {
    raw_session_t s;
    raw_session_init(&s, conn, screen, server_win);
    xcb_atom_t prop_atom = intern_atom(conn, "_TEST_MALFORMED_PROP");

    // 1. Truncated length (< XCB_IM_HEADER_SIZE) and XCB_ATOM_NONE.
    send_prop_cm(&s, 2, prop_atom);
    send_prop_cm(&s, 16, XCB_ATOM_NONE);

    // 2. Property has only 5 bytes, but ClientMessage claims 20 bytes and the
    // XIM_OPEN header claims 4 * 4 = 16 body bytes (heap OOB read).
    uint8_t short_open[5] = {XCB_XIM_OPEN, 0, 0, 0, 12};
    uint16_t body_words = 4;
    memcpy(&short_open[2], &body_words, sizeof(body_words));
    xcb_change_property(conn, XCB_PROP_MODE_REPLACE, s.accept_win, prop_atom,
                        XCB_ATOM_STRING, 8, sizeof(short_open), short_open);
    send_prop_cm(&s, 20, prop_atom);

    // 3. Non-zero property offset + 32-bit length wraparound (8 + 0xFFFFFFFC).
    xcb_atom_t wrap_atom = intern_atom(conn, "_TEST_WRAP_PROP");
    uint8_t two_msgs[16] = {0};
    two_msgs[0] = XCB_XIM_QUERY_EXTENSION;
    uint16_t words1 = 1;
    memcpy(&two_msgs[2], &words1, sizeof(words1));
    two_msgs[8] = XCB_XIM_OPEN;
    memcpy(&two_msgs[10], &words1, sizeof(words1));
    xcb_change_property(conn, XCB_PROP_MODE_REPLACE, s.accept_win, wrap_atom,
                        XCB_ATOM_STRING, 8, sizeof(two_msgs), two_msgs);
    send_prop_cm(&s, 8, wrap_atom);
    send_prop_cm(&s, 0xFFFFFFFCu, wrap_atom);

    free(xcb_get_window_attributes_reply(
        conn, xcb_get_window_attributes(conn, server_win), NULL));
    raw_session_cleanup(&s);
}

static void test_malformed_open_error(raw_session_t *s) {
    // Malformed XIM_OPEN (string length 10 > 4-byte body) must elicit a
    // valid 12-byte XIM_ERROR frame with XIM_IMID_VALID (0x0001).
    uint8_t bad_open_body[4] = {10, 0, 0, 0};
    send_inline_xim(s, XCB_XIM_OPEN, bad_open_body, sizeof(bad_open_body));

    xcb_im_error_fr_t err_fr;
    read_error_reply(s, &err_fr);
    TEST_CHECK(err_fr.flag == 0x0001);
    TEST_CHECK(err_fr.input_method_ID != 0);
    TEST_CHECK(err_fr.Error_Code == 13);
    s->imid = err_fr.input_method_ID;
}

static void test_unknown_ic_error(const raw_session_t *s) {
    // IC-scoped requests with unknown icid must elicit a 12-byte XIM_ERROR
    // with both XIM_IMID_VALID | XIM_ICID_VALID (0x0003).
    uint16_t icid = 0x4321;
    uint16_t getic_body[4] = {s->imid, icid, 0, 0};
    send_inline_xim(s, XCB_XIM_GET_IC_VALUES, getic_body, sizeof(getic_body));

    xcb_im_error_fr_t err_fr;
    read_error_reply(s, &err_fr);
    TEST_CHECK(err_fr.flag == 0x0003);
    TEST_CHECK(err_fr.input_method_ID == s->imid);
    TEST_CHECK(err_fr.input_context_ID == icid);
    TEST_CHECK(err_fr.Error_Code == 13);

    uint16_t reset_body[2] = {s->imid, icid};
    send_inline_xim(s, XCB_XIM_RESET_IC, reset_body, sizeof(reset_body));

    read_error_reply(s, &err_fr);
    TEST_CHECK(err_fr.flag == 0x0003);
    TEST_CHECK(err_fr.input_method_ID == s->imid);
    TEST_CHECK(err_fr.input_context_ID == icid);
    TEST_CHECK(err_fr.Error_Code == 13);
}

static void test_invalid_trigger_notify_error(const raw_session_t *s) {
    uint8_t tn_body[16] = {0};
    uint16_t wrong_imid = s->imid + 1;
    uint16_t bad_icid = 0x4321;
    xcb_im_error_fr_t err_fr;

    // 1. Wrong input_method_ID -> IM-level XIM_ERROR (0x0001).
    memcpy(&tn_body[0], &wrong_imid, sizeof(wrong_imid));
    memcpy(&tn_body[2], &bad_icid, sizeof(bad_icid));
    send_inline_xim(s, XCB_XIM_TRIGGER_NOTIFY, tn_body, sizeof(tn_body));

    read_error_reply(s, &err_fr);
    TEST_CHECK(err_fr.flag == 0x0001);
    TEST_CHECK(err_fr.input_method_ID == s->imid);

    // 2. Unknown input_context_ID -> IC-level XIM_ERROR (0x0003).
    memcpy(&tn_body[0], &s->imid, sizeof(s->imid));
    send_inline_xim(s, XCB_XIM_TRIGGER_NOTIFY, tn_body, sizeof(tn_body));

    read_error_reply(s, &err_fr);
    TEST_CHECK(err_fr.flag == 0x0003);
    TEST_CHECK(err_fr.input_method_ID == s->imid);
    TEST_CHECK(err_fr.input_context_ID == bad_icid);

    // 3. Valid IC with invalid flag=2 -> IC-level XIM_ERROR (0x0003).
    uint16_t valid_icid = raw_create_ic(s);
    uint32_t bad_flag = 2;
    memcpy(&tn_body[0], &s->imid, sizeof(s->imid));
    memcpy(&tn_body[2], &valid_icid, sizeof(valid_icid));
    memcpy(&tn_body[4], &bad_flag, sizeof(bad_flag));
    send_inline_xim(s, XCB_XIM_TRIGGER_NOTIFY, tn_body, sizeof(tn_body));

    read_error_reply(s, &err_fr);
    TEST_CHECK(err_fr.flag == 0x0003);
    TEST_CHECK(err_fr.input_method_ID == s->imid);
    TEST_CHECK(err_fr.input_context_ID == valid_icid);
}

static void open_callback(xcb_xim_t *im, void *user_data) { opened = true; }

static void test_normal_client_after_errors(xcb_connection_t *conn,
                                            int screen_number) {
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
}

int main() {
    atomic_store(&allow_open, false);
    test_xim_server_t *server =
        test_xim_server_new("test_malformed_server", server_callback, NULL);

    int screen_number;
    xcb_connection_t *conn = xcb_connect(NULL, &screen_number);
    xcb_screen_t *screen = xcb_aux_get_screen(conn, screen_number);
    TEST_CHECK(screen);

    test_malformed_property_frames(conn, screen, server->window);

    // Opening a second raw session acts as a barrier guaranteeing the server
    // thread has drained all property messages before allow_open=true later.
    raw_session_t session;
    raw_session_init(&session, conn, screen, server->window);
    test_malformed_open_error(&session);
    test_unknown_ic_error(&session);
    test_invalid_trigger_notify_error(&session);
    raw_session_cleanup(&session);

    test_normal_client_after_errors(conn, screen_number);

    xcb_disconnect(conn);
    test_xim_server_destroy(server);
    return 0;
}
