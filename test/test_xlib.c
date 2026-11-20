/*
 * SPDX-FileCopyrightText: 2026 Weng Xuetian <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 *
 */

#include "encoding.h"
#include "imdkit.h"
#include "test.h"
#include "ximproto.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <locale.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xproto.h>

#define COMMIT_TEXT "你好Xlib"
#define RESET_TEXT "重置提交"

typedef struct {
    xcb_connection_t *conn;
    int screen_number;
    const char *name;
    bool use_sync_event;
    atomic_bool corrupt_next_close;
    xcb_window_t window;
    xcb_atom_t stop_atom;
    xcb_atom_t proto_atom;
    mtx_t mutex;
    cnd_t ready;
    bool started;
    thrd_t thread;
} xlib_test_server_t;

static xcb_atom_t intern_xcb_atom(xcb_connection_t *conn, const char *name) {
    xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(
        conn, xcb_intern_atom(conn, false, strlen(name), name), NULL);
    TEST_CHECK(reply);
    xcb_atom_t atom = reply->atom;
    free(reply);
    return atom;
}

static void server_callback(xcb_im_t *im, xcb_im_client_t *client,
                            xcb_im_input_context_t *ic,
                            const xcb_im_packet_header_fr_t *hdr, void *frame,
                            void *arg, void *user_data) {
    if (hdr->major_opcode == XCB_XIM_FORWARD_EVENT) {
        xcb_key_press_event_t *ev = arg;
        bool is_press = (ev->response_type & ~0x80) == XCB_KEY_PRESS;
        if (ev->detail == 38) {
            if (is_press) {
                size_t len = 0;
                char *ctext = xcb_utf8_to_compound_text(
                    COMMIT_TEXT, strlen(COMMIT_TEXT), &len);
                TEST_CHECK(ctext);
                xcb_im_commit_string(im, ic, XCB_XIM_LOOKUP_CHARS, ctext, len,
                                     0);
                free(ctext);
            }
        } else {
            xcb_im_forward_event(im, ic, ev);
        }
    } else if (hdr->major_opcode == XCB_XIM_RESET_IC) {
        xcb_im_reset_ic_reply_fr_t *reply = arg;
        size_t len = 0;
        char *ctext =
            xcb_utf8_to_compound_text(RESET_TEXT, strlen(RESET_TEXT), &len);
        TEST_CHECK(ctext);
        reply->committed_string = (uint8_t *)ctext;
        reply->byte_length_of_committed_string = len;
    }
}

static int server_thread_main(void *data) {
    xlib_test_server_t *srv = data;
    static uint32_t styles[] = {
        XCB_IM_PreeditPosition | XCB_IM_StatusNothing,
        XCB_IM_PreeditNothing | XCB_IM_StatusNothing,
    };
    static char *encodings[] = {
        "COMPOUND_TEXT",
        "UTF8_STRING",
    };
    xcb_im_styles_t input_styles = {2, styles};
    xcb_im_encodings_t encoding_list = {2, encodings};

    xcb_connection_t *conn = srv->conn;
    xcb_screen_t *screen = xcb_aux_get_screen(conn, srv->screen_number);
    TEST_CHECK(screen);

    srv->window = xcb_generate_id(conn);
    xcb_create_window(conn, XCB_COPY_FROM_PARENT, srv->window, screen->root, 0,
                      0, 1, 1, 1, XCB_WINDOW_CLASS_INPUT_OUTPUT,
                      screen->root_visual, 0, NULL);
    srv->stop_atom = intern_xcb_atom(conn, "_XCB_IMDKIT_TEST_STOP");
    srv->proto_atom = intern_xcb_atom(conn, "_XIM_PROTOCOL");

    xcb_im_t *im = xcb_im_create(
        conn, srv->screen_number, srv->window, srv->name, XCB_IM_ALL_LOCALES,
        &input_styles, NULL, NULL, &encoding_list,
        XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE, server_callback,
        srv);
    TEST_CHECK(im);
    xcb_im_set_use_sync_event(im, srv->use_sync_event);
    TEST_CHECK(xcb_im_open_im(im));
    xcb_flush(conn);

    TEST_CHECK(mtx_lock(&srv->mutex) == thrd_success);
    srv->started = true;
    TEST_CHECK(cnd_signal(&srv->ready) == thrd_success);
    TEST_CHECK(mtx_unlock(&srv->mutex) == thrd_success);

    xcb_generic_event_t *event;
    while ((event = xcb_wait_for_event(conn))) {
        if ((event->response_type & ~0x80) == XCB_CLIENT_MESSAGE) {
            xcb_client_message_event_t *cm =
                (xcb_client_message_event_t *)event;
            if (cm->window == srv->window && cm->type == srv->stop_atom) {
                free(event);
                break;
            }
            if (cm->type == srv->proto_atom && cm->format == 8 &&
                cm->data.data8[0] == XCB_XIM_CLOSE &&
                atomic_exchange(&srv->corrupt_next_close, false)) {
                // Truncate XIM_CLOSE body length to 0 so the server replies
                // with XIM_ERROR while libX11's XCloseIM is in _XimRead.
                cm->data.data8[2] = 0;
                cm->data.data8[3] = 0;
            }
        }
        xcb_im_filter_event(im, event);
        free(event);
    }

    xcb_im_close_im(im);
    xcb_im_destroy(im);
    xcb_destroy_window(conn, srv->window);
    xcb_aux_sync(conn);
    while ((event = xcb_poll_for_event(conn)) != NULL) {
        free(event);
    }
    return thrd_success;
}

static xlib_test_server_t *start_server(xcb_connection_t *conn,
                                        int screen_number, const char *name,
                                        bool use_sync_event) {
    xlib_test_server_t *srv = calloc(1, sizeof(*srv));
    TEST_CHECK(srv);
    srv->conn = conn;
    srv->screen_number = screen_number;
    srv->name = name;
    srv->use_sync_event = use_sync_event;
    TEST_CHECK(mtx_init(&srv->mutex, mtx_plain) == thrd_success);
    TEST_CHECK(cnd_init(&srv->ready) == thrd_success);
    TEST_CHECK(thrd_create(&srv->thread, server_thread_main, srv) ==
               thrd_success);

    TEST_CHECK(mtx_lock(&srv->mutex) == thrd_success);
    while (!srv->started) {
        TEST_CHECK(cnd_wait(&srv->ready, &srv->mutex) == thrd_success);
    }
    TEST_CHECK(mtx_unlock(&srv->mutex) == thrd_success);
    return srv;
}

static void stop_server(xlib_test_server_t *srv) {
    xcb_client_message_event_t ev = {
        .response_type = XCB_CLIENT_MESSAGE,
        .format = 32,
        .window = srv->window,
        .type = srv->stop_atom,
    };
    xcb_send_event(srv->conn, false, srv->window, XCB_EVENT_MASK_NO_EVENT,
                   (const char *)&ev);
    xcb_flush(srv->conn);

    TEST_CHECK(thrd_join(srv->thread, NULL) == thrd_success);
    cnd_destroy(&srv->ready);
    mtx_destroy(&srv->mutex);
    free(srv);
}

static void send_xlib_key(Display *dpy, Window win, unsigned int keycode,
                          int type, Time time) {
    XEvent ev;
    memset(&ev, 0, sizeof(ev));
    ev.xkey.type = type;
    ev.xkey.display = dpy;
    ev.xkey.window = win;
    ev.xkey.root = DefaultRootWindow(dpy);
    ev.xkey.time =
        time; // Must be non-zero so libX11 doesn't treat as fabricated
    ev.xkey.keycode = keycode;
    ev.xkey.same_screen = True;
    TEST_CHECK(XSendEvent(dpy, win, False,
                          type == KeyPress ? KeyPressMask : KeyReleaseMask,
                          &ev) != 0);
    XFlush(dpy);
}

static void run_xlib_round(xcb_connection_t *server_conn, int screen_number,
                           const char *server_name, bool use_sync_event,
                           bool test_close_error) {
    xlib_test_server_t *srv =
        start_server(server_conn, screen_number, server_name, use_sync_event);

    char mod[64];
    snprintf(mod, sizeof(mod), "@im=%s", server_name);
    TEST_CHECK(XSetLocaleModifiers(mod) != NULL);

    Display *dpy = XOpenDisplay(NULL);
    TEST_CHECK(dpy != NULL);

    Window win =
        XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0, 10, 10, 0, 0, 0);
    XSelectInput(dpy, win, KeyPressMask | KeyReleaseMask);

    XIM xim = XOpenIM(dpy, NULL, NULL, NULL);
    TEST_CHECK(xim != NULL);

    XIMStyles *xim_styles = NULL;
    TEST_CHECK(XGetIMValues(xim, XNQueryInputStyle, &xim_styles, NULL) == NULL);
    TEST_CHECK(xim_styles && xim_styles->count_styles == 2);
    XFree(xim_styles);

    char **missing_list = NULL;
    int missing_count = 0;
    char *def_str = NULL;
    XFontSet fontset =
        XCreateFontSet(dpy, "fixed,*", &missing_list, &missing_count, &def_str);
    if (missing_list) {
        XFreeStringList(missing_list);
    }

    XIC xic = NULL;
    if (fontset) {
        XPoint spot = {.x = 12, .y = 34};
        XVaNestedList preedit_attr = XVaCreateNestedList(
            0, XNSpotLocation, &spot, XNFontSet, fontset, NULL);
        TEST_CHECK(preedit_attr != NULL);
        xic =
            XCreateIC(xim, XNInputStyle, XIMPreeditPosition | XIMStatusNothing,
                      XNClientWindow, win, XNFocusWindow, win,
                      XNPreeditAttributes, preedit_attr, NULL);
        XFree(preedit_attr);
    } else {
        xic = XCreateIC(xim, XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
                        XNClientWindow, win, XNFocusWindow, win, NULL);
    }
    TEST_CHECK(xic != NULL);

    XSetICFocus(xic);

    // 1. Keycode 38 -> committed string via XIM_COMMIT + Xutf8LookupString.
    send_xlib_key(dpy, win, 38, KeyPress, 100);
    send_xlib_key(dpy, win, 38, KeyRelease, 101);

    bool got_commit = false;
    while (!got_commit) {
        XEvent ev;
        XNextEvent(dpy, &ev);
        if (XFilterEvent(&ev, None)) {
            continue;
        }
        if (ev.type == KeyPress) {
            char buf[64] = {0};
            KeySym ks = NoSymbol;
            Status status = 0;
            int len = Xutf8LookupString(xic, &ev.xkey, buf, sizeof(buf) - 1,
                                        &ks, &status);
            TEST_CHECK(status == XLookupChars);
            TEST_CHECK(len == (int)strlen(COMMIT_TEXT));
            TEST_CHECK(strcmp(buf, COMMIT_TEXT) == 0);
            got_commit = true;
        }
    }

    // 2. Keycode 39 -> forwarded back by server via XIM_FORWARD_EVENT.
    send_xlib_key(dpy, win, 39, KeyPress, 102);
    send_xlib_key(dpy, win, 39, KeyRelease, 103);

    bool got_press_39 = false;
    bool got_release_39 = false;
    while (!(got_press_39 && got_release_39)) {
        XEvent ev;
        XNextEvent(dpy, &ev);
        if (XFilterEvent(&ev, None)) {
            continue;
        }
        if (ev.type == KeyPress && ev.xkey.keycode == 39) {
            got_press_39 = true;
        } else if (ev.type == KeyRelease && ev.xkey.keycode == 39) {
            got_release_39 = true;
        }
    }

    // 3. Xutf8ResetIC must decode the Compound Text returned by XIM_RESET_IC.
    char *reset_str = Xutf8ResetIC(xic);
    TEST_CHECK(reset_str != NULL);
    TEST_CHECK(strcmp(reset_str, RESET_TEXT) == 0);
    XFree(reset_str);

    XUnsetICFocus(xic);
    XDestroyIC(xic);
    if (fontset) {
        XFreeFontSet(dpy, fontset);
    }

    if (test_close_error) {
        atomic_store(&srv->corrupt_next_close, true);
    }
    XCloseIM(xim);
    XDestroyWindow(dpy, win);
    XCloseDisplay(dpy);

    stop_server(srv);
}

int main(void) {
    xcb_compound_text_init();

    static const char *const locales[] = {
        "en_US.UTF-8", "en_US.utf8", "C.UTF-8", "C.utf8",
        "zh_CN.UTF-8", "zh_CN.utf8", "",
    };
    bool locale_ok = false;
    for (size_t i = 0; i < sizeof(locales) / sizeof(locales[0]); i++) {
        if (setlocale(LC_CTYPE, locales[i]) && XSupportsLocale()) {
            const char *cur = setlocale(LC_CTYPE, NULL);
            if (cur && strcmp(cur, "C") != 0 && strcmp(cur, "POSIX") != 0) {
                locale_ok = true;
                break;
            }
        }
    }
    if (!locale_ok) {
        fprintf(stderr, "No UTF-8 X11 locale available, skipping test_xlib\n");
        return 0;
    }

    int screen_number = 0;
    xcb_connection_t *server_conn = xcb_connect(NULL, &screen_number);
    TEST_CHECK(server_conn && !xcb_connection_has_error(server_conn));

    run_xlib_round(server_conn, screen_number, "test_xlib_async", false, true);
    run_xlib_round(server_conn, screen_number, "test_xlib_sync_ev", true,
                   false);

    xcb_disconnect(server_conn);
    return 0;
}
