/*
 * SPDX-FileCopyrightText: 2026 Weng Xuetian <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 *
 */

#include "test_xim_server.h"
#include "imdkit.h"
#include "test.h"
#include <stdint.h>
#include <stdlib.h>
#include <threads.h>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xproto.h>

static xcb_atom_t test_xim_server_stop_atom(xcb_connection_t *connection) {
    static const char atom_name[] = "_XCB_IMDKIT_TEST_STOP";
    xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(
        connection,
        xcb_intern_atom(connection, false, sizeof(atom_name) - 1, atom_name),
        NULL);
    TEST_CHECK(reply);
    xcb_atom_t atom = reply->atom;
    free(reply);
    return atom;
}

static int test_xim_server_run(void *data) {
    test_xim_server_t *server = data;
    static uint32_t styles[] = {
        XCB_IM_PreeditNothing | XCB_IM_StatusNothing,
    };
    static char *encodings[] = {
        "COMPOUND_TEXT",
    };
    xcb_im_styles_t input_styles = {
        .nStyles = 1,
        .styles = styles,
    };
    xcb_im_encodings_t encoding_list = {
        .nEncodings = 1,
        .encodings = encodings,
    };

    int screen_number;
    xcb_connection_t *connection = xcb_connect(NULL, &screen_number);
    xcb_screen_t *screen = xcb_aux_get_screen(connection, screen_number);
    TEST_CHECK(screen);

    server->window = xcb_generate_id(connection);
    xcb_create_window(
        connection, XCB_COPY_FROM_PARENT, server->window, screen->root, 0, 0, 1,
        1, 1, XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, 0, NULL);
    server->stop_atom = test_xim_server_stop_atom(connection);
    xcb_im_t *im =
        xcb_im_create(connection, screen_number, server->window, server->name,
                      XCB_IM_ALL_LOCALES, &input_styles, NULL, NULL,
                      &encoding_list, 0, server->callback, server->user_data);
    TEST_CHECK(xcb_im_open_im(im));
    xcb_flush(connection);

    TEST_CHECK(mtx_lock(&server->mutex) == thrd_success);
    server->started = true;
    TEST_CHECK(cnd_signal(&server->ready) == thrd_success);
    TEST_CHECK(mtx_unlock(&server->mutex) == thrd_success);

    xcb_generic_event_t *event;
    while ((event = xcb_wait_for_event(connection))) {
        xcb_client_message_event_t *client_message =
            (xcb_client_message_event_t *)event;
        if ((event->response_type & ~0x80) == XCB_CLIENT_MESSAGE &&
            client_message->window == server->window &&
            client_message->type == server->stop_atom) {
            free(event);
            break;
        }
        xcb_im_filter_event(im, event);
        free(event);
    }

    xcb_im_close_im(im);
    xcb_im_destroy(im);
    xcb_disconnect(connection);
    return thrd_success;
}

test_xim_server_t *test_xim_server_new(const char *name,
                                       xcb_im_callback callback,
                                       void *user_data) {
    test_xim_server_t *server = calloc(1, sizeof(*server));
    TEST_CHECK(server);
    server->name = name;
    server->callback = callback;
    server->user_data = user_data;
    TEST_CHECK(mtx_init(&server->mutex, mtx_plain) == thrd_success);
    TEST_CHECK(cnd_init(&server->ready) == thrd_success);
    TEST_CHECK(thrd_create(&server->thread, test_xim_server_run, server) ==
               thrd_success);

    TEST_CHECK(mtx_lock(&server->mutex) == thrd_success);
    while (!server->started) {
        TEST_CHECK(cnd_wait(&server->ready, &server->mutex) == thrd_success);
    }
    TEST_CHECK(mtx_unlock(&server->mutex) == thrd_success);
    return server;
}

static void test_xim_server_stop(test_xim_server_t *server) {
    int screen_number;
    xcb_connection_t *connection = xcb_connect(NULL, &screen_number);
    TEST_CHECK(!xcb_connection_has_error(connection));

    xcb_client_message_event_t event = {
        .response_type = XCB_CLIENT_MESSAGE,
        .format = 32,
        .window = server->window,
        .type = server->stop_atom,
    };
    xcb_void_cookie_t cookie =
        xcb_send_event(connection, false, server->window,
                       XCB_EVENT_MASK_NO_EVENT, (const char *)&event);
    xcb_generic_error_t *error = xcb_request_check(connection, cookie);
    TEST_CHECK(!error);
    free(error);
    xcb_disconnect(connection);
}

void test_xim_server_destroy(test_xim_server_t *server) {
    test_xim_server_stop(server);
    TEST_CHECK(thrd_join(server->thread, NULL) == thrd_success);
    cnd_destroy(&server->ready);
    mtx_destroy(&server->mutex);
    free(server);
}
