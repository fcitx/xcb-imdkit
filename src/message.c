#include "message.h"

uint8_t* _xcb_im_new_message(xcb_im_t* im,
                             xcb_im_client_table_t* client,
                             uint8_t major_opcode,
                             uint8_t minor_opcode,
                             size_t length)
{
    uint8_t* message = calloc(length + XCB_IM_HEADER_SIZE, 1);
    if (message) {
        _xcb_im_write_message_header(im, client, message, major_opcode, minor_opcode, length);
    }

    return message;
}

void _xcb_im_write_message_header(xcb_im_t* im,
                                  xcb_im_client_table_t* client,
                                  uint8_t* message,
                                  uint8_t major_opcode,
                                  uint8_t minor_opcode,
                                  size_t length)
{
    uint16_t p_len = length / 4;
    message = uint8_t_write(&major_opcode, message, client->c.byte_order != im->byte_order);
    message = uint8_t_write(&minor_opcode, message, client->c.byte_order != im->byte_order);
    message = uint16_t_write(&p_len, message, client->c.byte_order != im->byte_order);
}

// length is the body without header size in byte
bool _xcb_im_send_message(xcb_im_t* im,
                          xcb_im_client_table_t* client,
                          uint8_t* data, size_t length)
{
    if (!data) {
        return false;
    }

    // add header size
    length += XCB_IM_HEADER_SIZE;

    xcb_client_message_event_t event;

    event.response_type = XCB_CLIENT_MESSAGE;
    event.sequence = 0;
    event.window = client->c.client_win;
    event.type = im->atoms[XIM_ATOM_XIM_PROTOCOL];

    if (length > XCM_DATA_LIMIT) {
        xcb_atom_t atom;
        char atomName[64];

        int len = sprintf(atomName, "_server%u_%u", client->c.connect_id, im->sequence++);

        xcb_intern_atom_cookie_t atom_cookie = xcb_intern_atom(im->conn, false, len, atomName);
        xcb_intern_atom_reply_t* atom_reply = xcb_intern_atom_reply(im->conn, atom_cookie, NULL);
        if (!atom_reply) {
            return false;
        }
        atom = atom_reply->atom;
        free(atom_reply);
        xcb_get_property_cookie_t get_property_cookie = xcb_get_property(im->conn,
                                                                         false,
                                                                         client->c.client_win,
                                                                         atom,
                                                                         XCB_ATOM_STRING,
                                                                         0L,
                                                                         10000L);

        xcb_get_property_reply_t* get_property_reply = xcb_get_property_reply(im->conn, get_property_cookie, NULL);
        if (!get_property_reply) {
            return false;
        }
        free(get_property_reply);
        xcb_void_cookie_t cookie = xcb_change_property_checked(im->conn,
                                            XCB_PROP_MODE_APPEND,
                                            client->c.client_win,
                                            atom,
                                            XCB_ATOM_STRING,
                                            8,
                                            length,
                                            data);
        xcb_generic_error_t* error = NULL;
        if ((error = xcb_request_check(im->conn, cookie)) != NULL) {
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
        for (size_t i = length; i < XCM_DATA_LIMIT; i++)
            event.data.data8[i] = 0;
    }
    xcb_send_event(im->conn, false, client->c.client_win, XCB_EVENT_MASK_NO_EVENT, (const char*) &event);
    xcb_flush(im->conn);
    return true;
}

void _xcb_im_send_error_message(xcb_im_t* im,
                                xcb_im_client_table_t* client)
{
    // use stack to avoid alloc fails
    uint8_t message[XCB_IM_HEADER_SIZE];
    _xcb_im_write_message_header(im, client, message, XIM_ERROR, 0, 0);
    _xcb_im_send_message(im, client, message, 0);
}
