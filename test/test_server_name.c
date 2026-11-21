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

static bool wrong_server_hit;
static bool right_server_hit;
static bool completed;

static void wrong_server_callback(xcb_im_t *im, xcb_im_client_t *client,
                                  xcb_im_input_context_t *ic,
                                  const xcb_im_packet_header_fr_t *hdr,
                                  void *frame, void *arg, void *user_data) {
    if (hdr->major_opcode == XCB_XIM_OPEN) {
        wrong_server_hit = true;
    }
    if (hdr->major_opcode == XCB_XIM_FORWARD_EVENT) {
        xcb_im_forward_event(im, ic, arg);
    }
}

static void right_server_callback(xcb_im_t *im, xcb_im_client_t *client,
                                  xcb_im_input_context_t *ic,
                                  const xcb_im_packet_header_fr_t *hdr,
                                  void *frame, void *arg, void *user_data) {
    if (hdr->major_opcode == XCB_XIM_OPEN) {
        right_server_hit = true;
    }
    if (hdr->major_opcode == XCB_XIM_FORWARD_EVENT) {
        xcb_im_forward_event(im, ic, arg);
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

typedef struct {
    xcb_screen_t *screen;
    xcb_window_t window;
    xcb_xim_t *im;
} client_ctx_t;

static void forward_event(xcb_xim_t *im, xcb_xic_t ic,
                          xcb_key_press_event_t *event, void *user_data) {
    completed = true;
}

static void create_ic_callback(xcb_xim_t *im, xcb_xic_t ic, void *user_data) {
    client_ctx_t *ctx = user_data;
    TEST_CHECK(ic);
    xcb_key_press_event_t event = {
        .response_type = XCB_KEY_PRESS,
        .root = ctx->screen->root,
        .event = ctx->window,
        .detail = 38,
    };
    TEST_CHECK(xcb_xim_forward_event(ctx->im, ic, &event));
}

static void open_callback(xcb_xim_t *im, void *user_data) {
    client_ctx_t *ctx = user_data;
    uint32_t input_style = XCB_IM_PreeditNothing | XCB_IM_StatusNothing;
    TEST_CHECK(xcb_xim_create_ic(ctx->im, create_ic_callback, ctx,
                                 XCB_XIM_XNInputStyle, &input_style,
                                 XCB_XIM_XNClientWindow, &ctx->window,
                                 XCB_XIM_XNFocusWindow, &ctx->window, NULL));
}

static xcb_selection_notify_event_t *
wait_selection_notify(xcb_connection_t *conn) {
    xcb_generic_event_t *event;
    while ((event = xcb_wait_for_event(conn))) {
        if ((event->response_type & ~0x80) == XCB_SELECTION_NOTIFY) {
            return (xcb_selection_notify_event_t *)event;
        }
        free(event);
    }
    TEST_CHECK(false);
    return NULL;
}

static void test_selection_icccm(xcb_connection_t *conn, int screen_number,
                                 xcb_screen_t *screen) {
    xcb_atom_t server_atom = intern_atom(conn, "@server=fcitx5");
    xcb_atom_t transport_atom = intern_atom(conn, "TRANSPORT");
    xcb_atom_t locales_atom = intern_atom(conn, "LOCALES");
    xcb_atom_t targets_atom = intern_atom(conn, "TARGETS");
    xcb_atom_t custom_prop = intern_atom(conn, "_TEST_CUSTOM_SEL_PROP");

    xcb_window_t req_win = xcb_generate_id(conn);
    xcb_create_window(conn, XCB_COPY_FROM_PARENT, req_win, screen->root, 0, 0,
                      1, 1, 1, XCB_WINDOW_CLASS_INPUT_OUTPUT,
                      screen->root_visual, 0, NULL);

    // 1. Custom property != target must write converted value to custom_prop.
    xcb_convert_selection(conn, req_win, server_atom, transport_atom,
                          custom_prop, XCB_CURRENT_TIME);
    xcb_flush(conn);

    xcb_selection_notify_event_t *sn = wait_selection_notify(conn);
    TEST_CHECK(sn->target == transport_atom);
    TEST_CHECK(sn->property == custom_prop);
    free(sn);

    xcb_get_property_reply_t *prop_reply = xcb_get_property_reply(
        conn,
        xcb_get_property(conn, true, req_win, custom_prop, transport_atom, 0,
                         1024),
        NULL);
    TEST_CHECK(prop_reply);
    int len = xcb_get_property_value_length(prop_reply);
    const char *val = xcb_get_property_value(prop_reply);
    TEST_CHECK(len == (int)strlen("@transport=X/") &&
               memcmp(val, "@transport=X/", len) == 0);
    free(prop_reply);

    // 2. Obsolete client passing property == XCB_ATOM_NONE falls back to
    // target.
    xcb_convert_selection(conn, req_win, server_atom, transport_atom,
                          XCB_ATOM_NONE, XCB_CURRENT_TIME);
    xcb_flush(conn);

    sn = wait_selection_notify(conn);
    TEST_CHECK(sn->target == transport_atom);
    TEST_CHECK(sn->property == transport_atom);
    free(sn);

    prop_reply = xcb_get_property_reply(
        conn,
        xcb_get_property(conn, true, req_win, transport_atom, transport_atom, 0,
                         1024),
        NULL);
    TEST_CHECK(prop_reply);
    len = xcb_get_property_value_length(prop_reply);
    val = xcb_get_property_value(prop_reply);
    TEST_CHECK(len == (int)strlen("@transport=X/") &&
               memcmp(val, "@transport=X/", len) == 0);
    free(prop_reply);

    // 3. Unsupported target (TARGETS) must reply SelectionNotify with
    // property == XCB_ATOM_NONE (pipelined with TRANSPORT so an ignored request
    // fails immediately instead of hanging).
    xcb_convert_selection(conn, req_win, server_atom, targets_atom, custom_prop,
                          XCB_CURRENT_TIME);
    xcb_convert_selection(conn, req_win, server_atom, transport_atom,
                          custom_prop, XCB_CURRENT_TIME);
    xcb_flush(conn);

    sn = wait_selection_notify(conn);
    TEST_CHECK(sn->target == targets_atom);
    TEST_CHECK(sn->property == XCB_ATOM_NONE);
    free(sn);

    sn = wait_selection_notify(conn);
    TEST_CHECK(sn->target == transport_atom);
    TEST_CHECK(sn->property == custom_prop);
    free(sn);

    // 4. Long locale list exceeding legacy LOCALES_BUFSIZE must not be
    // truncated.
    int server_screen_num;
    xcb_connection_t *srv_conn = xcb_connect(NULL, &server_screen_num);
    xcb_screen_t *srv_screen = xcb_aux_get_screen(srv_conn, server_screen_num);
    TEST_CHECK(srv_screen);
    xcb_window_t srv_win = xcb_generate_id(srv_conn);
    xcb_create_window(srv_conn, XCB_COPY_FROM_PARENT, srv_win, srv_screen->root,
                      0, 0, 1, 1, 1, XCB_WINDOW_CLASS_INPUT_OUTPUT,
                      srv_screen->root_visual, 0, NULL);

    char long_locale[800];
    memset(long_locale, 'a', sizeof(long_locale) - 1);
    long_locale[sizeof(long_locale) - 1] = '\0';
    static uint32_t styles[] = {XCB_IM_PreeditNothing | XCB_IM_StatusNothing};
    static char *encodings[] = {"COMPOUND_TEXT"};
    xcb_im_styles_t input_styles = {.nStyles = 1, .styles = styles};
    xcb_im_encodings_t encoding_list = {.nEncodings = 1,
                                        .encodings = encodings};

    xcb_im_t *long_im = xcb_im_create(
        srv_conn, server_screen_num, srv_win, "long_locale_srv", long_locale,
        &input_styles, NULL, NULL, &encoding_list, 0, NULL, NULL);
    TEST_CHECK(long_im && xcb_im_open_im(long_im));

    xcb_atom_t long_srv_atom = intern_atom(conn, "@server=long_locale_srv");
    xcb_convert_selection(conn, req_win, long_srv_atom, locales_atom,
                          custom_prop, XCB_CURRENT_TIME);
    xcb_flush(conn);

    xcb_generic_event_t *sev;
    while ((sev = xcb_wait_for_event(srv_conn))) {
        bool handled = xcb_im_filter_event(long_im, sev);
        uint8_t type = sev->response_type & ~0x80;
        free(sev);
        if (handled && type == XCB_SELECTION_REQUEST) {
            break;
        }
    }

    sn = wait_selection_notify(conn);
    TEST_CHECK(sn->target == locales_atom);
    TEST_CHECK(sn->property == custom_prop);
    free(sn);

    prop_reply = xcb_get_property_reply(conn,
                                        xcb_get_property(conn, true, req_win,
                                                         custom_prop,
                                                         locales_atom, 0, 1024),
                                        NULL);
    TEST_CHECK(prop_reply);
    len = xcb_get_property_value_length(prop_reply);
    val = xcb_get_property_value(prop_reply);
    TEST_CHECK(len == (int)(strlen("@locale=") + strlen(long_locale)));
    TEST_CHECK(memcmp(val, "@locale=", strlen("@locale=")) == 0);
    TEST_CHECK(memcmp(val + strlen("@locale="), long_locale,
                      strlen(long_locale)) == 0);
    free(prop_reply);

    xcb_im_close_im(long_im);
    xcb_im_destroy(long_im);
    xcb_disconnect(srv_conn);
    xcb_destroy_window(conn, req_win);
}

static bool window_exists(xcb_connection_t *conn, xcb_window_t win) {
    xcb_get_window_attributes_reply_t *reply = xcb_get_window_attributes_reply(
        conn, xcb_get_window_attributes(conn, win), NULL);
    free(reply);
    return reply != NULL;
}

static void test_client_connect_window_cleanup(xcb_connection_t *conn,
                                               int screen_number,
                                               xcb_screen_t *screen) {
    int srv_screen_num = 0;
    xcb_connection_t *srv_conn = xcb_connect(NULL, &srv_screen_num);
    xcb_screen_t *srv_screen = xcb_aux_get_screen(srv_conn, srv_screen_num);
    TEST_CHECK(srv_screen);

    xcb_atom_t servers_atom = intern_atom(srv_conn, "XIM_SERVERS");
    xcb_atom_t transport_atom = intern_atom(srv_conn, "TRANSPORT");
    xcb_atom_t xconn_atom = intern_atom(srv_conn, "_XIM_XCONNECT");
    xcb_atom_t proto_atom = intern_atom(srv_conn, "_XIM_PROTOCOL");
    xcb_atom_t bad_trans_atom = intern_atom(srv_conn, "@server=bad_trans");
    xcb_atom_t bad_name_atom = intern_atom(srv_conn, "NOT_A_SERVER");
    xcb_atom_t bad_conn_atom = intern_atom(srv_conn, "@server=bad_conn");
    xcb_atom_t silent_atom = intern_atom(srv_conn, "@server=silent_srv");

    xcb_window_t fake_srv_win = xcb_generate_id(srv_conn);
    xcb_create_window(
        srv_conn, XCB_COPY_FROM_PARENT, fake_srv_win, srv_screen->root, 0, 0, 1,
        1, 1, XCB_WINDOW_CLASS_INPUT_OUTPUT, srv_screen->root_visual, 0, NULL);
    xcb_set_selection_owner(srv_conn, fake_srv_win, bad_trans_atom,
                            XCB_CURRENT_TIME);
    xcb_set_selection_owner(srv_conn, fake_srv_win, bad_name_atom,
                            XCB_CURRENT_TIME);
    xcb_set_selection_owner(srv_conn, fake_srv_win, bad_conn_atom,
                            XCB_CURRENT_TIME);
    xcb_set_selection_owner(srv_conn, fake_srv_win, silent_atom,
                            XCB_CURRENT_TIME);

    // 1. CHECK_NEXT_SERVER: index 0 (@server=bad_trans) creates
    // requestor_window then fails TRANSPORT_WAIT; index 1 (NOT_A_SERVER) fails
    // _xcb_xim_check_server_prepare before creating a window and must not
    // double-destroy the previous requestor_window (which causes BadWindow).
    xcb_atom_t list1[2] = {bad_trans_atom, bad_name_atom};
    xcb_change_property(srv_conn, XCB_PROP_MODE_REPLACE, srv_screen->root,
                        servers_atom, XCB_ATOM_ATOM, 32, 2, list1);
    xcb_aux_sync(srv_conn);

    unsetenv("XMODIFIERS");
    xcb_xim_t *xim1 = xcb_xim_create(conn, screen_number, NULL);
    TEST_CHECK(xcb_xim_open(xim1, NULL, false, NULL));

    // Fake server receives SelectionRequest for bad_trans and replies NONE.
    xcb_generic_event_t *srv_ev = xcb_wait_for_event(srv_conn);
    TEST_CHECK(srv_ev &&
               (srv_ev->response_type & ~0x80) == XCB_SELECTION_REQUEST);
    xcb_selection_request_event_t *sel_req =
        (xcb_selection_request_event_t *)srv_ev;
    xcb_selection_notify_event_t sel_notify = {
        .response_type = XCB_SELECTION_NOTIFY,
        .requestor = sel_req->requestor,
        .selection = sel_req->selection,
        .target = sel_req->target,
        .time = sel_req->time,
        .property = XCB_ATOM_NONE,
    };
    xcb_send_event(srv_conn, false, sel_req->requestor, XCB_EVENT_MASK_NO_EVENT,
                   (const char *)&sel_notify);
    xcb_flush(srv_conn);
    free(srv_ev);

    xcb_generic_event_t *cli_ev = xcb_wait_for_event(conn);
    TEST_CHECK(cli_ev &&
               (cli_ev->response_type & ~0x80) == XCB_SELECTION_NOTIFY);
    xcb_xim_filter_event(xim1, cli_ev);
    free(cli_ev);
    xcb_aux_sync(conn);
    while ((cli_ev = xcb_poll_for_event(conn)) != NULL) {
        // Must not receive any X11 error (response_type == 0, e.g. BadWindow).
        TEST_CHECK((cli_ev->response_type & ~0x80) != 0);
        free(cli_ev);
    }
    xcb_xim_close(xim1);
    xcb_xim_destroy(xim1);

    // 2. XIM_CONNECT_CONNECT failure must destroy im_client_window immediately.
    xcb_change_property(srv_conn, XCB_PROP_MODE_REPLACE, srv_screen->root,
                        servers_atom, XCB_ATOM_ATOM, 32, 1, &bad_conn_atom);
    xcb_aux_sync(srv_conn);

    xcb_xim_t *xim2 = xcb_xim_create(conn, screen_number, "@im=bad_conn");
    TEST_CHECK(xcb_xim_open(xim2, NULL, false, NULL));

    // Reply "@transport=X/" to TRANSPORT SelectionRequest.
    srv_ev = xcb_wait_for_event(srv_conn);
    TEST_CHECK(srv_ev &&
               (srv_ev->response_type & ~0x80) == XCB_SELECTION_REQUEST);
    sel_req = (xcb_selection_request_event_t *)srv_ev;
    const char trans_val[] = "@transport=X/";
    xcb_change_property(srv_conn, XCB_PROP_MODE_REPLACE, sel_req->requestor,
                        sel_req->property, transport_atom, 8,
                        sizeof(trans_val) - 1, trans_val);
    sel_notify.requestor = sel_req->requestor;
    sel_notify.selection = sel_req->selection;
    sel_notify.target = sel_req->target;
    sel_notify.time = sel_req->time;
    sel_notify.property = sel_req->property;
    xcb_send_event(srv_conn, false, sel_req->requestor, XCB_EVENT_MASK_NO_EVENT,
                   (const char *)&sel_notify);
    xcb_flush(srv_conn);
    free(srv_ev);

    cli_ev = xcb_wait_for_event(conn);
    TEST_CHECK(cli_ev);
    xcb_xim_filter_event(xim2, cli_ev);
    free(cli_ev);

    // Receive _XIM_XCONNECT on fake_srv_win, record im_client_window, reply.
    srv_ev = xcb_wait_for_event(srv_conn);
    TEST_CHECK(srv_ev && (srv_ev->response_type & ~0x80) == XCB_CLIENT_MESSAGE);
    xcb_client_message_event_t *cm = (xcb_client_message_event_t *)srv_ev;
    TEST_CHECK(cm->type == xconn_atom);
    xcb_window_t im_cli_win = cm->data.data32[0];
    TEST_CHECK(window_exists(srv_conn, im_cli_win));

    xcb_client_message_event_t xconn_reply = {
        .response_type = XCB_CLIENT_MESSAGE,
        .format = 32,
        .window = im_cli_win,
        .type = xconn_atom,
    };
    xconn_reply.data.data32[0] = fake_srv_win;
    xcb_send_event(srv_conn, false, im_cli_win, XCB_EVENT_MASK_NO_EVENT,
                   (const char *)&xconn_reply);
    xcb_flush(srv_conn);
    free(srv_ev);

    cli_ev = xcb_wait_for_event(conn);
    TEST_CHECK(cli_ev);
    xcb_xim_filter_event(xim2, cli_ev);
    free(cli_ev);

    // Receive XIM_CONNECT and send truncated 0-byte XIM_CONNECT_REPLY.
    srv_ev = xcb_wait_for_event(srv_conn);
    TEST_CHECK(srv_ev && (srv_ev->response_type & ~0x80) == XCB_CLIENT_MESSAGE);
    free(srv_ev);

    xcb_client_message_event_t bad_conn_reply = {
        .response_type = XCB_CLIENT_MESSAGE,
        .format = 8,
        .window = im_cli_win,
        .type = proto_atom,
    };
    bad_conn_reply.data.data8[0] = XCB_XIM_CONNECT_REPLY;
    xcb_send_event(srv_conn, false, im_cli_win, XCB_EVENT_MASK_NO_EVENT,
                   (const char *)&bad_conn_reply);
    xcb_flush(srv_conn);

    cli_ev = xcb_wait_for_event(conn);
    TEST_CHECK(cli_ev);
    xcb_xim_filter_event(xim2, cli_ev);
    free(cli_ev);
    xcb_aux_sync(conn);

    TEST_CHECK(!window_exists(srv_conn, im_cli_win));
    xcb_xim_close(xim2);
    xcb_xim_destroy(xim2);

    // 3. Closing xcb_xim_t while still waiting in XIM_CONNECT_CHECK_SERVER must
    // destroy requestor_window.
    xcb_change_property(srv_conn, XCB_PROP_MODE_REPLACE, srv_screen->root,
                        servers_atom, XCB_ATOM_ATOM, 32, 1, &silent_atom);
    xcb_aux_sync(srv_conn);

    xcb_xim_t *xim3 = xcb_xim_create(conn, screen_number, "@im=silent_srv");
    TEST_CHECK(xcb_xim_open(xim3, NULL, false, NULL));

    srv_ev = xcb_wait_for_event(srv_conn);
    TEST_CHECK(srv_ev &&
               (srv_ev->response_type & ~0x80) == XCB_SELECTION_REQUEST);
    xcb_window_t pending_req_win =
        ((xcb_selection_request_event_t *)srv_ev)->requestor;
    free(srv_ev);
    TEST_CHECK(window_exists(srv_conn, pending_req_win));

    xcb_xim_close(xim3);
    xcb_xim_destroy(xim3);
    xcb_aux_sync(conn);
    TEST_CHECK(!window_exists(srv_conn, pending_req_win));

    xcb_destroy_window(srv_conn, fake_srv_win);
    xcb_disconnect(srv_conn);
}

static xcb_window_t get_selection_owner(xcb_connection_t *conn,
                                        xcb_atom_t selection) {
    xcb_get_selection_owner_reply_t *reply = xcb_get_selection_owner_reply(
        conn, xcb_get_selection_owner(conn, selection), NULL);
    TEST_CHECK(reply);
    xcb_window_t owner = reply->owner;
    free(reply);
    return owner;
}

static bool server_atom_in_xim_servers(xcb_connection_t *conn,
                                       xcb_window_t root,
                                       xcb_atom_t servers_atom,
                                       xcb_atom_t target) {
    xcb_get_property_reply_t *reply =
        xcb_get_property_reply(conn,
                               xcb_get_property(conn, false, root, servers_atom,
                                                XCB_ATOM_ATOM, 0L, 1000L),
                               NULL);
    TEST_CHECK(reply);
    xcb_atom_t *atoms = xcb_get_property_value(reply);
    int len = xcb_get_property_value_length(reply) / (int)sizeof(xcb_atom_t);
    bool found = false;
    for (int i = 0; i < len; i++) {
        if (atoms[i] == target) {
            found = true;
            break;
        }
    }
    free(reply);
    return found;
}

static void test_server_close_im(xcb_connection_t *conn, int screen_number,
                                 xcb_screen_t *screen) {
    static uint32_t styles[] = {XCB_IM_PreeditNothing | XCB_IM_StatusNothing};
    static char *encodings[] = {"COMPOUND_TEXT"};
    xcb_im_styles_t input_styles = {.nStyles = 1, .styles = styles};
    xcb_im_encodings_t encoding_list = {.nEncodings = 1,
                                        .encodings = encodings};

    xcb_atom_t servers_atom = intern_atom(conn, "XIM_SERVERS");
    xcb_atom_t close_srv_atom = intern_atom(conn, "@server=close_test");

    xcb_window_t win1 = xcb_generate_id(conn);
    xcb_window_t win2 = xcb_generate_id(conn);
    xcb_create_window(conn, XCB_COPY_FROM_PARENT, win1, screen->root, 0, 0, 1,
                      1, 1, XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual,
                      0, NULL);
    xcb_create_window(conn, XCB_COPY_FROM_PARENT, win2, screen->root, 0, 0, 1,
                      1, 1, XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual,
                      0, NULL);

    xcb_im_t *srv1 = xcb_im_create(conn, screen_number, win1, "close_test",
                                   XCB_IM_ALL_LOCALES, &input_styles, NULL,
                                   NULL, &encoding_list, 0, NULL, NULL);
    xcb_im_t *srv2 = xcb_im_create(conn, screen_number, win2, "close_test",
                                   XCB_IM_ALL_LOCALES, &input_styles, NULL,
                                   NULL, &encoding_list, 0, NULL, NULL);
    TEST_CHECK(srv1 && srv2);

    // Calling xcb_im_close_im before xcb_im_open_im must be a safe no-op.
    xcb_im_close_im(srv2);

    TEST_CHECK(xcb_im_open_im(srv1));
    TEST_CHECK(get_selection_owner(conn, close_srv_atom) == win1);
    TEST_CHECK(server_atom_in_xim_servers(conn, screen->root, servers_atom,
                                          close_srv_atom));

    // Conflicting server fails xcb_im_open_im; calling xcb_im_close_im on it
    // (like fcitx5 ~XIMServer()) must NOT clobber srv1's XIM_SERVERS entry.
    TEST_CHECK(!xcb_im_open_im(srv2));
    xcb_im_close_im(srv2);
    TEST_CHECK(get_selection_owner(conn, close_srv_atom) == win1);
    TEST_CHECK(server_atom_in_xim_servers(conn, screen->root, servers_atom,
                                          close_srv_atom));

    // Closing srv1 while win1 is still alive must release selection ownership
    // and remove @server=close_test from XIM_SERVERS so srv2 can take over.
    xcb_im_close_im(srv1);
    TEST_CHECK(get_selection_owner(conn, close_srv_atom) == XCB_WINDOW_NONE);
    TEST_CHECK(!server_atom_in_xim_servers(conn, screen->root, servers_atom,
                                           close_srv_atom));

    TEST_CHECK(xcb_im_open_im(srv2));
    TEST_CHECK(get_selection_owner(conn, close_srv_atom) == win2);
    xcb_im_close_im(srv2);
    TEST_CHECK(get_selection_owner(conn, close_srv_atom) == XCB_WINDOW_NONE);

    xcb_im_destroy(srv2);
    xcb_im_destroy(srv1);
    xcb_destroy_window(conn, win2);
    xcb_destroy_window(conn, win1);
}

int main() {
    // xcb_im_open_im prepends each server atom to XIM_SERVERS, so starting
    // fcitx5 -> fcitx -> "" produces ["@server=", "@server=fcitx",
    // "@server=fcitx5"].
    test_xim_server_t *right_server =
        test_xim_server_new("fcitx5", right_server_callback, NULL);
    test_xim_server_t *prefix_server =
        test_xim_server_new("fcitx", wrong_server_callback, NULL);
    test_xim_server_t *empty_server =
        test_xim_server_new("", wrong_server_callback, NULL);

    int screen_number;
    xcb_connection_t *conn = xcb_connect(NULL, &screen_number);
    xcb_screen_t *screen = xcb_aux_get_screen(conn, screen_number);
    TEST_CHECK(screen);

    // Also prepend a 1-byte atom ("A") shorter than "@server=" (8 bytes) so
    // _xcb_xim_check_server_name must not read out-of-bounds.
    xcb_atom_t short_atom = intern_atom(conn, "A");
    xcb_atom_t servers_atom = intern_atom(conn, "XIM_SERVERS");
    xcb_change_property(conn, XCB_PROP_MODE_PREPEND, screen->root, servers_atom,
                        XCB_ATOM_ATOM, 32, 1, &short_atom);
    xcb_flush(conn);

    xcb_window_t window = xcb_generate_id(conn);
    xcb_create_window(conn, XCB_COPY_FROM_PARENT, window, screen->root, 0, 0, 1,
                      1, 1, XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual,
                      0, NULL);

    client_ctx_t ctx = {
        .screen = screen,
        .window = window,
    };
    xcb_xim_im_callback callbacks = {
        .forward_event = forward_event,
    };
    ctx.im = xcb_xim_create(conn, screen_number, "@im=fcitx5");
    xcb_xim_set_im_callback(ctx.im, &callbacks, &ctx);
    TEST_CHECK(xcb_xim_open(ctx.im, open_callback, false, &ctx));

    xcb_generic_event_t *event;
    while (!completed && (event = xcb_wait_for_event(conn))) {
        xcb_xim_filter_event(ctx.im, event);
        free(event);
    }

    TEST_CHECK(completed);
    TEST_CHECK(!wrong_server_hit);
    TEST_CHECK(right_server_hit);

    test_selection_icccm(conn, screen_number, screen);
    test_client_connect_window_cleanup(conn, screen_number, screen);
    test_server_close_im(conn, screen_number, screen);

    xcb_xim_close(ctx.im);
    xcb_xim_destroy(ctx.im);
    xcb_disconnect(conn);

    test_xim_server_destroy(empty_server);
    test_xim_server_destroy(prefix_server);
    test_xim_server_destroy(right_server);
    return 0;
}
