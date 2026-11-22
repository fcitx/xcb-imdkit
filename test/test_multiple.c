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

typedef union {
    xcb_client_message_event_t cm;
    xcb_generic_event_t generic;
} synthetic_client_message_t;

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
    synthetic_client_message_t bogus_proto;
    memset(&bogus_proto, 0, sizeof(bogus_proto));
    bogus_proto.cm.response_type = XCB_CLIENT_MESSAGE;
    bogus_proto.cm.format = 8;
    bogus_proto.cm.window = client->window;
    bogus_proto.cm.type = proto_reply->atom;
    free(proto_reply);
    TEST_CHECK(!xcb_xim_filter_event(client->im, &bogus_proto.generic));

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
    synthetic_client_message_t bogus_xconn;
    memset(&bogus_xconn, 0, sizeof(bogus_xconn));
    bogus_xconn.cm.response_type = XCB_CLIENT_MESSAGE;
    bogus_xconn.cm.format = 32;
    bogus_xconn.cm.window = shared[0].window;
    bogus_xconn.cm.type = xconn_atom;
    TEST_CHECK(!xcb_xim_filter_event(shared[0].im, &bogus_xconn.generic));

    xcb_generic_event_t *event;
    while (!(shared[0].completed && shared[1].completed) &&
           (event = xcb_wait_for_event(conn))) {
        if (!xcb_xim_filter_event(shared[0].im, event)) {
            xcb_xim_filter_event(shared[1].im, event);
        }
        free(event);
    }

    synthetic_client_message_t bogus_proto;
    memset(&bogus_proto, 0, sizeof(bogus_proto));
    bogus_proto.cm.response_type = XCB_CLIENT_MESSAGE;
    bogus_proto.cm.format = 8;
    bogus_proto.cm.window = shared[0].window;
    bogus_proto.cm.type = proto_atom;
    TEST_CHECK(!xcb_xim_filter_event(shared[0].im, &bogus_proto.generic));

    for (unsigned int i = 0; i < CLIENT_COUNT; i++) {
        TEST_CHECK(shared[i].completed);
        xcb_xim_close(shared[i].im);
        xcb_xim_destroy(shared[i].im);
    }
    xcb_disconnect(conn);
}

typedef struct {
    atomic_uint disconnect_count;
    atomic_uint close_count;
    uint16_t closed_ids[4];
} table_stats_t;

static void table_server_callback(xcb_im_t *im, xcb_im_client_t *client,
                                  xcb_im_input_context_t *ic,
                                  const xcb_im_packet_header_fr_t *hdr,
                                  void *frame, void *arg, void *user_data) {
    table_stats_t *stats = user_data;
    if (hdr->major_opcode == XCB_XIM_CLOSE) {
        xcb_im_close_fr_t *close_fr = frame;
        unsigned int idx = atomic_fetch_add(&stats->close_count, 1);
        if (idx < 4) {
            stats->closed_ids[idx] = close_fr->input_method_ID;
        }
    } else if (hdr->major_opcode == XCB_XIM_DISCONNECT) {
        atomic_fetch_add(&stats->disconnect_count, 1);
    }
}

static void simple_open_callback(xcb_xim_t *im, void *user_data) {
    bool *opened = user_data;
    *opened = true;
}

static xcb_window_t create_test_window(xcb_connection_t *conn,
                                       xcb_screen_t *screen) {
    xcb_window_t w = xcb_generate_id(conn);
    xcb_create_window(conn, XCB_COPY_FROM_PARENT, w, screen->root, 0, 0, 1, 1,
                      1, XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, 0,
                      NULL);
    return w;
}

static void send_xconnect(xcb_connection_t *cli_conn, xcb_window_t srv_win,
                          xcb_atom_t xconn_atom, xcb_window_t cli_win) {
    xcb_client_message_event_t ev = {
        .response_type = XCB_CLIENT_MESSAGE,
        .format = 32,
        .window = srv_win,
        .type = xconn_atom,
    };
    ev.data.data32[0] = cli_win;
    xcb_send_event(cli_conn, false, srv_win, XCB_EVENT_MASK_NO_EVENT,
                   (const char *)&ev);
    xcb_aux_sync(cli_conn);
}

static void pump_server_events(xcb_connection_t *srv_conn, xcb_im_t *srv_im) {
    xcb_aux_sync(srv_conn);
    xcb_generic_event_t *ev;
    while ((ev = xcb_poll_for_event(srv_conn)) != NULL) {
        xcb_im_filter_event(srv_im, ev);
        free(ev);
    }
}

static void test_server_client_tables(void) {
    table_stats_t stats = {0};
    test_xim_server_t *table_server = test_xim_server_new(
        "test_client_tables", table_server_callback, &stats);

    int cli_screen_num = 0;
    xcb_connection_t *cli_conn = xcb_connect(NULL, &cli_screen_num);
    xcb_screen_t *cli_screen = xcb_aux_get_screen(cli_conn, cli_screen_num);
    TEST_CHECK(cli_screen);

    // 1. Open two clients concurrently (IDs 1 and 2), close both so both go on
    // free_list, then open and close two more clients: both freed IDs (1 and 2)
    // must be reused without free_list losing its tail entry.
    xcb_xim_t *c1 =
        xcb_xim_create(cli_conn, cli_screen_num, "@im=test_client_tables");
    xcb_xim_t *c2 =
        xcb_xim_create(cli_conn, cli_screen_num, "@im=test_client_tables");
    bool o1 = false;
    bool o2 = false;
    TEST_CHECK(xcb_xim_open(c1, simple_open_callback, true, &o1));
    TEST_CHECK(xcb_xim_open(c2, simple_open_callback, true, &o2));
    xcb_generic_event_t *ev;
    while (!(o1 && o2) && (ev = xcb_wait_for_event(cli_conn))) {
        if (!xcb_xim_filter_event(c1, ev)) {
            xcb_xim_filter_event(c2, ev);
        }
        free(ev);
    }
    TEST_CHECK(o1 && o2);
    xcb_xim_close(c1);
    xcb_xim_close(c2);
    xcb_xim_destroy(c1);
    xcb_xim_destroy(c2);

    xcb_xim_t *c3 =
        xcb_xim_create(cli_conn, cli_screen_num, "@im=test_client_tables");
    xcb_xim_t *c4 =
        xcb_xim_create(cli_conn, cli_screen_num, "@im=test_client_tables");
    bool o3 = false;
    bool o4 = false;
    TEST_CHECK(xcb_xim_open(c3, simple_open_callback, true, &o3));
    TEST_CHECK(xcb_xim_open(c4, simple_open_callback, true, &o4));
    while (!(o3 && o4) && (ev = xcb_wait_for_event(cli_conn))) {
        if (!xcb_xim_filter_event(c3, ev)) {
            xcb_xim_filter_event(c4, ev);
        }
        free(ev);
    }
    TEST_CHECK(o3 && o4);
    xcb_xim_close(c3);
    xcb_xim_close(c4);
    xcb_xim_destroy(c3);
    xcb_xim_destroy(c4);
    test_xim_server_destroy(table_server);

    TEST_CHECK(atomic_load(&stats.close_count) == 4);
    TEST_CHECK(stats.closed_ids[0] == 1);
    TEST_CHECK(stats.closed_ids[1] == 2);
    TEST_CHECK(stats.closed_ids[2] == 2);
    TEST_CHECK(stats.closed_ids[3] == 1);

    // 2. Test DestroyNotify cleanup across multiple clients (destroying the
    // second client window first so _xcb_im_filter_destroy_window traverses
    // clients_by_win via hh2.next).
    int srv_screen_num = 0;
    xcb_connection_t *srv_conn = xcb_connect(NULL, &srv_screen_num);
    xcb_screen_t *srv_screen = xcb_aux_get_screen(srv_conn, srv_screen_num);
    TEST_CHECK(srv_screen);

    xcb_intern_atom_reply_t *xconn_reply = xcb_intern_atom_reply(
        cli_conn,
        xcb_intern_atom(cli_conn, false, strlen("_XIM_XCONNECT"),
                        "_XIM_XCONNECT"),
        NULL);
    TEST_CHECK(xconn_reply);
    xcb_atom_t xconn_atom = xconn_reply->atom;
    free(xconn_reply);

    table_stats_t destroy_stats = {0};
    xcb_window_t srv_win = create_test_window(srv_conn, srv_screen);
    uint32_t style = XCB_IM_PreeditNothing | XCB_IM_StatusNothing;
    xcb_im_styles_t styles = {1, &style};
    char *enc = "COMPOUND_TEXT";
    xcb_im_encodings_t encodings = {1, &enc};

    xcb_im_t *srv_im =
        xcb_im_create(srv_conn, srv_screen_num, srv_win, "test_destroy_notify",
                      XCB_IM_ALL_LOCALES, &styles, NULL, NULL, &encodings, 0,
                      table_server_callback, &destroy_stats);
    TEST_CHECK(srv_im);
    TEST_CHECK(xcb_im_open_im(srv_im));

    xcb_window_t w1 = create_test_window(cli_conn, cli_screen);
    xcb_window_t w2 = create_test_window(cli_conn, cli_screen);
    send_xconnect(cli_conn, srv_win, xconn_atom, w1);
    pump_server_events(srv_conn, srv_im);
    send_xconnect(cli_conn, srv_win, xconn_atom, w2);
    pump_server_events(srv_conn, srv_im);

    xcb_destroy_window(cli_conn, w2);
    xcb_aux_sync(cli_conn);
    pump_server_events(srv_conn, srv_im);
    TEST_CHECK(atomic_load(&destroy_stats.disconnect_count) == 1);

    xcb_destroy_window(cli_conn, w1);
    xcb_aux_sync(cli_conn);
    pump_server_events(srv_conn, srv_im);
    TEST_CHECK(atomic_load(&destroy_stats.disconnect_count) == 2);

    xcb_im_close_im(srv_im);
    xcb_im_destroy(srv_im);
    xcb_destroy_window(srv_conn, srv_win);
    xcb_disconnect(srv_conn);
    xcb_disconnect(cli_conn);
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
    test_server_client_tables();

    test_xim_server_destroy(server);
    TEST_CHECK(atomic_load(&server_forwarded_events) ==
               2 * CLIENT_COUNT * IC_COUNT);
    return 0;
}
