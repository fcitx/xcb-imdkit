#ifndef CLIENTPROTOCOLHANDLER_H
#define CLIENTPROTOCOLHANDLER_H
#include "imclient.h"
#include "ximproto.h"

void _xcb_xim_handle_open_reply(xcb_xim_t* im, const xcb_im_packet_header_fr_t* hdr, uint8_t* data);
void _xcb_xim_handle_query_extension_reply(xcb_xim_t* im, const xcb_im_packet_header_fr_t* hdr, uint8_t* data);
void _xcb_xim_handle_encoding_negotiation_reply(xcb_xim_t* im, const xcb_im_packet_header_fr_t* hdr, uint8_t* data);
void _xcb_xim_handle_create_ic_reply(xcb_xim_t* im, const xcb_im_packet_header_fr_t* hdr, uint8_t* data);
void _xcb_xim_handle_destroy_ic_reply(xcb_xim_t* im, const xcb_im_packet_header_fr_t* hdr, uint8_t* data);
void _xcb_xim_handle_get_im_values_reply(xcb_xim_t* im, const xcb_im_packet_header_fr_t* hdr, uint8_t* data);
void _xcb_xim_handle_register_triggerkeys(xcb_xim_t* im, const xcb_im_packet_header_fr_t* hdr, uint8_t* data);
void _xcb_xim_handle_get_ic_values_reply(xcb_xim_t* im, const xcb_im_packet_header_fr_t* hdr, uint8_t* data);
void _xcb_xim_handle_set_ic_values_reply(xcb_xim_t* im, const xcb_im_packet_header_fr_t* hdr, uint8_t* data);

#endif // CLIENTPROTOCOLHANDLER_H
