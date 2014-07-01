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

#ifndef PROTOCOLHANDLER_H
#define PROTOCOLHANDLER_H

#include "imdkit.h"
#include "imdkit_p.h"

void _xcb_im_handle_connect(xcb_im_t* im,
                            xcb_im_client_table_t* client,
                            const xcb_im_packet_header_fr_t* hdr,
                            uint8_t* data);

void _xcb_im_handle_open(xcb_im_t* im,
                         xcb_im_client_table_t* client,
                         const xcb_im_packet_header_fr_t* hdr,
                         uint8_t* data);

void _xcb_im_handle_close(xcb_im_t* im,
                          xcb_im_client_table_t* client,
                          const xcb_im_packet_header_fr_t* hdr,
                          uint8_t* data);

void _xcb_im_handle_query_extension(xcb_im_t* im,
                                    xcb_im_client_table_t* client,
                                    const xcb_im_packet_header_fr_t* hdr,
                                    uint8_t* data);

void _xcb_im_handle_encoding_negotiation(xcb_im_t* im,
                                         xcb_im_client_table_t* client,
                                         const xcb_im_packet_header_fr_t* hdr,
                                         uint8_t* data);

void _xcb_im_handle_get_im_values(xcb_im_t* im,
                                  xcb_im_client_table_t* client,
                                  const xcb_im_packet_header_fr_t* hdr,
                                  uint8_t* data);

void _xcb_im_handle_disconnect(xcb_im_t* im,
                               xcb_im_client_table_t* client,
                               const xcb_im_packet_header_fr_t* hdr,
                               uint8_t* data);

void _xcb_im_handle_create_ic(xcb_im_t* im,
                              xcb_im_client_table_t* client,
                              const xcb_im_packet_header_fr_t* hdr,
                              uint8_t* data);

void _xcb_im_handle_destroy_ic(xcb_im_t* im,
                               xcb_im_client_table_t* client,
                               const xcb_im_packet_header_fr_t* hdr,
                               uint8_t* data);

void _xcb_im_handle_set_ic_values(xcb_im_t* im,
                                  xcb_im_client_table_t* client,
                                  const xcb_im_packet_header_fr_t* hdr,
                                  uint8_t* data);

void _xcb_im_handle_get_ic_values(xcb_im_t* im,
                                  xcb_im_client_table_t* client,
                                  const xcb_im_packet_header_fr_t* hdr,
                                  uint8_t* data);

void _xcb_im_handle_set_ic_focus(xcb_im_t* im,
                                 xcb_im_client_table_t* client,
                                 const xcb_im_packet_header_fr_t* hdr,
                                 uint8_t* data);

void _xcb_im_handle_unset_ic_focus(xcb_im_t* im,
                                   xcb_im_client_table_t* client,
                                   const xcb_im_packet_header_fr_t* hdr,
                                   uint8_t* data);

void _xcb_im_handle_preedit_caret_reply(xcb_im_t* im,
                                        xcb_im_client_table_t* client,
                                        const xcb_im_packet_header_fr_t* hdr,
                                        uint8_t* data);

void _xcb_im_handle_reset_ic(xcb_im_t* im,
                                        xcb_im_client_table_t* client,
                                        const xcb_im_packet_header_fr_t* hdr,
                                        uint8_t* data);

void _xcb_im_handle_forward_event(xcb_im_t* im,
                                        xcb_im_client_table_t* client,
                                        const xcb_im_packet_header_fr_t* hdr,
                                        uint8_t* data);

void _xcb_im_handle_extension(xcb_im_t* im,
                                        xcb_im_client_table_t* client,
                                        const xcb_im_packet_header_fr_t* hdr,
                                        uint8_t* data);

void _xcb_im_handle_sync_reply(xcb_im_t* im,
                                        xcb_im_client_table_t* client,
                                        const xcb_im_packet_header_fr_t* hdr,
                                        uint8_t* data);

void _xcb_im_handle_trigger_notify(xcb_im_t* im,
                                        xcb_im_client_table_t* client,
                                        const xcb_im_packet_header_fr_t* hdr,
                                        uint8_t* data);

void _xcb_im_handle_preedit_start_reply(xcb_im_t* im,
                                        xcb_im_client_table_t* client,
                                        const xcb_im_packet_header_fr_t* hdr,
                                        uint8_t* data);

#endif // PROTOCOLHANDLER_H
