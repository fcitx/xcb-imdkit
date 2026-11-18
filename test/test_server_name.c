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

    xcb_xim_close(ctx.im);
    xcb_xim_destroy(ctx.im);
    xcb_disconnect(conn);

    test_xim_server_destroy(empty_server);
    test_xim_server_destroy(prefix_server);
    test_xim_server_destroy(right_server);
    return 0;
}
