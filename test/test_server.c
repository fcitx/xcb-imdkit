/*
 * (C) Copyright 2014 Weng Xuetian <wengxt@gmail.com>
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the GNU Lesser General Public License
 * (LGPL) version 2.1 which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/lgpl-2.1.html
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 */
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_keysyms.h>
#include <stdlib.h>
#include <assert.h>
#include "imdkit.h"

bool end = false;

void callback(xcb_im_t* im, xcb_im_client_t* client, xcb_im_input_context_t* ic,
              const xcb_im_packet_header_fr_t* hdr,
              void* frame, void* arg, void* user_data)
{
    if (hdr->major_opcode == XIM_DISCONNECT) {
        // end = true;
    }

    xcb_key_press_event_t* event = arg;

    xcb_key_symbols_t* key_symbols = user_data;
    xcb_keysym_t sym = xcb_key_symbols_get_keysym(key_symbols, event->detail, 0);
    if (sym == 't') {
        xcb_im_commit_string(im, ic, XimLookupChars, "hello world", strlen("hello world"), 0);
    } else {
        xcb_im_forward_event(im, ic, event);
    }
}

static uint32_t style_array[] = {
    XCB_IM_PreeditPosition | XCB_IM_StatusArea, //OverTheSpot
    XCB_IM_PreeditPosition | XCB_IM_StatusNothing,      //OverTheSpot
    XCB_IM_PreeditPosition | XCB_IM_StatusNone, //OverTheSpot
    XCB_IM_PreeditNothing | XCB_IM_StatusNothing,       //Root
    XCB_IM_PreeditNothing | XCB_IM_StatusNone,  //Root
};
static char* encoding_array[] = {
    "COMPOUND_TEXT",
};

static xcb_im_encodings_t encodings = {
    1, encoding_array
};

static xcb_im_styles_t styles = {
    5, style_array
};

int main(int argc, char* argv[])
{
    /* Open the connection to the X server */

    int screen_default_nbr;
    xcb_connection_t *connection = xcb_connect (NULL, &screen_default_nbr);
    xcb_screen_t* screen = xcb_aux_get_screen(connection, screen_default_nbr);
    xcb_key_symbols_t* key_symbols = xcb_key_symbols_alloc(connection);

    if (!screen) {
        return false;
    }
    xcb_window_t root = screen->root;

    xcb_window_t w = xcb_generate_id (connection);
    xcb_create_window (connection, XCB_COPY_FROM_PARENT, w, root,
                       0, 0, 1, 1, 1,
                       XCB_WINDOW_CLASS_INPUT_OUTPUT,
                       screen->root_visual,
                       0, NULL);
    xcb_im_trigger_keys_t keys;
    xcb_im_ximtriggerkey_fr_t key;
    key.keysym = ' ';
    key.modifier = 1 << 2;
    key.modifier_mask = 1 << 2;
    keys.nKeys = 1;
    keys.keys = &key;
    xcb_im_t* im = xcb_im_create(connection,
                                 screen_default_nbr,
                                 w,
                                 "test_server",
                                 XCB_IM_ALL_LOCALES,
                                 &styles,
                                 &keys,
                                 &keys,
                                 &encodings,
                                 0,
                                 callback,
                                 key_symbols
                                );
    assert(xcb_im_open_im(im));

    printf("winid:%u\n", w);

    xcb_generic_event_t *event;
    while ( (event = xcb_wait_for_event (connection)) ) {
        xcb_im_filter_event(im, event);
        free(event);
        if (end) {
            break;
        }
    }

    xcb_im_close_im(im);
    xcb_im_destroy(im);

    xcb_key_symbols_free(key_symbols);
    xcb_disconnect(connection);

    return 0;
}
