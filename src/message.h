#ifndef MESSAGE_H
#define MESSAGE_H

#include "imdkit_p.h"

uint8_t* _xcb_im_new_message(xcb_im_t* im,
                             xcb_im_client_table_t* client,
                             uint8_t major_opcode,
                             uint8_t minor_opcode,
                             size_t length);

void _xcb_im_write_message_header(xcb_im_t* im,
                                  xcb_im_client_table_t* client,
                                  uint8_t* message,
                                  uint8_t major_opcode,
                                  uint8_t minor_opcode,
                                  size_t length);

// length is the body without header size in byte
bool _xcb_im_send_message(xcb_im_t* im,
                          xcb_im_client_table_t* client,
                          uint8_t* data, size_t length);

void _xcb_im_send_error_message(xcb_im_t* im,
                                xcb_im_client_table_t* client);
#endif // MESSAGE_H
