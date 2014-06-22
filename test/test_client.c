#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <stdlib.h>
#include <assert.h>
#include "imclient.h"

int main(int argc, char* argv[])
{
    /* Open the connection to the X server */

    int screen_default_nbr;
    xcb_connection_t *connection = xcb_connect (NULL, &screen_default_nbr);
    xcb_screen_t* screen = xcb_aux_get_screen(connection, screen_default_nbr);

    if (!screen) {
        return false;
    }
    xcb_xim_t* im = xcb_xim_create(connection, screen_default_nbr, "@im=test_server");
    assert(xcb_xim_open(im));

    xcb_generic_event_t *event;
    while ( (event = xcb_wait_for_event (connection)) ) {
        xcb_xim_filter_event(im, event);
        free(event);
    }

    xcb_disconnect(connection);

    return 0;
}
