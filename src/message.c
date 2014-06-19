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

