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
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xproto.h>

#define CLIENT_COUNT 2
#define IC_COUNT 2

typedef struct {
    unsigned int index;
    xcb_connection_t *connection;
    xcb_screen_t *screen;
    xcb_xim_t *im;
    xcb_window_t window;
    xcb_xic_t ics[IC_COUNT];
    unsigned int created_ics;
    unsigned int forwarded_events;
    bool completed;
} client_t;

static atomic_uint server_forwarded_events;

static void server_callback(xcb_im_t *im, xcb_im_client_t *client,
                            xcb_im_input_context_t *ic,
                            const xcb_im_packet_header_fr_t *hdr, void *frame,
                            void *arg, void *user_data) {
    if (hdr->major_opcode == XCB_XIM_FORWARD_EVENT) {
        atomic_fetch_add(&server_forwarded_events, 1);
        xcb_im_forward_event(im, ic, arg);
    }
}

static bool has_ic(const client_t *client, xcb_xic_t ic) {
    for (unsigned int i = 0; i < client->created_ics; i++) {
        if (client->ics[i] == ic) {
            return true;
        }
    }
    return false;
}

static void forward_event(xcb_xim_t *im, xcb_xic_t ic,
                          xcb_key_press_event_t *event, void *user_data) {
    client_t *client = user_data;
    TEST_CHECK(has_ic(client, ic));
    TEST_CHECK(event->response_type == XCB_KEY_PRESS);
    TEST_CHECK(event->detail == 38 + client->index);
    client->forwarded_events++;
    if (client->forwarded_events == IC_COUNT) {
        client->completed = true;
    }
}

static void create_ic_callback(xcb_xim_t *im, xcb_xic_t ic, void *user_data) {
    client_t *client = user_data;
    TEST_CHECK(ic);
    TEST_CHECK(client->created_ics < IC_COUNT);
    client->ics[client->created_ics++] = ic;

    xcb_key_press_event_t event = {
        .response_type = XCB_KEY_PRESS,
        .root = client->screen->root,
        .event = client->window,
        .detail = 38 + client->index,
    };
    TEST_CHECK(xcb_xim_forward_event(client->im, ic, &event));
}

static void open_callback(xcb_xim_t *im, void *user_data) {
    client_t *client = user_data;
    uint32_t input_style = XCB_IM_PreeditNothing | XCB_IM_StatusNothing;
    for (unsigned int i = 0; i < IC_COUNT; i++) {
        TEST_CHECK(xcb_xim_create_ic(
            client->im, create_ic_callback, client, XCB_XIM_XNInputStyle,
            &input_style, XCB_XIM_XNClientWindow, &client->window,
            XCB_XIM_XNFocusWindow, &client->window, NULL));
    }
}

static int client_thread(void *data) {
    client_t *client = data;
    int screen_number;
    client->connection = xcb_connect(NULL, &screen_number);
    client->screen = xcb_aux_get_screen(client->connection, screen_number);
    TEST_CHECK(client->screen);

    client->window = xcb_generate_id(client->connection);
    xcb_create_window(client->connection, XCB_COPY_FROM_PARENT, client->window,
                      client->screen->root, 0, 0, 1, 1, 1,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT,
                      client->screen->root_visual, 0, NULL);

    xcb_xim_im_callback callbacks = {
        .forward_event = forward_event,
    };
    client->im = xcb_xim_create(client->connection, screen_number,
                                "@im=test_multiple_server");
    xcb_xim_set_im_callback(client->im, &callbacks, client);
    TEST_CHECK(xcb_xim_open(client->im, open_callback, true, client));

    xcb_generic_event_t *event;
    while (!client->completed &&
           (event = xcb_wait_for_event(client->connection))) {
        xcb_xim_filter_event(client->im, event);
        free(event);
    }

    TEST_CHECK(client->completed);

    xcb_intern_atom_reply_t *proto_reply = xcb_intern_atom_reply(
        client->connection,
        xcb_intern_atom(client->connection, false, strlen("_XIM_PROTOCOL"),
                        "_XIM_PROTOCOL"),
        NULL);
    TEST_CHECK(proto_reply);
    xcb_client_message_event_t bogus_proto = {
        .response_type = XCB_CLIENT_MESSAGE,
        .format = 8,
        .window = client->window,
        .type = proto_reply->atom,
    };
    free(proto_reply);
    TEST_CHECK(
        !xcb_xim_filter_event(client->im, (xcb_generic_event_t *)&bogus_proto));

    xcb_xim_close(client->im);
    xcb_xim_destroy(client->im);
    xcb_disconnect(client->connection);
    return thrd_success;
}

static void run_shared_connection_clients(void) {
    int screen_number;
    xcb_connection_t *conn = xcb_connect(NULL, &screen_number);
    xcb_screen_t *screen = xcb_aux_get_screen(conn, screen_number);
    TEST_CHECK(screen);

    xcb_intern_atom_reply_t *proto_reply = xcb_intern_atom_reply(
        conn,
        xcb_intern_atom(conn, false, strlen("_XIM_PROTOCOL"), "_XIM_PROTOCOL"),
        NULL);
    xcb_intern_atom_reply_t *xconn_reply = xcb_intern_atom_reply(
        conn,
        xcb_intern_atom(conn, false, strlen("_XIM_XCONNECT"), "_XIM_XCONNECT"),
        NULL);
    TEST_CHECK(proto_reply && xconn_reply);
    xcb_atom_t proto_atom = proto_reply->atom;
    xcb_atom_t xconn_atom = xconn_reply->atom;
    free(proto_reply);
    free(xconn_reply);

    client_t shared[CLIENT_COUNT] = {0};
    xcb_xim_im_callback callbacks = {
        .forward_event = forward_event,
    };

    for (unsigned int i = 0; i < CLIENT_COUNT; i++) {
        shared[i].index = i;
        shared[i].connection = conn;
        shared[i].screen = screen;
        shared[i].window = xcb_generate_id(conn);
        xcb_create_window(
            conn, XCB_COPY_FROM_PARENT, shared[i].window, screen->root, 0, 0, 1,
            1, 1, XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, 0, NULL);
        shared[i].im =
            xcb_xim_create(conn, screen_number, "@im=test_multiple_server");
        xcb_xim_set_im_callback(shared[i].im, &callbacks, &shared[i]);
        TEST_CHECK(xcb_xim_open(shared[i].im, open_callback, true, &shared[i]));
    }

    // ClientMessages addressed to an unrelated window must not be consumed.
    xcb_client_message_event_t bogus_xconn = {
        .response_type = XCB_CLIENT_MESSAGE,
        .format = 32,
        .window = shared[0].window,
        .type = xconn_atom,
    };
    TEST_CHECK(!xcb_xim_filter_event(shared[0].im,
                                     (xcb_generic_event_t *)&bogus_xconn));

    xcb_generic_event_t *event;
    while (!(shared[0].completed && shared[1].completed) &&
           (event = xcb_wait_for_event(conn))) {
        if (!xcb_xim_filter_event(shared[0].im, event)) {
            xcb_xim_filter_event(shared[1].im, event);
        }
        free(event);
    }

    xcb_client_message_event_t bogus_proto = {
        .response_type = XCB_CLIENT_MESSAGE,
        .format = 8,
        .window = shared[0].window,
        .type = proto_atom,
    };
    TEST_CHECK(!xcb_xim_filter_event(shared[0].im,
                                     (xcb_generic_event_t *)&bogus_proto));

    for (unsigned int i = 0; i < CLIENT_COUNT; i++) {
        TEST_CHECK(shared[i].completed);
        xcb_xim_close(shared[i].im);
        xcb_xim_destroy(shared[i].im);
    }
    xcb_disconnect(conn);
}

int main() {
    thrd_t clients[CLIENT_COUNT];
    client_t client_data[CLIENT_COUNT] = {0};

    test_xim_server_t *server =
        test_xim_server_new("test_multiple_server", server_callback, NULL);

    for (unsigned int i = 0; i < CLIENT_COUNT; i++) {
        client_data[i].index = i;
        TEST_CHECK(thrd_create(&clients[i], client_thread, &client_data[i]) ==
                   thrd_success);
    }
    for (unsigned int i = 0; i < CLIENT_COUNT; i++) {
        TEST_CHECK(thrd_join(clients[i], NULL) == thrd_success);
    }

    run_shared_connection_clients();

    test_xim_server_destroy(server);
    TEST_CHECK(atomic_load(&server_forwarded_events) ==
               2 * CLIENT_COUNT * IC_COUNT);
    return 0;
}
