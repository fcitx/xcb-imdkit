#include <stdlib.h>
#include <string.h>
#include <ximproto.h>
#include "message.h"
#include "common.h"

uint8_t* _xcb_new_xim_message(uint8_t major_opcode,
                             uint8_t minor_opcode,
                             size_t length,
                             bool swap)
{
    uint8_t* message = calloc(length + XCB_IM_HEADER_SIZE, 1);
    if (message) {
        _xcb_write_xim_message_header(message, major_opcode, minor_opcode, length, swap);
    }

    return message;
}

void _xcb_write_xim_message_header(uint8_t* message,
                                  uint8_t major_opcode,
                                  uint8_t minor_opcode,
                                  size_t length,
                                  bool swap)
{
    uint16_t p_len = length / 4;
    message = uint8_t_write(&major_opcode, message, swap);
    message = uint8_t_write(&minor_opcode, message, swap);
    message = uint16_t_write(&p_len, message, swap);
}

// length is the body without header size in byte
bool _xcb_send_xim_message(xcb_connection_t* conn,
                           xcb_atom_t protocol_atom,
                           xcb_window_t window,
                           uint8_t* data, size_t length,
                           const char* name, size_t len)
{
    if (!data) {
        return false;
    }

    // add header size
    length += XCB_IM_HEADER_SIZE;

    xcb_client_message_event_t event;

    event.response_type = XCB_CLIENT_MESSAGE;
    event.sequence = 0;
    event.window = window;
    event.type = protocol_atom;

    if (length > XIM_CM_DATA_SIZE) {
        xcb_atom_t atom;

        xcb_intern_atom_cookie_t atom_cookie = xcb_intern_atom(conn, false, len, name);
        xcb_intern_atom_reply_t* atom_reply = xcb_intern_atom_reply(conn, atom_cookie, NULL);
        if (!atom_reply) {
            return false;
        }
        atom = atom_reply->atom;
        free(atom_reply);
        xcb_get_property_cookie_t get_property_cookie = xcb_get_property(conn,
                                                                         false,
                                                                         window,
                                                                         atom,
                                                                         XCB_ATOM_STRING,
                                                                         0L,
                                                                         10000L);

        xcb_get_property_reply_t* get_property_reply = xcb_get_property_reply(conn, get_property_cookie, NULL);
        if (!get_property_reply) {
            return false;
        }
        free(get_property_reply);
        xcb_void_cookie_t cookie = xcb_change_property_checked(conn,
                                            XCB_PROP_MODE_APPEND,
                                            window,
                                            atom,
                                            XCB_ATOM_STRING,
                                            8,
                                            length,
                                            data);
        xcb_generic_error_t* error = NULL;
        if ((error = xcb_request_check(conn, cookie)) != NULL) {
            DebugLog("Error code: %d", error->error_code);
            free(error);
        }
        event.format = 32;
        event.data.data32[0] = length;
        event.data.data32[1] = atom;
        for (size_t i = 2; i < ARRAY_SIZE(event.data.data32); i++)
            event.data.data32[i] = 0;
    } else {
        event.format = 8;

        memcpy(event.data.data8, data, length);
        /* Clear unused field with NULL */
        for (size_t i = length; i < XIM_CM_DATA_SIZE; i++)
            event.data.data8[i] = 0;
    }
    xcb_send_event(conn, false, window, XCB_EVENT_MASK_NO_EVENT, (const char*) &event);
    xcb_flush(conn);
    return true;
}

void _xcb_send_xim_error_message(xcb_connection_t* conn,
                                 xcb_atom_t protocol_atom,
                                 xcb_window_t window,
                                 bool swap)
{
    // use stack to avoid alloc fails
    uint8_t message[XCB_IM_HEADER_SIZE];
    _xcb_write_xim_message_header(message, XIM_ERROR, 0, 0, swap);
    _xcb_send_xim_message(conn, protocol_atom, window, message, 0, NULL, 0);
}
