#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <stdlib.h>
#include <assert.h>
#include "imdkit.h"

bool callback(xcb_im_t* im, xcb_im_client_t* client, xcb_im_input_context_t* ic,
              const xcb_im_proto_header_t* hdr,
              void* frame, void* arg, void* user_data)
{
    if (hdr->major_opcode != XIM_FORWARD_EVENT) {
        return false;
    }

    xcb_key_press_event_t* event = arg;

    xcb_im_forward_event(im, ic, event);

    return true;
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
    xcb_im_trigger_key_t key;
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
                                 NULL
                                );
    assert(xcb_im_open_im(im));

    printf("winid:%u\n", w);

    xcb_generic_event_t *event;
    while ( (event = xcb_wait_for_event (connection)) ) {
        xcb_im_filter_event(im, event);
        free(event);
    }

    return 0;
}
