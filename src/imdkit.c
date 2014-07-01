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
#include "common.h"

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
                    xcb_im_ximtriggerkey_fr_t,
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

    if (!event_mask) {
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
        im->icattr[i].attribute_ID = id;
        im->id2preeditoffset[id] = -1;
        im->id2statusoffset[id] = -1;
        im->id2icoffset[id] = -1;
        // this is comparing two constant string so it should be fast
        if (strcmp(Default_ICattr[i].name, XNPreeditAttributes) == 0) {
            im->preeditAttr_id = im->icattr[i].attribute_ID;
        } else if (strcmp(Default_ICattr[i].name, XNStatusAttributes) == 0) {
            im->statusAttr_id = im->icattr[i].attribute_ID;
        } else if (strcmp(Default_ICattr[i].name, XNSeparatorofNestedList) == 0) {
            im->separatorAttr_id = im->icattr[i].attribute_ID;
        } else if (strcmp(Default_ICattr[i].name, XNArea) == 0) {
            im->id2preeditoffset[id] = offsetof(xcb_im_preedit_attr_t, area);
            im->id2statusoffset[id] = offsetof(xcb_im_preedit_attr_t, area);
        } else if (strcmp(Default_ICattr[i].name, XNAreaNeeded) == 0) {
            im->id2preeditoffset[id] = offsetof(xcb_im_preedit_attr_t, area_needed);
            im->id2statusoffset[id] = offsetof(xcb_im_preedit_attr_t, area_needed);
        } else if (strcmp(Default_ICattr[i].name, XNAreaNeeded) == 0) {
            im->id2preeditoffset[id] = offsetof(xcb_im_preedit_attr_t, area_needed);
            im->id2statusoffset[id] = offsetof(xcb_im_preedit_attr_t, area_needed);
        } else if (strcmp(Default_ICattr[i].name, XNSpotLocation) == 0) {
            im->id2preeditoffset[id] = offsetof(xcb_im_preedit_attr_t, spot_location);
        } else if (strcmp(Default_ICattr[i].name, XNColormap) == 0) {
            im->id2preeditoffset[id] = offsetof(xcb_im_preedit_attr_t, colormap);
            im->id2statusoffset[id] = offsetof(xcb_im_preedit_attr_t, colormap);
        } else if (strcmp(Default_ICattr[i].name, XNStdColormap) == 0) {
            im->id2preeditoffset[id] = offsetof(xcb_im_preedit_attr_t, colormap);
            im->id2statusoffset[id] = offsetof(xcb_im_preedit_attr_t, colormap);
        } else if (strcmp(Default_ICattr[i].name, XNForeground) == 0) {
            im->id2preeditoffset[id] = offsetof(xcb_im_preedit_attr_t, foreground);
            im->id2statusoffset[id] = offsetof(xcb_im_preedit_attr_t, foreground);
        } else if (strcmp(Default_ICattr[i].name, XNBackground) == 0) {
            im->id2preeditoffset[id] = offsetof(xcb_im_preedit_attr_t, background);
            im->id2statusoffset[id] = offsetof(xcb_im_preedit_attr_t, background);
        } else if (strcmp(Default_ICattr[i].name, XNBackgroundPixmap) == 0) {
            im->id2preeditoffset[id] = offsetof(xcb_im_preedit_attr_t, bg_pixmap);
            im->id2statusoffset[id] = offsetof(xcb_im_preedit_attr_t, bg_pixmap);
        } else if (strcmp(Default_ICattr[i].name, XNLineSpace) == 0) {
            im->id2preeditoffset[id] = offsetof(xcb_im_preedit_attr_t, line_space);
            im->id2statusoffset[id] = offsetof(xcb_im_preedit_attr_t, line_space);
        } else if (strcmp(Default_ICattr[i].name, XNClientWindow) == 0) {
            im->id2icoffset[id] = offsetof(xcb_im_input_context_t, client_win);
        } else if (strcmp(Default_ICattr[i].name, XNInputStyle) == 0) {
            im->id2icoffset[id] = offsetof(xcb_im_input_context_t, input_style);
        } else if (strcmp(Default_ICattr[i].name, XNFocusWindow) == 0) {
            im->id2icoffset[id] = offsetof(xcb_im_input_context_t, focus_win);
        }
        im->id2attr[id] = (xcb_im_ximattr_fr_t*) &im->icattr[i];
        id++;
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
    im->init = _xcb_im_init_atoms(im->conn, ARRAY_SIZE(atom_names), atom_names, im->atoms);
    free(buf);

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
        // save as a cache, since we want to memset the item from free list
        new_connect_id = client->c.connect_id;
        xcb_im_client_table_t* dup = NULL;
        HASH_FIND(hh1, im->clients_by_id, &new_connect_id, sizeof(uint16_t), dup);
        if (dup) {
            DebugLog("Bug in implementation");
            return NULL;
        }
        memset(client, 0, sizeof(xcb_im_client_table_t));
        client->c.connect_id = new_connect_id;
        im->free_list = im->free_list->hh1.next;
    } else {
        while (new_connect_id == 0) {
            new_connect_id = ++im->connect_id;
        }
        xcb_im_client_table_t* dup = NULL;
        HASH_FIND(hh1, im->clients_by_id, &new_connect_id, sizeof(uint16_t), dup);
        if (dup) {
            DebugLog("overflow! too many clients");
            return NULL;
        }
        client = calloc(1, sizeof(xcb_im_client_table_t));
        client->c.connect_id = new_connect_id;
    }

    list_init(&client->queue);

    xcb_window_t w = xcb_generate_id (im->conn);
    xcb_im_client_table_t* dup = NULL;
    HASH_FIND(hh2, im->clients_by_win, &w, sizeof(xcb_window_t), dup);
    if (dup) {
        DebugLog("duplicate XID, Bug in XCB?");
        client->hh1.next = im->free_list;
        im->free_list = client;
        return NULL;
    }

    xcb_create_window (im->conn, XCB_COPY_FROM_PARENT, w, im->root,
                       0, 0, 1, 1, 1,
                       XCB_WINDOW_CLASS_INPUT_OUTPUT,
                       im->screen->root_visual,
                       0, NULL);

    client->c.client_win = client_window;
    client->c.accept_win = w;
    client->c.byte_order = '?'; // initial value
    HASH_ADD(hh1, im->clients_by_id, c.connect_id, sizeof(int), client);
    HASH_ADD(hh2, im->clients_by_win, c.accept_win, sizeof(xcb_window_t), client);


    _xcb_change_event_mask(im->conn, client_window, XCB_EVENT_MASK_STRUCTURE_NOTIFY, false);

    return client;
}

xcb_im_input_context_table_t* _xcb_im_new_input_context(xcb_im_t* im,
                                                        xcb_im_client_table_t* client)
{
    uint16_t icid = 0;
    xcb_im_input_context_table_t* ic = NULL;
    if (client->ic_free_list) {
        ic = client->ic_free_list;
        icid = ic->ic.id;
        xcb_im_input_context_table_t* dup = NULL;
        HASH_FIND(hh, client->input_contexts, &icid, sizeof(uint16_t), dup);
        if (dup) {
            DebugLog("Bug in implementation");
            return NULL;
        }

        client->ic_free_list = client->ic_free_list->hh.next;
        memset(ic, 0, sizeof(xcb_im_input_context_table_t));
        ic->ic.id = icid;
    } else {
        while (icid == 0) {
            icid = ++client->icid;
        }
        xcb_im_input_context_table_t* dup = NULL;
        HASH_FIND(hh, client->input_contexts, &icid, sizeof(uint16_t), dup);
        if (dup) {
            DebugLog("overflow! too many clients");
            return NULL;
        }

        ic = calloc(1, sizeof(xcb_im_input_context_table_t));
        if (!ic) {
            return NULL;
        }
        ic->ic.id = icid;
    }

    ic->ic.client = &client->c;
    HASH_ADD(hh, client->input_contexts, ic.id, sizeof(uint16_t), ic);
    return ic;
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
        if (!client) {
            break;
        }
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
        ev.data.data32[3] = XIM_CM_DATA_SIZE;
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
                                     xcb_im_packet_header_fr_t* hdr)
{
    if (ev->format == 8) {
        if (client->c.byte_order == '?') {
            // major_opcode
            if (ev->data.data8[0] != XIM_CONNECT) {
                return NULL;  /* can do nothing */
            }
            client->c.byte_order = ev->data.data8[XCB_IM_HEADER_SIZE];
        }
    }
    return _xcb_read_xim_message(im->conn, client->c.accept_win, ev, hdr, client->c.byte_order != im->byte_order);
}

void _xcb_im_handle_message(xcb_im_t* im,
                            xcb_im_client_table_t* client,
                            const xcb_im_packet_header_fr_t* hdr,
                            uint8_t* data)
{
    switch (hdr->major_opcode) {
    case XIM_CONNECT:
        DebugLog("-- XIM_CONNECT\n");
        _xcb_im_handle_connect(im, client, hdr, data);
        break;

    case XIM_DISCONNECT:
        DebugLog("-- XIM_DISCONNECT\n");
        _xcb_im_handle_disconnect(im, client, hdr, data);
        break;

    case XIM_OPEN:
        DebugLog("-- XIM_OPEN\n");
        _xcb_im_handle_open(im, client, hdr, data);
        break;

    case XIM_CLOSE:
        DebugLog("-- XIM_CLOSE\n");
        _xcb_im_handle_close(im, client, hdr, data);
        break;

    case XIM_QUERY_EXTENSION:
        DebugLog("-- XIM_QUERY_EXTENSION\n");
        _xcb_im_handle_query_extension(im, client, hdr, data);
        break;

    case XIM_GET_IM_VALUES:
        DebugLog("-- XIM_GET_IM_VALUES\n");
        _xcb_im_handle_get_im_values(im, client, hdr, data);
        break;

    case XIM_CREATE_IC:
        DebugLog("-- XIM_CREATE_IC\n");
        _xcb_im_handle_create_ic(im, client, hdr, data);
        break;

    case XIM_SET_IC_VALUES:
        DebugLog("-- XIM_SET_IC_VALUES\n");
        _xcb_im_handle_set_ic_values(im, client, hdr, data);
        break;

    case XIM_GET_IC_VALUES:
        DebugLog("-- XIM_GET_IC_VALUES\n");
        _xcb_im_handle_get_ic_values(im, client, hdr, data);
        break;

    case XIM_SET_IC_FOCUS:
        DebugLog("-- XIM_SET_IC_FOCUS\n");
        _xcb_im_handle_set_ic_focus(im, client, hdr, data);
        break;

    case XIM_UNSET_IC_FOCUS:
        DebugLog("-- XIM_UNSET_IC_FOCUS\n");
        _xcb_im_handle_unset_ic_focus(im, client, hdr, data);
        break;

    case XIM_DESTROY_IC:
        DebugLog("-- XIM_DESTROY_IC\n");
        _xcb_im_handle_destroy_ic(im, client, hdr, data);
        break;

    case XIM_RESET_IC:
        DebugLog("-- XIM_RESET_IC\n");
        _xcb_im_handle_reset_ic(im, client, hdr, data);
        break;

    case XIM_FORWARD_EVENT:
        DebugLog("-- XIM_FORWARD_EVENT\n");
        _xcb_im_handle_forward_event(im, client, hdr, data);
        break;

    case XIM_EXTENSION:
        DebugLog("-- XIM_EXTENSION\n");
        _xcb_im_handle_extension(im, client, hdr, data);
        break;

    case XIM_SYNC:
        DebugLog("-- XIM_SYNC\n");
        // nothing is needed here
        break;

    case XIM_SYNC_REPLY:
        DebugLog("-- XIM_SYNC_REPLY\n");
        _xcb_im_handle_sync_reply(im, client, hdr, data);
        break;

    case XIM_TRIGGER_NOTIFY:
        DebugLog("-- XIM_TRIGGER_NOTIFY\n");
        _xcb_im_handle_trigger_notify(im, client, hdr, data);
        break;

    case XIM_ENCODING_NEGOTIATION:
        DebugLog("-- XIM_ENCODING_NEGOTIATION\n");
        _xcb_im_handle_encoding_negotiation(im, client, hdr, data);
        break;

    case XIM_PREEDIT_START_REPLY:
        DebugLog("-- XIM_PREEDIT_START_REPLY\n");
        _xcb_im_handle_preedit_start_reply(im, client, hdr, data);
        break;

    case XIM_PREEDIT_CARET_REPLY:
        DebugLog("-- XIM_PREEDIT_CARET_REPLY\n");
        _xcb_im_handle_preedit_caret_reply(im, client, hdr, data);
        break;

    case XIM_STR_CONVERSION_REPLY:
        DebugLog("-- XIM_STR_CONVERSION_REPLY\n");
        // we don't care this one.
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

        xcb_im_packet_header_fr_t hdr;
        uint8_t* message = _xcb_im_read_message(im, clientmessage, client, &hdr);
        if (message) {
            _xcb_im_handle_message(im, client, &hdr, message);
            free(message);
        }

        return true;

    } while(0);

    return false;
}

bool _xcb_im_filter_destroy_window(xcb_im_t* im, xcb_generic_event_t* event)
{
    do {
        if ((event->response_type & ~0x80) != XCB_DESTROY_NOTIFY) {
            break;
        }

        xcb_destroy_notify_event_t* destroy_notify = (xcb_destroy_notify_event_t*) event;

        xcb_im_client_table_t* client = im->clients_by_win;
        while (client && client->c.client_win != destroy_notify->window) {
            client = client->hh1.next;
        }
        if (!client) {
            break;
        }

        _xcb_im_destroy_client(im, client);

        return true;
    } while(0);

    return false;
}

bool xcb_im_filter_event(xcb_im_t* im, xcb_generic_event_t* event)
{
    return _xcb_im_filter_xconnect_message(im, event)
        || _xcb_im_filter_selection_request(im, event)
        || _xcb_im_filter_client(im, event)
        || _xcb_im_filter_destroy_window(im, event);
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
        uint32_t* data = xcb_get_property_value(reply);
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
    while (im->clients_by_id) {
        _xcb_im_destroy_client(im, im->clients_by_id);
    }

    while (im->free_list) {
        xcb_im_client_table_t* p = im->free_list;
        // TODO: might need to free more?
        im->free_list = im->free_list->hh1.next;
        free(p);
    }
    im->sync = false;
    im->connect_id = 0;
}

void xcb_im_destroy(xcb_im_t* im)
{
    free(im->locale);
    free(im->serverName);
    _free_encodings(&im->encodings);
    _free_trigger_keys(&im->onKeys);
    _free_trigger_keys(&im->offKeys);
    _free_input_styles(&im->inputStyles);
    free(im);
}

void xcb_im_forward_event(xcb_im_t* im, xcb_im_input_context_t* ic, xcb_key_press_event_t* event)
{
    xcb_im_forward_event_fr_t frame;
    frame.input_method_ID = ic->client->connect_id;
    frame.input_context_ID = ic->id;
    frame.flag = XimSYNCHRONUS;
    frame.sequence_number = event->sequence;
    xcb_im_client_table_t* client = (xcb_im_client_table_t*) ic->client;
    client->c.sync = true;

    const size_t length = 8 /* xcb_im_forward_event_fr_size(&frame) */ + sizeof(xcb_key_press_event_t);
    uint8_t data[XCB_IM_HEADER_SIZE + length];
    _xcb_write_xim_message_header(data, XIM_FORWARD_EVENT, 0, length, client->c.byte_order != im->byte_order);
    uint8_t* p = xcb_im_forward_event_fr_write(&frame, data + XCB_IM_HEADER_SIZE, client->c.byte_order != im->byte_order);
    memcpy(p, event, sizeof(xcb_key_press_event_t));

    _xcb_im_send_message(im, client, data, length);
}


void xcb_im_commit_string(xcb_im_t* im, xcb_im_input_context_t* ic, uint32_t flag, char* str, uint32_t length, uint32_t keysym)
{
    if (!(flag & XimLookupKeySym) && (flag & XimLookupChars)) {
        xcb_im_commit_chars_fr_t frame;
        frame.input_method_ID = ic->client->connect_id;
        frame.input_context_ID = ic->id;
        frame.byte_length_of_committed_string = length;
        frame.flag = flag | XimSYNCHRONUS;
        frame.committed_string = (uint8_t*) str;
        _xcb_im_send_frame(im, (xcb_im_client_table_t*) ic->client, frame, false);
    } else {
        xcb_im_commit_both_fr_t frame;
        frame.input_method_ID = ic->client->connect_id;
        frame.input_context_ID = ic->id;
        frame.byte_length_of_committed_string = length;
        frame.flag = flag | XimSYNCHRONUS;
        frame.committed_string = (uint8_t*) str;
        frame.keysym = keysym;
        _xcb_im_send_frame(im, (xcb_im_client_table_t*) ic->client, frame, false);
    }
}

void xcb_im_preedit_start(xcb_im_t* im, xcb_im_input_context_t* ic)
{
    if (im->onKeys.nKeys == 0  &&  im->onKeys.nKeys == 0) {
        return;
    }

    _xcb_im_set_event_mask(im, (xcb_im_client_table_t*) ic->client, ic->id, im->event_mask, ~im->event_mask);
}

void xcb_im_preedit_end(xcb_im_t* im, xcb_im_input_context_t* ic)
{
    if (im->onKeys.nKeys == 0  &&  im->onKeys.nKeys == 0) {
        return;
    }

    _xcb_im_set_event_mask(im, (xcb_im_client_table_t*) ic->client, ic->id, 0, 0);
}

void xcb_im_sync_xlib(xcb_im_t* im, xcb_im_input_context_t* ic)
{
    im->sync = true;
    xcb_im_sync_fr_t frame;
    frame.input_method_ID = ic->client->connect_id;
    frame.input_context_ID = ic->id;

    _xcb_im_send_frame(im, (xcb_im_client_table_t*) ic->client, frame, false);
}

bool _xcb_im_get_input_styles_attr(xcb_im_t* im, xcb_im_client_table_t* client, xcb_im_ximattribute_fr_t* attr)
{
    xcb_im_input_styles_fr_t fr;
    memset(&fr, 0, sizeof(fr));
    fr.XIMStyle_list.size = im->inputStyles.nStyles;
    if (im->inputStyles.nStyles) {
        if ((fr.XIMStyle_list.items = calloc(im->inputStyles.nStyles, sizeof(xcb_im_inputstyle_fr_t))) == NULL) {
            return false;
        }
        for (size_t j = 0; j < im->inputStyles.nStyles; j ++) {
            fr.XIMStyle_list.items[j].inputstyle = im->inputStyles.styles[j];
        }
    }

    size_t frame_size = xcb_im_input_styles_fr_size(&fr);
    if ((attr->value = malloc(frame_size)) != NULL) {;
        attr->value_length = frame_size;
        xcb_im_input_styles_fr_write(&fr, attr->value, client->c.byte_order != im->byte_order);
    }
    xcb_im_input_styles_fr_free(&fr);
    return attr->value != NULL;
}

const xcb_im_default_ic_attr_t* _xcb_im_default_ic_attr_entry(xcb_im_t* im,
                                                              uint32_t id)
{
    if (id >= ARRAY_SIZE(im->id2attr)) {
        return NULL;
    }
    xcb_im_xicattr_fr_t* attr = (xcb_im_xicattr_fr_t*) im->id2attr[id];
    if ((attr < im->icattr) || (attr >= im->icattr + ARRAY_SIZE(Default_ICattr))) {
        return NULL;
    }

    return &Default_ICattr[attr - im->icattr];
}

void _xcb_im_destroy_ic(xcb_im_t* im,
                        xcb_im_input_context_table_t* ic) {

    xcb_im_client_table_t* client = (xcb_im_client_table_t*) ic->ic.client;

    if (im->callback) {
        xcb_im_packet_header_fr_t hdr;
        hdr.length = 0;
        hdr.major_opcode = XIM_DESTROY_IC;
        hdr.minor_opcode = 0;
        im->callback(im, ic->ic.client, &ic->ic, &hdr, NULL, NULL, im->user_data);
    }

    // Destroy ic
    HASH_DEL(client->input_contexts, ic);
    ic->hh.next = client->ic_free_list;
    client->ic_free_list = ic;
}

void _xcb_im_destroy_client(xcb_im_t* im,
                            xcb_im_client_table_t* client)
{
    xcb_im_packet_header_fr_t hdr;
    hdr.length = 0;
    hdr.major_opcode = XIM_DISCONNECT;
    hdr.minor_opcode = 0;

    while (client->input_contexts) {
        _xcb_im_destroy_ic(im, client->input_contexts);
    }

    if (im->callback) {
        im->callback(im, &client->c, NULL, &hdr, NULL, NULL, im->user_data);
    }

    list_entry_foreach_safe(item, xcb_im_queue_t, &client->queue, list) {
        free(item);
    }

    HASH_DELETE(hh2, im->clients_by_win, client);
    HASH_DELETE(hh1, im->clients_by_id, client);
    xcb_destroy_window(im->conn, client->c.accept_win);

    while (client->ic_free_list) {
        xcb_im_input_context_table_t* p = client->ic_free_list;
        // TODO: mind need to free more?
        client->ic_free_list = client->ic_free_list->hh.next;
        free(p);
    }

    client->hh1.next = im->free_list;
    im->free_list = client;
}


void _xcb_im_set_event_mask(xcb_im_t* im, xcb_im_client_table_t* client, uint32_t icid, uint32_t forward_event_mask, uint32_t sync_mask)
{
    xcb_im_set_event_mask_fr_t frame;
    frame.forward_event_mask = forward_event_mask;
    frame.synchronous_event_mask = sync_mask;
    frame.input_method_ID = client->c.connect_id;
    frame.input_context_ID = icid;
    _xcb_im_send_frame(im, client, frame, false);
}

void _xcb_im_add_queue(xcb_im_t* im, xcb_im_client_table_t* client, uint16_t icid, const xcb_im_packet_header_fr_t* hdr, xcb_im_forward_event_fr_t* frame, uint8_t* data)
{
    xcb_im_queue_t* item = malloc(sizeof(xcb_im_queue_t));
    if (!item) {
        return;
    }

    item->icid = icid;
    memcpy(&item->event, data, sizeof(xcb_key_press_event_t));
    memcpy(&item->hdr, hdr, sizeof(xcb_im_packet_header_fr_t));
    memcpy(&item->frame, frame, sizeof(xcb_im_forward_event_fr_t));


    list_append(&item->list, &client->queue);
}

void _xcb_im_process_queue(xcb_im_t* im, xcb_im_client_table_t* client)
{
    while (!client->c.sync && !list_is_empty(&client->queue))
    {
        xcb_im_queue_t* item = list_container_of(client->queue.next, xcb_im_queue_t, list);
        list_remove(&item->list);

        xcb_im_input_context_table_t* ic = NULL;
        HASH_FIND(hh, client->input_contexts, &item->icid, sizeof(uint16_t), ic);
        if (ic) {
            if (im->callback) {
                im->callback(im, &client->c, &ic->ic, &item->hdr, &item->frame, &item->event, im->user_data);
            }
        }
        free(item);
    }
}

void xcb_im_geometry_callback(xcb_im_t* im, xcb_im_input_context_t* ic)
{
    xcb_im_geometry_fr_t frame;
    frame.input_method_ID = ic->client->connect_id;
    frame.input_context_ID = ic->id;

    _xcb_im_send_frame(im, (xcb_im_client_table_t*) ic->client, frame, false);
}

void xcb_im_preedit_start_callback(xcb_im_t* im, xcb_im_input_context_t* ic)
{
    xcb_im_preedit_start_fr_t frame;
    frame.input_method_ID = ic->client->connect_id;
    frame.input_context_ID = ic->id;

    _xcb_im_send_frame(im, (xcb_im_client_table_t*) ic->client, frame, false);
}

void xcb_im_preedit_draw_callback(xcb_im_t* im, xcb_im_input_context_t* ic, xcb_im_preedit_draw_fr_t* frame)
{
    frame->input_method_ID = ic->client->connect_id;
    frame->input_context_ID = ic->id;

    _xcb_im_send_frame(im, (xcb_im_client_table_t*) ic->client, *frame, false);
}

void xcb_im_preedit_caret_callback(xcb_im_t* im, xcb_im_input_context_t* ic, xcb_im_preedit_caret_fr_t* frame)
{
    frame->input_method_ID = ic->client->connect_id;
    frame->input_context_ID = ic->id;

    _xcb_im_send_frame(im, (xcb_im_client_table_t*) ic->client, *frame, false);
}

void xcb_im_preedit_done_callback(xcb_im_t* im, xcb_im_input_context_t* ic)
{
    xcb_im_preedit_done_fr_t frame;
    frame.input_method_ID = ic->client->connect_id;
    frame.input_context_ID = ic->id;

    _xcb_im_send_frame(im, (xcb_im_client_table_t*) ic->client, frame, false);
}

void xcb_im_status_start_callback(xcb_im_t* im, xcb_im_input_context_t* ic)
{
    xcb_im_status_start_fr_t frame;
    frame.input_method_ID = ic->client->connect_id;
    frame.input_context_ID = ic->id;

    _xcb_im_send_frame(im, (xcb_im_client_table_t*) ic->client, frame, false);
}

void xcb_im_status_done_callback(xcb_im_t* im, xcb_im_input_context_t* ic)
{
    xcb_im_status_done_fr_t frame;
    frame.input_method_ID = ic->client->connect_id;
    frame.input_context_ID = ic->id;

    _xcb_im_send_frame(im, (xcb_im_client_table_t*) ic->client, frame, false);
}

void xcb_im_status_draw_text_callback(xcb_im_t* im, xcb_im_input_context_t* ic, xcb_im_status_draw_text_fr_t* frame)
{
    frame->input_method_ID = ic->client->connect_id;
    frame->input_context_ID = ic->id;
    frame->type = XCB_IM_TextType;

    _xcb_im_send_frame(im, (xcb_im_client_table_t*) ic->client, *frame, false);
}

void xcb_im_status_draw_bitmap_callback(xcb_im_t* im, xcb_im_input_context_t* ic, xcb_im_status_draw_bitmap_fr_t* frame)
{
    frame->input_method_ID = ic->client->connect_id;
    frame->input_context_ID = ic->id;
    frame->type = XCB_IM_BitmapType;

    _xcb_im_send_frame(im, (xcb_im_client_table_t*) ic->client, *frame, false);
}

// length is the body without header size in byte
bool _xcb_im_send_message(xcb_im_t* im,
                          xcb_im_client_table_t* client,
                          uint8_t* data, size_t length)
{
    char atomName[64];
    int len = sprintf(atomName, "_server%u_%u", client->c.connect_id, im->sequence++);
    return _xcb_send_xim_message(im->conn, im->atoms[XIM_ATOM_XIM_PROTOCOL], client->c.client_win,
                                 data, length, atomName, len);
}

void _xcb_im_send_error_message(xcb_im_t* im,
                                xcb_im_client_table_t* client)
{
    _xcb_send_xim_error_message(im->conn, im->atoms[XIM_ATOM_XIM_PROTOCOL], client->c.client_win, im->byte_order != client->c.byte_order);
}

bool xcb_im_support_extension(xcb_im_t* im, uint16_t major_code, uint16_t minor_code)
{
    for (size_t i = 0 ; i < ARRAY_SIZE(im->extension); i++) {
        if (im->extension[i].extension_major_opcode == major_code
         && im->extension[i].extension_minor_opcode == minor_code) {
            return true;
        }
    }
    return false;
}

