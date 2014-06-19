#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_atom.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uthash.h>
#include "imdkit.h"
#include "parser.h"
#include "ximproto.h"
#include "protocolhandler.h"
#include "message.h"

typedef struct _xcb_im_ext_t {
    uint16_t  major_opcode;
    uint16_t  minor_opcode;
    uint16_t  length;
    const char    *name;
} xcb_im_ext_t;

#define SIMPLE_ARRAY_FUNC(NAME, ARRAY_TYPE, ELEM_TYPE, NUM, PTR) \
void _copy_##NAME(ARRAY_TYPE* to, const ARRAY_TYPE* from) \
{ \
    if (!from || !from->NUM) { \
        to->NUM = 0; \
        to->PTR = NULL; \
        return; \
    } \
    to->PTR = malloc(from->NUM * sizeof(ELEM_TYPE)); \
    if (!to->PTR) { \
        return; \
    } \
    to->NUM = from->NUM; \
    memcpy(to->PTR, from->PTR, from->NUM * sizeof(ELEM_TYPE)); \
} \
void _free_##NAME(ARRAY_TYPE* to) \
{ \
    free(to->PTR); \
}

SIMPLE_ARRAY_FUNC(trigger_keys,
                    xcb_im_trigger_keys_t,
                    xcb_im_trigger_key_t,
                    nKeys,
                    keys)

SIMPLE_ARRAY_FUNC(input_styles,
                    xcb_im_styles_t,
                    xcb_im_style_t,
                    nStyles,
                    styles)

void _copy_encodings (xcb_im_encodings_t * to, const xcb_im_encodings_t * from)
{
    to->encodings  = malloc(from->nEncodings * sizeof(xcb_im_encoding_t));
    if (!to->encodings) {
        return;
    }
    to->nEncodings = from->nEncodings;
    for (int i = 0;i < to->nEncodings; i++) {
        to->encodings[i] = strdup(from->encodings[i]);
    }
}

void _free_encodings (xcb_im_encodings_t * to)
{
    for (int i = 0;i < to->nEncodings; i++) {
        free(to->encodings[i]);
    }
    free(to->encodings);
}

xcb_im_t* xcb_im_create(xcb_connection_t* conn,
                        int screen,
                        xcb_window_t serverWindow,
                        const char* serverName,
                        const char* locale,
                        const xcb_im_styles_t* inputStyles,
                        const xcb_im_trigger_keys_t* onKeysList,
                        const xcb_im_trigger_keys_t* offKeysList,
                        const xcb_im_encodings_t* encodingList,
                        uint32_t event_mask,
                        xcb_im_callback callback,
                        void* user_data)
{
    xcb_im_t* im = calloc(1, sizeof(xcb_im_t));
    im->conn = conn;
    im->screen_id = screen;
    im->callback = callback;
    im->user_data = user_data;

    if (event_mask) {
        im->event_mask = XCB_EVENT_MASK_KEY_PRESS;
    } else {
        im->event_mask = event_mask;
    }

    _copy_trigger_keys(&im->onKeys, onKeysList);
    _copy_trigger_keys(&im->offKeys, offKeysList);
    _copy_input_styles(&im->inputStyles, inputStyles);
    _copy_encodings(&im->encodings, encodingList);
    im->locale = strdup(locale);
    im->serverName = strdup(serverName);
    im->serverWindow = serverWindow;

    uint16_t endian = 1;
    if (*(char *) &endian) {
        im->byte_order = 'l';
    } else {
        im->byte_order = 'B';
    }

    int id = 0;
    for (size_t i = 0; i < ARRAY_SIZE(Default_IMattr); i++) {
        im->imattr[i].im_attribute = (uint8_t*) Default_IMattr[i].name;
        im->imattr[i].length_of_im_attribute = strlen(Default_IMattr[i].name);
        im->imattr[i].type_of_the_value = Default_IMattr[i].type;
        im->imattr[i].attribute_ID = id;
        im->id2attr[id] = &im->imattr[i];
        id++;
    }

    for (size_t i = 0; i < ARRAY_SIZE(Default_ICattr); i++) {
        im->icattr[i].ic_attribute = (uint8_t*) Default_ICattr[i].name;
        im->icattr[i].length_of_ic_attribute = strlen(Default_ICattr[i].name);
        im->icattr[i].type_of_the_value = Default_ICattr[i].type;
        im->icattr[i].attribute_ID = id++;
        if (strcmp(Default_ICattr[i].name, XNPreeditAttributes) == 0) {
            im->preeditAttr_id = im->icattr[i].attribute_ID;
        } else if (strcmp(Default_ICattr[i].name, XNStatusAttributes) == 0) {
            im->statusAttr_id = im->icattr[i].attribute_ID;
        } else if (strcmp(Default_ICattr[i].name, XNSeparatorofNestedList) == 0) {
            im->separatorAttr_id = im->icattr[i].attribute_ID;
        }
        im->id2attr[id] = (ximattr_fr*) &im->icattr[i];
    }

    for (size_t i = 0;  i < ARRAY_SIZE(Default_Extension);  i++) {
        im->extension[i].extension_major_opcode = Default_Extension[i].major_opcode;
        im->extension[i].extension_minor_opcode = Default_Extension[i].minor_opcode;
        im->extension[i].extension_name = (uint8_t*) Default_Extension[i].name;
        im->extension[i].length_of_extension_name = strlen(Default_Extension[i].name);
    }

    return im;
}

bool _xcb_im_init(xcb_im_t* im)
{
    if (im->init) {
        return true;
    }
    xcb_screen_t* screen = xcb_aux_get_screen(im->conn, im->screen_id);

    if (!screen) {
        return false;
    }

    im->screen = screen;
    xcb_window_t root = screen->root;
    im->root = root;
    char* buf;
    asprintf(&buf, "@server=%s", im->serverName);
    const char* atom_names[] = {buf, XIM_SERVERS, XIM_LOCALES, XIM_TRANSPORT, _XIM_PROTOCOL, _XIM_XCONNECT};
    xcb_intern_atom_cookie_t atom_cookies[ARRAY_SIZE(atom_names)];
    for (size_t i = 0; i < ARRAY_SIZE(atom_names); i ++) {
        atom_cookies[i] = xcb_intern_atom(im->conn, false, strlen(atom_names[i]), atom_names[i]);
    }
    xcb_atom_t* atoms = im->atoms;
    size_t i;
    for (i = 0; i < ARRAY_SIZE(atom_names); i ++) {
        xcb_intern_atom_reply_t* atom_reply = xcb_intern_atom_reply(im->conn, atom_cookies[i], NULL);
        if (atom_reply) {
            atoms[i] = atom_reply->atom;
            free(atom_reply);
        } else {
            break;
        }
    }

    free(buf);
    if (i == ARRAY_SIZE(atom_names)) {
        im->init = true;
    }

    return im->init;
}

bool _xcb_im_set_selection_owner(xcb_im_t* im)
{
    xcb_atom_t* atoms = im->atoms;
    xcb_get_property_cookie_t cookie = xcb_get_property(im->conn,
                                                        false,
                                                        im->root,
                                                        atoms[XIM_ATOM_XIM_SERVERS],
                                                        XCB_ATOM_ATOM,
                                                        0L,
                                                        1000000L);

    xcb_get_property_reply_t* reply = xcb_get_property_reply(im->conn, cookie, NULL);
    bool found = false;
    bool conflict = false;
    bool result = false;
    do {
        if (!reply) {
            break;
        }
        if (reply->type != XCB_ATOM_NONE && (reply->type != XCB_ATOM_ATOM || reply->format != 32)) {
            break;
        }
        uint32_t* data = xcb_get_property_value(reply);
        if (!data) {
            break;
        }
        uint32_t length = xcb_get_property_value_length(reply) / sizeof(uint32_t);
        for (uint32_t i = 0; i < length ; i++) {
            if (data[i] == atoms[XIM_ATOM_SERVER_NAME]) {
                found = true;
                xcb_get_selection_owner_reply_t* owner_reply = xcb_get_selection_owner_reply(im->conn,
                                                                                             xcb_get_selection_owner(im->conn, atoms[XIM_ATOM_SERVER_NAME]),
                                                                                             NULL);
                if (owner_reply) {
                    xcb_window_t owner = owner_reply->owner;
                    free(owner_reply);
                    if (owner != im->serverWindow) {
                        if (owner == XCB_WINDOW_NONE) {
                            xcb_set_selection_owner(im->conn, im->serverWindow, atoms[XIM_ATOM_SERVER_NAME], XCB_CURRENT_TIME);
                        } else {
                            conflict = true;
                        }
                    }
                }
                break;
            }
        }

        if (conflict) {
            break;
        }

        if (!found) {
            xcb_set_selection_owner(im->conn, im->serverWindow, atoms[XIM_ATOM_SERVER_NAME], XCB_CURRENT_TIME);
            xcb_change_property(im->conn,
                                XCB_PROP_MODE_PREPEND,
                                im->root,
                                atoms[XIM_ATOM_XIM_SERVERS],
                                XCB_ATOM_ATOM,
                                32,
                                1,
                                &atoms[XIM_ATOM_SERVER_NAME]);
        } else {
            /*
            * We always need to generate the PropertyNotify to the Root Window
            */
            xcb_change_property(im->conn,
                                XCB_PROP_MODE_PREPEND,
                                im->root,
                                atoms[XIM_ATOM_XIM_SERVERS],
                                XCB_ATOM_ATOM,
                                32,
                                0,
                                data);
        }

        xcb_get_selection_owner_reply_t* owner_reply = xcb_get_selection_owner_reply(im->conn,
                                                                                     xcb_get_selection_owner(im->conn, atoms[XIM_ATOM_SERVER_NAME]),
                                                                                     NULL);
        if (owner_reply) {
            xcb_window_t owner = owner_reply->owner;
            free(owner_reply);
            result = owner == im->serverWindow;
        }
        xcb_flush(im->conn);
    } while(0);
    free(reply);

    return result;
}

bool xcb_im_open_im(xcb_im_t* im)
{
    if (!_xcb_im_init(im)) {
        return false;
    }

    if (!_xcb_im_set_selection_owner(im)) {
        return false;
    }

    return true;
}

xcb_im_client_table_t* _xcb_im_new_client(xcb_im_t* im, xcb_window_t client_window)
{
    xcb_im_client_table_t* client;
    int new_connect_id;
    if (im->free_list) {
        client = im->free_list;
        im->free_list = im->free_list->hh1.next;
        new_connect_id = client->c.connect_id;
    } else {
        client = calloc(1, sizeof(xcb_im_client_table_t));
        new_connect_id = ++im->connect_id;
        client->c.connect_id = new_connect_id;
        HASH_ADD(hh1, im->clients_by_id, c.connect_id, sizeof(int), client);
    }

    xcb_window_t w = xcb_generate_id (im->conn);
    xcb_create_window (im->conn, XCB_COPY_FROM_PARENT, w, im->root,
                       0, 0, 1, 1, 1,
                       XCB_WINDOW_CLASS_INPUT_OUTPUT,
                       im->screen->root_visual,
                       0, NULL);

    client->c.client_win = client_window;
    client->c.accept_win = w;
    client->c.byte_order = '?'; // initial value
    HASH_ADD(hh2, im->clients_by_win, c.accept_win, sizeof(xcb_window_t), client);

    return client;
}

bool _xcb_im_filter_xconnect_message(xcb_im_t* im, xcb_generic_event_t* event)
{
    do {
        if ((event->response_type & ~0x80) != XCB_CLIENT_MESSAGE) {
            break;
        }

        xcb_client_message_event_t* clientmessage = (xcb_client_message_event_t*) event;
        if (clientmessage->window != im->serverWindow) {
            break;
        }

        if (clientmessage->type != im->atoms[XIM_ATOM_XIM_CONNECT]) {
            break;
        }

        xcb_window_t client_window = clientmessage->data.data32[0];
        uint32_t major_version = clientmessage->data.data32[1];
        uint32_t minor_version = clientmessage->data.data32[2];

        xcb_im_client_table_t *client = _xcb_im_new_client(im, client_window);
        if (major_version != 0  ||  minor_version != 0) {
            major_version = minor_version = 0;
            /* Only supporting only-CM & Property-with-CM method */
        }

        xcb_client_message_event_t ev;
        ev.response_type = XCB_CLIENT_MESSAGE;
        ev.window = client_window;
        ev.type = im->atoms[XIM_ATOM_XIM_CONNECT];
        ev.sequence = 0;
        ev.format = 32;
        ev.data.data32[0] = client->c.accept_win;
        ev.data.data32[1] = major_version;
        ev.data.data32[2] = minor_version;
        ev.data.data32[3] = XCM_DATA_LIMIT;
        xcb_send_event(im->conn, false, client_window, XCB_EVENT_MASK_NO_EVENT, (char*) &ev);
        xcb_flush(im->conn);

        return true;
    } while(0);

    return false;
}

bool _xcb_im_filter_selection_request(xcb_im_t* im, xcb_generic_event_t* event)
{
    do {
        if ((event->response_type & ~0x80) != XCB_SELECTION_REQUEST) {
            break;
        }

        xcb_selection_request_event_t* selection_request = (xcb_selection_request_event_t*) event;
        if (selection_request->owner != im->serverWindow) {
            break;
        }

        if (selection_request->selection != im->atoms[XIM_ATOM_SERVER_NAME]) {
            break;
        }
        char buf[LOCALES_BUFSIZE];
        if (selection_request->target == im->atoms[XIM_ATOM_LOCALES]) {
            snprintf(buf, LOCALES_BUFSIZE - 2, "@locale=%s", im->locale);
            buf[LOCALES_BUFSIZE-1] = 0;
        } else if (selection_request->target == im->atoms[XIM_ATOM_TRANSPORT]) {
            strcpy(buf, "@transport=X/");
        } else {
            return true;
        }
        DebugLog("requestor:%u\n", selection_request->requestor);
        DebugLog("buf:%s\n", buf);
        xcb_change_property(im->conn,
                            XCB_PROP_MODE_REPLACE,
                            selection_request->requestor,
                            selection_request->target,
                            selection_request->target,
                            8,
                            strlen(buf),
                            (unsigned char*)buf);

        xcb_selection_notify_event_t ev;
        ev.sequence = 0;
        ev.response_type = XCB_SELECTION_NOTIFY;
        ev.requestor = selection_request->requestor;
        ev.selection = selection_request->selection;
        ev.target = selection_request->target;
        ev.time = selection_request->time;
        ev.property = selection_request->property;
        xcb_send_event(im->conn, false, ev.requestor, XCB_EVENT_MASK_NO_EVENT, (char*) &ev);
        xcb_flush(im->conn);

        return true;
    } while(0);

    return false;
}

static uint8_t* _xcb_im_read_message(xcb_im_t* im,
                                     xcb_client_message_event_t *ev,
                                     xcb_im_client_table_t* client,
                                     xcb_im_proto_header_t* hdr)
{
    uint8_t *p = NULL;

    if (ev->format == 8) {
        if (client->c.byte_order == '?') {
            // major_opcode
            if (ev->data.data8[0] != XIM_CONNECT) {
                return (unsigned char *) NULL;  /* can do nothing */
            }
            client->c.byte_order = ev->data.data8[XCB_IM_HEADER_SIZE];
        }
        /* ClientMessage only */
        uint8_t* rec = ev->data.data8;
        size_t len  = sizeof(ev->data.data8);
        uint8_t_read(&hdr->major_opcode, &rec, &len, client->c.byte_order != im->byte_order);
        uint8_t_read(&hdr->minor_opcode, &rec, &len, client->c.byte_order != im->byte_order);
        uint16_t_read(&hdr->length, &rec, &len, client->c.byte_order != im->byte_order);

        // check message is well formed
        if (len >= hdr->length * 4) {
            p = malloc(hdr->length * 4);
        }
        if (p) {
            memcpy(p, rec, hdr->length * 4);
        }
    } else if (ev->format == 32) {
        /* ClientMessage and WindowProperty */
        size_t length = ev->data.data32[0];
        xcb_atom_t atom = ev->data.data32[1];

        xcb_get_property_cookie_t cookie = xcb_get_property(im->conn,
                                                           true,
                                                           client->c.accept_win,
                                                           atom,
                                                           XCB_ATOM_ANY,
                                                           0L,
                                                           length);

        xcb_get_property_reply_t* reply = xcb_get_property_reply(im->conn, cookie, NULL);
        uint8_t* rec;

        do {
            if (!reply || reply->format == 0 || reply->length == 0) {
                free(reply);
                return (unsigned char *) NULL;
            }

            rec = xcb_get_property_value(reply);

            if (length != reply->value_len)
                length = reply->value_len;

            // make length into byte
            if (reply->format == 16)
                length *= 2;
            else if (reply->format == 32)
                length *= 4;

            uint8_t_read(&hdr->major_opcode, &rec, &length, client->c.byte_order != im->byte_order);
            uint8_t_read(&hdr->minor_opcode, &rec, &length, client->c.byte_order != im->byte_order);
            uint16_t_read(&hdr->length, &rec, &length, client->c.byte_order != im->byte_order);

            // check message is well formed
            if (hdr->length * 4 <= length) {
                /* if hit, it might be an error */
                p = malloc(hdr->length * 4);
            }
        } while(0);

        if (p) {
            memcpy(p, rec, hdr->length * 4);
        }
        free(reply);
    }
    return (unsigned char *) p;
}

void _xcb_im_handle_message(xcb_im_t* im,
                            xcb_im_client_table_t* client,
                            const xcb_im_proto_header_t* hdr,
                            uint8_t* data,
                            bool *del)
{
    switch (hdr->major_opcode) {
    case XIM_CONNECT:
        DebugLog("-- XIM_CONNECT\n");
        _xcb_im_handle_connect(im, client, hdr, data);
        break;

    case XIM_DISCONNECT:
        DebugLog("-- XIM_DISCONNECT\n");
        _xcb_im_handle_disconnect(im, client, hdr, data, del);
        break;

    case XIM_OPEN:
        DebugLog("-- XIM_OPEN\n");
        _xcb_im_handle_open(im, client, hdr, data, del);
        break;

    case XIM_CLOSE:
        DebugLog("-- XIM_CLOSE\n");
        _xcb_im_handle_close(im, client, hdr, data, del);
        break;

    case XIM_QUERY_EXTENSION:
        DebugLog("-- XIM_QUERY_EXTENSION\n");
        _xcb_im_handle_query_extension(im, client, hdr, data, del);
        break;

    case XIM_GET_IM_VALUES:
        _xcb_im_handle_get_im_values(im, client, hdr, data, del);
        DebugLog("-- XIM_GET_IM_VALUES\n");
        break;

    case XIM_CREATE_IC:
        DebugLog("-- XIM_CREATE_IC\n");
        _xcb_im_handle_create_ic(im, client, hdr, data, del);
        break;

    case XIM_SET_IC_VALUES:
        DebugLog("-- XIM_SET_IC_VALUES\n");
        break;

    case XIM_GET_IC_VALUES:
        DebugLog("-- XIM_GET_IC_VALUES\n");
        break;

    case XIM_SET_IC_FOCUS:
        DebugLog("-- XIM_SET_IC_FOCUS\n");
        break;

    case XIM_UNSET_IC_FOCUS:
        DebugLog("-- XIM_UNSET_IC_FOCUS\n");
        break;

    case XIM_DESTROY_IC:
        DebugLog("-- XIM_DESTROY_IC\n");
        break;

    case XIM_RESET_IC:
        DebugLog("-- XIM_RESET_IC\n");
        break;

    case XIM_FORWARD_EVENT:
        DebugLog("-- XIM_FORWARD_EVENT\n");
        break;

    case XIM_EXTENSION:
        DebugLog("-- XIM_EXTENSION\n");
        break;

    case XIM_SYNC:
        DebugLog("-- XIM_SYNC\n");
        break;

    case XIM_SYNC_REPLY:
        DebugLog("-- XIM_SYNC_REPLY\n");
        break;

    case XIM_TRIGGER_NOTIFY:
        DebugLog("-- XIM_TRIGGER_NOTIFY\n");
        break;

    case XIM_ENCODING_NEGOTIATION:
        DebugLog("-- XIM_ENCODING_NEGOTIATION\n");
        _xcb_im_handle_encoding_negotiation(im, client, hdr, data, del);
        break;

    case XIM_PREEDIT_START_REPLY:
        DebugLog("-- XIM_PREEDIT_START_REPLY\n");
        break;

    case XIM_PREEDIT_CARET_REPLY:
        DebugLog("-- XIM_PREEDIT_CARET_REPLY\n");
        break;

    case XIM_STR_CONVERSION_REPLY:
        DebugLog("-- XIM_STR_CONVERSION_REPLY\n");
        break;
    }
    /*endswitch*/
}

bool _xcb_im_filter_client(xcb_im_t* im, xcb_generic_event_t* event)
{
    do {
        if ((event->response_type & ~0x80) != XCB_CLIENT_MESSAGE) {
            break;
        }

        xcb_client_message_event_t* clientmessage = (xcb_client_message_event_t*) event;

        if (clientmessage->type != im->atoms[XIM_ATOM_XIM_PROTOCOL]) {
            break;
        }

        xcb_im_client_table_t* client = NULL;
        HASH_FIND(hh2, im->clients_by_win, &clientmessage->window, sizeof(xcb_window_t), client);
        if (!client) {
            break;
        }

        bool del = true;
        xcb_im_proto_header_t hdr;
        uint8_t* message = _xcb_im_read_message(im, clientmessage, client, &hdr);
        if (message) {
            _xcb_im_handle_message(im, client, &hdr, message, &del);
            if (del) {
                free(message);
            }
        }

        return true;

    } while(0);

    return false;
}

bool xcb_im_filter_event(xcb_im_t* im, xcb_generic_event_t* event)
{
    return _xcb_im_filter_xconnect_message(im, event)
        || _xcb_im_filter_selection_request(im, event)
        || _xcb_im_filter_client(im, event);
}

void xcb_im_close_im(xcb_im_t* im)
{
    xcb_atom_t* atoms = im->atoms;
    xcb_get_property_cookie_t cookie = xcb_get_property(im->conn,
                                                        false,
                                                        im->root,
                                                        atoms[XIM_ATOM_XIM_SERVERS],
                                                        XCB_ATOM_ATOM,
                                                        0L,
                                                        1000000L);

    xcb_get_property_reply_t* reply = xcb_get_property_reply(im->conn, cookie, NULL);
    bool found = false;
    do {
        if (!reply) {
            break;
        }
        if (reply->type != XCB_ATOM_NONE && (reply->type != XCB_ATOM_ATOM || reply->format != 32)) {
            break;
        }
        long* data = xcb_get_property_value(reply);
        if (!data) {
            break;
        }
        uint32_t length = xcb_get_property_value_length(reply) / sizeof(uint32_t);
        uint32_t i;
        for (i = 0; i < length; i++) {
            if (data[i] == atoms[XIM_ATOM_SERVER_NAME]) {
                found = true;
                break;
            }
        }

        if (found) {
            for (i = i + 1; i < length; i++) {
                data[i - 1] = data[i];
            }
            xcb_change_property(im->conn,
                                XCB_PROP_MODE_REPLACE,
                                im->root,
                                atoms[XIM_ATOM_XIM_SERVERS],
                                XCB_ATOM_ATOM,
                                32,
                                length - 1,
                                data);
        } else {
            /*
            * We always need to generate the PropertyNotify to the Root Window
            */
            xcb_change_property(im->conn,
                                XCB_PROP_MODE_PREPEND,
                                im->root,
                                atoms[XIM_ATOM_XIM_SERVERS],
                                XCB_ATOM_ATOM,
                                32,
                                0,
                                data);
        }
    } while(0);
    free(reply);
    while (im->clients_by_win) {
        HASH_DELETE(hh2, im->clients_by_win, im->clients_by_win);
    }
    while (im->clients_by_id) {
        xcb_im_client_table_t* client = im->clients_by_id;
        HASH_DELETE(hh1, im->clients_by_id, im->clients_by_id);
        xcb_destroy_window(im->conn, client->c.accept_win);
        free(client);
    }
    im->connect_id = 0;
}

void xcb_im_destory(xcb_im_t* im)
{
    _free_encodings(&im->encodings);
    _free_trigger_keys(&im->onKeys);
    _free_trigger_keys(&im->offKeys);
    _free_input_styles(&im->inputStyles);
    free(im);
}

void xcb_im_forward_event(xcb_im_t* im, xcb_key_press_event_t* event)
{
}


void xcb_im_comming_string(xcb_im_t* im)
{
}

void xcb_im_preedit_start(xcb_im_t* im)
{
}

void xcb_im_preedit_end(xcb_im_t* im)
{
}

void xcb_im_sync_xlib(xcb_im_t* im)
{
}
