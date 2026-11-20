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

    xcb_xim_close(ctx.im);
    xcb_xim_destroy(ctx.im);
    xcb_disconnect(conn);

    test_xim_server_destroy(empty_server);
    test_xim_server_destroy(prefix_server);
    test_xim_server_destroy(right_server);
    return 0;
}
