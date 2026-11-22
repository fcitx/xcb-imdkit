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
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xproto.h>

static bool opened;
static bool disconnected;
static atomic_int trigger_notify_count;
static int set_event_mask_count;
static bool ic2_destroyed;
static xcb_window_t client_win;
static xcb_window_t client_win2;
static xcb_window_t focus_win1;
static xcb_window_t focus_win2;
static xcb_xic_t created_ic;
static xcb_xic_t created_ic2;

static const uint32_t mask_client1 = XCB_EVENT_MASK_EXPOSURE;
static const uint32_t mask_client2 =
    XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_BUTTON_PRESS;
static const uint32_t mask_focus1 =
    XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_FOCUS_CHANGE;
static const uint32_t mask_focus2 =
    XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE;

static void server_callback(xcb_im_t *im, xcb_im_client_t *client,
                            xcb_im_input_context_t *ic,
                            const xcb_im_packet_header_fr_t *hdr, void *frame,
                            void *arg, void *user_data) {
    if (hdr->major_opcode == XCB_XIM_TRIGGER_NOTIFY) {
        xcb_im_trigger_notify_fr_t *tn = frame;
        int count = atomic_fetch_add(&trigger_notify_count, 1);
        if (count == 0) {
            // ic1 uses focus_win1 (not overwritten by ic2).
            TEST_CHECK(tn->input_context_ID == created_ic);
            TEST_CHECK(tn->flag == 0);
            TEST_CHECK(tn->client_select_event_mask == mask_focus1);
        } else if (count == 1) {
            // ic2 has no XNFocusWindow so must fall back to client_win2 (not
            // stale focus_win1 from ic1).
            TEST_CHECK(tn->input_context_ID == created_ic2);
            TEST_CHECK(tn->flag == 0);
            TEST_CHECK(tn->client_select_event_mask == mask_client2);
        } else if (count == 2) {
            // ic1 updated to focus_win2; failed SET_IC_VALUES on bogus IC and
            // destroying ic2 must not clobber ic1's focus_win2.
            TEST_CHECK(tn->input_context_ID == created_ic);
            TEST_CHECK(tn->flag == 1);
            TEST_CHECK(tn->client_select_event_mask == mask_focus2);
        }
    }
}

static void destroy_ic2_callback(xcb_xim_t *im, xcb_xic_t ic, void *user_data) {
    TEST_CHECK(ic == created_ic2);
    ic2_destroyed = true;
    TEST_CHECK(!xcb_xim_trigger_notify(im, 0x9999, 0, false));
    TEST_CHECK(!xcb_xim_trigger_notify(im, created_ic2, 0, false));
}

static void bogus_set_ic_callback(xcb_xim_t *im, xcb_xic_t ic,
                                  void *user_data) {
    TEST_CHECK(false);
}

static void set_ic_callback(xcb_xim_t *im, xcb_xic_t ic, void *user_data) {
    TEST_CHECK(ic == created_ic);
}

static void set_event_mask_callback(xcb_xim_t *im, xcb_xic_t ic,
                                    uint32_t forward_event_mask,
                                    uint32_t synchronous_event_mask,
                                    void *user_data) {
    set_event_mask_count++;
    if (set_event_mask_count == 2) {
        TEST_CHECK(xcb_xim_set_ic_values(im, created_ic, set_ic_callback, NULL,
                                         XCB_XIM_XNFocusWindow, &focus_win2,
                                         NULL));
        // Unknown IC must be rejected upfront without mutating any IC's window.
        TEST_CHECK(!xcb_xim_set_ic_values(im, 0x9999, bogus_set_ic_callback,
                                          NULL, XCB_XIM_XNFocusWindow,
                                          &client_win, NULL));
        TEST_CHECK(
            xcb_xim_destroy_ic(im, created_ic2, destroy_ic2_callback, NULL));
        // All IC-scoped requests must fail immediately once xcb_xim_destroy_ic
        // has been called on created_ic2.
        xcb_key_press_event_t dummy_key = {.response_type = XCB_KEY_PRESS};
        TEST_CHECK(!xcb_xim_destroy_ic(im, created_ic2, NULL, NULL));
        TEST_CHECK(!xcb_xim_trigger_notify(im, created_ic2, 0, false));
        TEST_CHECK(!xcb_xim_get_ic_values(im, created_ic2, NULL, NULL,
                                          XCB_XIM_XNFilterEvents, NULL));
        TEST_CHECK(!xcb_xim_set_ic_values(im, created_ic2, NULL, NULL,
                                          XCB_XIM_XNFocusWindow, &client_win,
                                          NULL));
        TEST_CHECK(!xcb_xim_set_ic_focus(im, created_ic2));
        TEST_CHECK(!xcb_xim_unset_ic_focus(im, created_ic2));
        TEST_CHECK(!xcb_xim_forward_event(im, created_ic2, &dummy_key));
        TEST_CHECK(!xcb_xim_reset_ic(im, created_ic2, NULL, NULL));
        TEST_CHECK(!xcb_xim_ext_move(im, created_ic2, 0, 0));
        // Pipelined right behind SET_IC_VALUES + DESTROY_IC (before replies
        // arrive): must wait in im->queue and read focus_win2's event mask.
        TEST_CHECK(xcb_xim_trigger_notify(im, created_ic, 0, true));
    } else if (set_event_mask_count == 3) {
        TEST_CHECK(ic2_destroyed);
        opened = true;
    }
}

static void create_ic2_callback(xcb_xim_t *im, xcb_xic_t ic, void *user_data) {
    TEST_CHECK(ic);
    TEST_CHECK(ic != created_ic);
    created_ic2 = ic;
    // Trigger on ic1 first, then ic2.
    TEST_CHECK(xcb_xim_trigger_notify(im, created_ic, 0, false));
    TEST_CHECK(xcb_xim_trigger_notify(im, created_ic2, 0, false));
}

static void create_ic_callback(xcb_xim_t *im, xcb_xic_t ic, void *user_data) {
    TEST_CHECK(ic);
    created_ic = ic;
    uint32_t input_style = XCB_IM_PreeditNothing | XCB_IM_StatusNothing;
    // Create ic2 with ONLY XNClientWindow (no XNFocusWindow).
    TEST_CHECK(xcb_xim_create_ic(im, create_ic2_callback, NULL,
                                 XCB_XIM_XNInputStyle, &input_style,
                                 XCB_XIM_XNClientWindow, &client_win2, NULL));
}

static void open_callback(xcb_xim_t *im, void *user_data) {
    uint32_t idx = UINT32_MAX;
    TEST_CHECK(
        xcb_xim_check_trigger_on_key(im, ' ', XCB_MOD_MASK_CONTROL, &idx));
    TEST_CHECK(idx == 0);
    TEST_CHECK(
        xcb_xim_check_trigger_on_key(im, ' ', XCB_MOD_MASK_CONTROL, NULL));
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    TEST_CHECK(xcb_xim_check_trigger_key(im, ' ', XCB_MOD_MASK_CONTROL, NULL) ==
               XCB_XIM_TRIGGER_ON_KEY);
#pragma GCC diagnostic pop

    idx = UINT32_MAX;
    TEST_CHECK(xcb_xim_check_trigger_off_key(im, 0xff1b, 0, &idx));
    TEST_CHECK(idx == 0);
    TEST_CHECK(xcb_xim_check_trigger_off_key(im, 0xff1b, 0, NULL));
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    TEST_CHECK(xcb_xim_check_trigger_key(im, 0xff1b, 0, NULL) ==
               XCB_XIM_TRIGGER_OFF_KEY);
#pragma GCC diagnostic pop

    uint32_t input_style = XCB_IM_PreeditNothing | XCB_IM_StatusNothing;
    TEST_CHECK(xcb_xim_create_ic(im, create_ic_callback, NULL,
                                 XCB_XIM_XNInputStyle, &input_style,
                                 XCB_XIM_XNClientWindow, &client_win,
                                 XCB_XIM_XNFocusWindow, &focus_win1, NULL));
}

static void disconnected_callback(xcb_xim_t *im, void *user_data) {
    disconnected = true;
}

int main() {
    xcb_im_ximtriggerkey_fr_t on_key = {
        .keysym = ' ',
        .modifier = XCB_MOD_MASK_CONTROL,
        .modifier_mask = XCB_MOD_MASK_CONTROL,
    };
    xcb_im_ximtriggerkey_fr_t off_key = {
        .keysym = 0xff1b,
        .modifier = 0,
        .modifier_mask = 0,
    };
    xcb_im_trigger_keys_t on_keys = {
        .nKeys = 1,
        .keys = &on_key,
    };
    xcb_im_trigger_keys_t off_keys = {
        .nKeys = 1,
        .keys = &off_key,
    };

    test_xim_server_t *server = test_xim_server_new_with_keys(
        "test_trigger_keys_server", &on_keys, &off_keys, server_callback, NULL);

    int screen_number;
    xcb_connection_t *client_connection = xcb_connect(NULL, &screen_number);
    TEST_CHECK(!xcb_connection_has_error(client_connection));
    xcb_screen_t *screen = xcb_aux_get_screen(client_connection, screen_number);
    TEST_CHECK(screen);

    client_win = xcb_generate_id(client_connection);
    xcb_create_window(client_connection, XCB_COPY_FROM_PARENT, client_win,
                      screen->root, 0, 0, 1, 1, 1,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual,
                      XCB_CW_EVENT_MASK, &mask_client1);

    client_win2 = xcb_generate_id(client_connection);
    xcb_create_window(client_connection, XCB_COPY_FROM_PARENT, client_win2,
                      screen->root, 0, 0, 1, 1, 1,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual,
                      XCB_CW_EVENT_MASK, &mask_client2);

    focus_win1 = xcb_generate_id(client_connection);
    xcb_create_window(client_connection, XCB_COPY_FROM_PARENT, focus_win1,
                      screen->root, 0, 0, 1, 1, 1,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual,
                      XCB_CW_EVENT_MASK, &mask_focus1);

    focus_win2 = xcb_generate_id(client_connection);
    xcb_create_window(client_connection, XCB_COPY_FROM_PARENT, focus_win2,
                      screen->root, 0, 0, 1, 1, 1,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual,
                      XCB_CW_EVENT_MASK, &mask_focus2);
    xcb_flush(client_connection);

    xcb_xim_im_callback callbacks = {
        .set_event_mask = set_event_mask_callback,
        .disconnected = disconnected_callback,
    };
    xcb_xim_t *client_im = xcb_xim_create(client_connection, screen_number,
                                          "@im=test_trigger_keys_server");
    xcb_xim_set_im_callback(client_im, &callbacks, NULL);
    // Before XIM_OPEN_DONE, IC-related requests must fail immediately.
    TEST_CHECK(!xcb_xim_trigger_notify(client_im, 1, 0, false));
    TEST_CHECK(!xcb_xim_set_ic_focus(client_im, 1));
    TEST_CHECK(!xcb_xim_unset_ic_focus(client_im, 1));
    TEST_CHECK(!xcb_xim_ext_move(client_im, 1, 0, 0));

    TEST_CHECK(xcb_xim_open(client_im, open_callback, true, NULL));
    TEST_CHECK(!xcb_xim_filter_event(client_im, NULL));

    xcb_generic_event_t *event;
    while (!opened && (event = xcb_wait_for_event(client_connection))) {
        xcb_xim_filter_event(client_im, event);
        free(event);
    }
    TEST_CHECK(opened);
    TEST_CHECK(atomic_load(&trigger_notify_count) == 3);

    test_xim_server_destroy(server);

    while (!disconnected && (event = xcb_wait_for_event(client_connection))) {
        xcb_xim_filter_event(client_im, event);
        free(event);
    }
    TEST_CHECK(disconnected);

    xcb_xim_close(client_im);
    xcb_xim_destroy(client_im);
    xcb_disconnect(client_connection);
    return 0;
}
