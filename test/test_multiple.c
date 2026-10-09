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
    xcb_xim_close(client->im);
    xcb_xim_destroy(client->im);
    xcb_disconnect(client->connection);
    return thrd_success;
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
    test_xim_server_destroy(server);
    TEST_CHECK(atomic_load(&server_forwarded_events) ==
               CLIENT_COUNT * IC_COUNT);
    return 0;
}
