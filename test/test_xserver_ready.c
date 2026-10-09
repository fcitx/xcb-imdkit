/*
 * SPDX-FileCopyrightText: 2026 Weng Xuetian <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 *
 */

#include <stdlib.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>

int main() {
    int screen_number;
    xcb_connection_t *connection = xcb_connect(NULL, &screen_number);
    if (xcb_connection_has_error(connection)) {
        xcb_disconnect(connection);
        return EXIT_FAILURE;
    }

    xcb_screen_iterator_t screens =
        xcb_setup_roots_iterator(xcb_get_setup(connection));
    for (int i = 0; i < screen_number && screens.rem; i++) {
        xcb_screen_next(&screens);
    }

    xcb_disconnect(connection);
    return screens.rem ? EXIT_SUCCESS : EXIT_FAILURE;
}
