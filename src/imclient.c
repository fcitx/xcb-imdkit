#include "imclient.h"
#include "imclient_p.h"
#include "common.h"
#include "ximproto.h"
#include "message.h"
#include <xcb/xcb_aux.h>
#include <stdlib.h>
#include <string.h>

// this phase is basically a directly mapping from _XimSOMETHING function in Xlib
// state machine is more suitable for xcb asynchronous nature.
typedef enum _xcb_xim_open_phase_t {
    XIM_OPEN_PHASE_DONE,
    XIM_OPEN_PHASE_FAIL,
    XIM_OPEN_PHASE_CHECK_SERVER,
    XIM_OPEN_PHASE_CONNECT,
} xcb_xim_open_phase_t;

typedef enum _xcb_xim_open_check_server_phase_t {
    XIM_OPEN_PHASE_CHECK_SERVER_PREPARE,
    XIM_OPEN_PHASE_CHECK_SERVER_LOCALE,
    XIM_OPEN_PHASE_CHECK_SERVER_LOCALE_WAIT,
    XIM_OPEN_PHASE_CHECK_SERVER_TRANSPORT,
    XIM_OPEN_PHASE_CHECK_SERVER_TRANSPORT_WAIT,
} xcb_xim_open_check_server_phase_t;

typedef enum _xcb_xim_open_connect_phase_t {
    XIM_OPEN_PHASE_CONNECT_PREPARE,
    XIM_OPEN_PHASE_CONNECT_WAIT,
    XIM_OPEN_PHASE_CONNECT_WAIT_REPLY,
    XIM_OPEN_PHASE_CONNECT_WAIT_OPEN_REPLY
} xcb_xim_open_connect_phase_t;

typedef struct _xcb_xim_open_t
{
    xcb_xim_open_phase_t phase;
    union {
        struct {
            int index;
            xcb_xim_open_check_server_phase_t subphase;
            xcb_window_t window;
            xcb_window_t requestor_window;
        } check_server;

        struct {
            xcb_xim_open_connect_phase_t subphase;
        } connect;
    };
} xcb_xim_open_t;

struct _xcb_xim_t
{
    char* server_name;
    int screen_id;
    xcb_connection_t* conn;
    xcb_atom_t atoms[XIM_ATOM_LAST]; // add one for SERVER_NAME
    bool init;
    xcb_screen_t* screen;
    xcb_atom_t* server_atoms;
    int n_server_atoms;
    bool connected;
    xcb_xim_open_t open;
    char* trans_addr;
    xcb_window_t im_window;
    xcb_window_t client_window;
    int major_code;
    int minor_code;
    uint32_t accept_win;
    uint32_t sequence;
    uint16_t connect_id;
    uint8_t byte_order;
};

#define CHECK_NEXT_SERVER(IM) \
    do { \
        (IM)->open.check_server.subphase = XIM_OPEN_PHASE_CHECK_SERVER_PREPARE; \
        (IM)->open.check_server.index++; \
        if ((IM)->open.check_server.requestor_window) { \
            xcb_destroy_window((IM)->conn, (IM)->open.check_server.requestor_window); \
        } \
    } while(0);

bool _xcb_xim_send_message(xcb_xim_t* im,
                           uint8_t* data, size_t length)
{
    char atomName[64];
    int len = sprintf(atomName, "_client%u_%u", im->connect_id, im->sequence++);
    return _xcb_send_xim_message(im->conn, im->atoms[XIM_ATOM_XIM_PROTOCOL], im->accept_win,
                                 data, length, atomName, len);
}


bool _xcb_xim_check_server_name(xcb_xim_t* im, char* name, int namelen)
{
    size_t category_len = strlen(XIM_SERVER_CATEGORY);
    if(strncmp(name, XIM_SERVER_CATEGORY, category_len) != 0) {
        return false;
    }

    if (!im->server_name) {
        return true;
    }

    return (strncmp(name + category_len, im->server_name, namelen - category_len) == 0);
}

bool _xcb_xim_check_transport(xcb_xim_t* im, char* address, int addresslen, char** trans_addr)
{
    size_t category_len = strlen(XIM_TRANSPORT_CATEGORY);
    if(strncmp(address, XIM_TRANSPORT_CATEGORY, strlen(XIM_TRANSPORT_CATEGORY)) != 0) {
        return false;
    }

    char *p, *pp = address + category_len;

    for(;;) {
        *trans_addr = pp;

        // find first non / , character
        for(p = pp; (*p != '/') && (*p != ',') && (*p); p++);
        if(*p == ',') {
            pp = p + 1;
            continue;
        }
        if(!(*p))
            return false;

        addresslen = (int)(p - pp);

        if(addresslen == strlen("X") && (0 == strncmp(pp, "X", strlen("X")))) {
            break;
        }
        pp = p + 1;
    }
    pp = p + 1;
    for(p = pp; (*p != ',') && (*p); p++);
    if (*p) {
        *p = '\0';
    }
    return true;
}

bool _xcb_xim_check_server_prepare(xcb_xim_t* im)
{
    xcb_atom_t server_atom = im->server_atoms[im->open.check_server.index];
    xcb_get_selection_owner_reply_t* owner_reply =
        xcb_get_selection_owner_reply(im->conn,
                                        xcb_get_selection_owner(im->conn, server_atom),
                                        NULL);
    if (!owner_reply) {
        return false;
    }
    im->open.check_server.window = owner_reply->owner;
    free(owner_reply);

    xcb_get_atom_name_reply_t* reply = xcb_get_atom_name_reply(im->conn,
                                                                xcb_get_atom_name(im->conn, server_atom),
                                                                NULL);
    if (!reply) {
        return false;
    }

    bool result = _xcb_xim_check_server_name(im, xcb_get_atom_name_name(reply), xcb_get_atom_name_name_length(reply));
    free(reply);

    if (!result) {
        return false;
    }

    xcb_window_t w = xcb_generate_id(im->conn);

    xcb_create_window (im->conn, XCB_COPY_FROM_PARENT, w, im->screen->root,
                        0, 0, 1, 1, 1,
                        XCB_WINDOW_CLASS_INPUT_OUTPUT,
                        im->screen->root_visual,
                        0, NULL);
    im->open.check_server.requestor_window = w;
    return true;
}

void _xcb_xim_check_server_transport(xcb_xim_t* im)
{
    xcb_atom_t server_atom = im->server_atoms[im->open.check_server.index];
    xcb_convert_selection(im->conn,
                            im->open.check_server.requestor_window,
                            server_atom, im->atoms[XIM_ATOM_TRANSPORT], im->atoms[XIM_ATOM_TRANSPORT],
                            XCB_CURRENT_TIME);

    xcb_flush(im->conn);
}

typedef enum _xcb_xim_open_phase_action_t
{
    ACTION_ACCEPT,
    ACTION_FAILED,
    ACTION_YIELD,
} xcb_xim_open_phase_action_t;

xcb_xim_open_phase_action_t _xcb_xim_check_server_transport_wait(xcb_xim_t* im, xcb_generic_event_t* event)
{
    xcb_atom_t server_atom = im->server_atoms[im->open.check_server.index];
    if (!event) {
        return ACTION_YIELD;
    }

    if ((event->response_type & ~0x80) != XCB_SELECTION_NOTIFY) {
        return ACTION_YIELD;
    }

    xcb_selection_notify_event_t* selection_notify = (xcb_selection_notify_event_t*) event;
    if (selection_notify->requestor != im->open.check_server.requestor_window) {
        return ACTION_YIELD;
    }

    if (selection_notify->property == XCB_ATOM_NONE) {
        return ACTION_FAILED;
    }

    xcb_get_property_cookie_t cookie = xcb_get_property(im->conn,
                                                        true,
                                                        im->open.check_server.requestor_window,
                                                        im->atoms[XIM_ATOM_TRANSPORT],
                                                        im->atoms[XIM_ATOM_TRANSPORT],
                                                        0L,
                                                        100000L);
    xcb_get_property_reply_t* reply = xcb_get_property_reply(im->conn, cookie, NULL);
    if (!reply) {
        return ACTION_FAILED;
    }

    char* value = xcb_get_property_value(reply);
    int length = xcb_get_property_value_length(reply);
    char* trans_addr;
    if (_xcb_xim_check_transport(im, value, length, &trans_addr)) {
        im->trans_addr = strdup(trans_addr);
        if (!im->trans_addr) {
            return ACTION_FAILED;
        }
        xcb_destroy_window(im->conn, im->open.check_server.requestor_window);
        im->im_window = im->open.check_server.window;
        im->atoms[XIM_ATOM_SERVER_NAME] = server_atom;
        return ACTION_ACCEPT;
    }

    return ACTION_FAILED;
}

void _xcb_xim_connect_prepare(xcb_xim_t* im) {
    xcb_window_t w = xcb_generate_id(im->conn);

    xcb_create_window (im->conn, XCB_COPY_FROM_PARENT, w, im->screen->root,
                        0, 0, 1, 1, 1,
                        XCB_WINDOW_CLASS_INPUT_OUTPUT,
                        im->screen->root_visual,
                        0, NULL);
    im->client_window = w;

    xcb_client_message_event_t ev;
    ev.response_type = XCB_CLIENT_MESSAGE;
    ev.format = 32;
    ev.sequence = 0;
    ev.type = im->atoms[XIM_ATOM_XIM_CONNECT];
    ev.window = im->im_window;
    ev.data.data32[0] = im->client_window;
    ev.data.data32[1] = 0; // major
    ev.data.data32[2] = 0; // minor
    ev.data.data32[3] = 0;
    ev.data.data32[4] = 0;
    xcb_send_event(im->conn, false, im->im_window, XCB_EVENT_MASK_NO_EVENT, (char*) &ev);
    xcb_flush(im->conn);
}

xcb_xim_open_phase_action_t _xcb_xim_connect_wait(xcb_xim_t* im, xcb_generic_event_t* event)
{
    if (!event) {
        return ACTION_YIELD;
    }
    if ((event->response_type & ~0x80) != XCB_CLIENT_MESSAGE) {
        return ACTION_YIELD;
    }

    xcb_client_message_event_t* client_message = (xcb_client_message_event_t*) event;
    if (client_message->type != im->atoms[XIM_ATOM_XIM_CONNECT]) {
        return ACTION_YIELD;
    }
    event = NULL;
    im->major_code = 0;
    im->minor_code = 0;
    im->accept_win = client_message->data.data32[0];

    xcb_im_connect_fr_t frame;
    frame.byte_order = im->byte_order;
    frame.client_auth_protocol_names.size = 0;
    frame.client_auth_protocol_names.items = NULL;
    frame.client_major_protocol_version = 0;
    frame.client_minor_protocol_version = 0;

    bool fail;
    _xcb_xim_send_frame(im, frame, fail);
    if (fail) {
        return ACTION_FAILED;
    }

    return ACTION_ACCEPT;
}

bool _xcb_xim_send_open(xcb_xim_t* im)
{
    xcb_im_open_fr_t frame;
    frame.field0.length_of_string = 0;
    frame.field0.string = 0;

    bool fail;
    _xcb_xim_send_frame(im, frame, fail);
    return !fail;
}

xcb_xim_open_phase_action_t _xcb_xim_connect_wait_reply(xcb_xim_t* im, xcb_generic_event_t* event)
{
    if (!event) {
        return ACTION_YIELD;
    }
    if ((event->response_type & ~0x80) != XCB_CLIENT_MESSAGE) {
        return ACTION_YIELD;
    }
    xcb_client_message_event_t* client_message = (xcb_client_message_event_t*) event;
    if (client_message->type != im->atoms[XIM_ATOM_XIM_PROTOCOL]) {
        return ACTION_YIELD;
    }

    xcb_im_packet_header_fr_t hdr;
    uint8_t* message = _xcb_read_xim_message(im->conn, im->accept_win, client_message, &hdr, false);

    if (!message) {
        return ACTION_FAILED;
    }

    if (hdr.major_opcode != XIM_CONNECT_REPLY) {
        return ACTION_YIELD;
    }

    xcb_xim_open_phase_action_t result = ACTION_FAILED;
    do {
        xcb_im_connect_reply_fr_t reply_frame;
        bool fail;
        _xcb_xim_read_frame(reply_frame, message, XIM_MESSAGE_BYTES(&hdr), fail);
        if (fail) {
            break;
        }

        if (!_xcb_xim_send_open(im)) {
            result = ACTION_FAILED;
        } else {
            result = ACTION_ACCEPT;
        }
    } while(0);

    free(message);
    return result;
}

bool _xcb_xim_preconnect_im(xcb_xim_t* im, xcb_generic_event_t* event)
{
    while (im->open.phase != XIM_OPEN_PHASE_DONE && im->open.phase != XIM_OPEN_PHASE_FAIL) {
        if (im->open.phase == XIM_OPEN_PHASE_CHECK_SERVER) {
            if (im->open.check_server.index == im->n_server_atoms) {
                im->open.phase = XIM_OPEN_PHASE_FAIL;
                continue;
            }
            if (im->open.check_server.subphase == XIM_OPEN_PHASE_CHECK_SERVER_PREPARE) {
                if (_xcb_xim_check_server_prepare(im)) {
                    im->open.check_server.subphase = XIM_OPEN_PHASE_CHECK_SERVER_LOCALE;
                } else {
                    CHECK_NEXT_SERVER(im);
                    continue;
                }
            } else if (im->open.check_server.subphase == XIM_OPEN_PHASE_CHECK_SERVER_LOCALE) {
                // TODO
                im->open.check_server.subphase = XIM_OPEN_PHASE_CHECK_SERVER_TRANSPORT;
            } else if (im->open.check_server.subphase == XIM_OPEN_PHASE_CHECK_SERVER_TRANSPORT) {
                _xcb_xim_check_server_transport(im);
                im->open.check_server.subphase = XIM_OPEN_PHASE_CHECK_SERVER_TRANSPORT_WAIT;
            } else if (im->open.check_server.subphase == XIM_OPEN_PHASE_CHECK_SERVER_TRANSPORT_WAIT) {
                xcb_xim_open_phase_action_t result = _xcb_xim_check_server_transport_wait(im, event);
                switch (result) {
                    case ACTION_ACCEPT:
                        event = NULL;
                        im->open.phase = XIM_OPEN_PHASE_CONNECT;
                        im->open.connect.subphase = XIM_OPEN_PHASE_CONNECT_PREPARE;
                        break;
                    case ACTION_FAILED:
                        event = NULL;
                        CHECK_NEXT_SERVER(im);
                        break;
                    case ACTION_YIELD:
                        return false;
                }
            }
        } else if (im->open.phase == XIM_OPEN_PHASE_CONNECT) {
            if (im->open.connect.subphase == XIM_OPEN_PHASE_CONNECT_PREPARE) {
                _xcb_xim_connect_prepare(im);
                im->open.connect.subphase = XIM_OPEN_PHASE_CONNECT_WAIT;
            } else if (im->open.connect.subphase == XIM_OPEN_PHASE_CONNECT_WAIT) {
                xcb_xim_open_phase_action_t result = _xcb_xim_connect_wait(im, event);
                switch (result) {
                    case ACTION_ACCEPT:
                        event = NULL;
                        im->open.connect.subphase = XIM_OPEN_PHASE_CONNECT_WAIT_REPLY;
                        break;
                    case ACTION_FAILED:
                        event = NULL;
                        im->open.phase = XIM_OPEN_PHASE_FAIL;
                        break;
                    case ACTION_YIELD:
                        return false;
                }
            } else if (im->open.connect.subphase == XIM_OPEN_PHASE_CONNECT_WAIT_REPLY) {
                xcb_xim_open_phase_action_t result = _xcb_xim_connect_wait_reply(im, event);
                switch (result) {
                    case ACTION_ACCEPT:
                        event = NULL;
                        im->open.phase = XIM_OPEN_PHASE_DONE;
                        break;
                    case ACTION_FAILED:
                        event = NULL;
                        im->open.phase = XIM_OPEN_PHASE_FAIL;
                        break;
                    case ACTION_YIELD:
                        return false;
                }

            }
        }
    }

    return event == NULL;
}

bool _xcb_xim_init(xcb_xim_t* im)
{
    if (im->init) {
        return true;
    }

    const char* atom_names[] = {XIM_SERVERS, XIM_LOCALES, XIM_TRANSPORT, _XIM_PROTOCOL, _XIM_XCONNECT};
    // plus 1 for server name
    if (!_xcb_im_init_atoms(im->conn, ARRAY_SIZE(atom_names), atom_names, im->atoms + 1)) {
        return false;
    }
    im->screen = xcb_aux_get_screen(im->conn, im->screen_id);
    if (!im->screen) {
        return false;
    }

    im->init = true;

    return true;
}


bool _xcb_xim_get_servers(xcb_xim_t* im)
{
    xcb_get_property_cookie_t cookie = xcb_get_property(im->conn,
                                                        false,
                                                        im->screen->root,
                                                        im->atoms[XIM_ATOM_XIM_SERVERS],
                                                        XCB_ATOM_ATOM,
                                                        0L,
                                                        100000L);

    xcb_get_property_reply_t* reply = xcb_get_property_reply(im->conn, cookie, NULL);
    if (!reply) {
        return false;
    }

    do {
        if (reply->type != XCB_ATOM_ATOM || reply->format != 32) {
            break;
        }

        int items = xcb_get_property_value_length(reply) / sizeof(xcb_atom_t);
        im->n_server_atoms = items;
        if (items <= 0) {
            break;
        }
        xcb_atom_t* server_atoms = xcb_get_property_value(reply);
        im->server_atoms = calloc(items, sizeof(xcb_atom_t));
        memcpy(im->server_atoms, server_atoms, items * sizeof(xcb_atom_t));
    } while(0);

    free(reply);

    return (im->n_server_atoms > 0);
}

char* _xcb_xim_make_im_name(const char* imname)
{
    if (!imname || strncmp(imname, "@im=", strlen("@im=")) != 0) {
        return NULL;
    }

    imname += strlen("@im=");

    return strdup(imname);
}

xcb_xim_t* xcb_xim_create(xcb_connection_t* conn, int screen_id, const char* imname)
{
    xcb_xim_t* im = calloc(1, sizeof(xcb_xim_t));

    if (!imname) {
        imname = getenv("XMODIFIERS");
    }

    im->conn = conn;
    im->server_name = _xcb_xim_make_im_name(imname);
    im->screen_id = screen_id;
    uint16_t endian = 1;
    if (*(char *) &endian) {
        im->byte_order = 'l';
    } else {
        im->byte_order = 'B';
    }
    return im;
}

bool xcb_xim_open(xcb_xim_t* im)
{
    im->open.phase = XIM_OPEN_PHASE_FAIL;

    if (!_xcb_xim_init(im)) {
        return false;
    }

    if (!_xcb_xim_get_servers(im)) {
        return false;
    }
    im->open.phase = XIM_OPEN_PHASE_CHECK_SERVER;
    im->open.check_server.index = 0;
    im->open.check_server.requestor_window = 0;
    im->open.check_server.window = 0;
    im->open.check_server.subphase = XIM_OPEN_PHASE_CHECK_SERVER_PREPARE;

    _xcb_xim_preconnect_im(im, NULL);
    return true;
}

void _xcb_xim_handle_message(xcb_xim_t* im, const xcb_im_packet_header_fr_t* hdr, uint8_t* data)
{
    switch (hdr->major_opcode) {
    case XIM_OPEN_REPLY:
        DebugLog("-- XIM_OPEN_REPLY\n");
        break;
    case XIM_REGISTER_TRIGGERKEYS:
        DebugLog("-- XIM_REGISTER_TRIGGERKEYS\n");
        break;
    case XIM_QUERY_EXTENSION_REPLY:
        DebugLog("-- XIM_QUERY_EXTENSION_REPLY\n");
        break;
    case XIM_ENCODING_NEGOTIATION_REPLY:
        DebugLog("-- XIM_ENCODING_NEGOTIATION_REPLY\n");
        break;
    case XIM_GET_IM_VALUES_REPLY:
        DebugLog("-- XIM_GET_IM_VALUES_REPLY\n");
        break;
    case XIM_SET_EVENT_MASK:
        DebugLog("-- XIM_COMMIT\n");
        break;
    case XIM_CREATE_IC_REPLY:
        DebugLog("-- XIM_CREATE_IC_REPLY\n");
        break;
    case XIM_GET_IC_VALUES_REPLY:
        DebugLog("-- XIM_GET_IC_VALUES_REPLY\n");
        break;
    case XIM_SET_IC_VALUES_REPLY:
        DebugLog("-- XIM_SET_IC_VALUES_REPLY\n");
        break;
    case XIM_FORWARD_EVENT:
        DebugLog("-- XIM_FORWARD_EVENT\n");
        break;
    case XIM_SYNC:
        DebugLog("-- XIM_SYNC\n");
        break;
    case XIM_COMMIT:
        DebugLog("-- XIM_COMMIT\n");
        break;
    case XIM_GEOMETRY:
        DebugLog("-- XIM_GEOMETRY\n");
        break;
    case XIM_PREEDIT_START:
        DebugLog("-- XIM_PREEDIT_START\n");
        break;
    case XIM_PREEDIT_DRAW:
        DebugLog("-- XIM_PREEDIT_DRAW\n");
        break;
    case XIM_PREEDIT_CARET:
        DebugLog("-- XIM_PREEDIT_CARET\n");
        break;
    case XIM_PREEDIT_DONE:
        DebugLog("-- XIM_PREEDIT_DONE\n");
        break;
    case XIM_STATUS_START:
        DebugLog("-- XIM_STATUS_START\n");
        break;
    case XIM_STATUS_DRAW:
        DebugLog("-- XIM_STATUS_DRAW\n");
        break;
    case XIM_STATUS_DONE:
        DebugLog("-- XIM_STATUS_DONE\n");
        break;
    case XIM_CLOSE_REPLY:
        DebugLog("-- XIM_CLOSE_REPLY\n");
        break;
    case XIM_DESTROY_IC_REPLY:
        DebugLog("-- XIM_DESTROY_IC_REPLY\n");
        break;
    case XIM_RESET_IC_REPLY:
        DebugLog("-- XIM_DESTROY_IC_REPLY\n");
        break;

    }
}

bool _xcb_xim_filter_event(xcb_xim_t* im, xcb_generic_event_t* event)
{
    if (im->open.phase != XIM_OPEN_PHASE_DONE) {
        return false;
    }

    do {
        if ((event->response_type & ~0x80) != XCB_CLIENT_MESSAGE) {
            break;
        }

        xcb_client_message_event_t* clientmessage = (xcb_client_message_event_t*) event;

        if (clientmessage->type != im->atoms[XIM_ATOM_XIM_PROTOCOL]) {
            break;
        }

        xcb_im_packet_header_fr_t hdr;
        uint8_t* message = _xcb_read_xim_message(im->conn, im->client_window, clientmessage, &hdr, false);
        if (message) {
            _xcb_xim_handle_message(im, &hdr, message);
            free(message);
        }

        return true;

    } while(0);

    return false;
}


bool xcb_xim_filter_event(xcb_xim_t* im, xcb_generic_event_t* event)
{
    return _xcb_xim_preconnect_im(im, event) || _xcb_xim_filter_event(im, event);
}

void xcb_xim_close(xcb_xim_t* im)
{

}
