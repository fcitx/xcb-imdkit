#ifndef PROTOCOLHANDLER_H
#define PROTOCOLHANDLER_H

#include "imdkit.h"
#include "imdkit_p.h"

void _xcb_im_handle_connect(xcb_im_t* im,
                            xcb_im_client_table_t* client,
                            const xcb_im_proto_header_t* hdr,
                            uint8_t* data);

void _xcb_im_handle_open(xcb_im_t* im,
                         xcb_im_client_table_t* client,
                         const xcb_im_proto_header_t* hdr,
                         uint8_t* data,
                         bool *del);

void _xcb_im_handle_close(xcb_im_t* im,
                          xcb_im_client_table_t* client,
                          const xcb_im_proto_header_t* hdr,
                          uint8_t* data,
                          bool *del);

void _xcb_im_handle_query_extension(xcb_im_t* im,
                                    xcb_im_client_table_t* client,
                                    const xcb_im_proto_header_t* hdr,
                                    uint8_t* data,
                                    bool *del);

void _xcb_im_handle_encoding_negotiation(xcb_im_t* im,
                                         xcb_im_client_table_t* client,
                                         const xcb_im_proto_header_t* hdr,
                                         uint8_t* data,
                                         bool *del);

void _xcb_im_handle_get_im_values(xcb_im_t* im,
                                  xcb_im_client_table_t* client,
                                  const xcb_im_proto_header_t* hdr,
                                  uint8_t* data,
                                  bool *del);

void _xcb_im_handle_disconnect(xcb_im_t* im,
                               xcb_im_client_table_t* client,
                               const xcb_im_proto_header_t* hdr,
                               uint8_t* data,
                               bool *del);

void _xcb_im_handle_create_ic(xcb_im_t* im,
                              xcb_im_client_table_t* client,
                              const xcb_im_proto_header_t* hdr,
                              uint8_t* data,
                              bool *del);

void _xcb_im_handle_destroy_ic(xcb_im_t* im,
                               xcb_im_client_table_t* client,
                               const xcb_im_proto_header_t* hdr,
                               uint8_t* data,
                               bool *del);

void _xcb_im_handle_set_ic_values(xcb_im_t* im,
                                  xcb_im_client_table_t* client,
                                  const xcb_im_proto_header_t* hdr,
                                  uint8_t* data,
                                  bool *del);

void _xcb_im_handle_get_ic_values(xcb_im_t* im,
                                  xcb_im_client_table_t* client,
                                  const xcb_im_proto_header_t* hdr,
                                  uint8_t* data,
                                  bool *del);

void _xcb_im_handle_set_ic_focus(xcb_im_t* im,
                                 xcb_im_client_table_t* client,
                                 const xcb_im_proto_header_t* hdr,
                                 uint8_t* data,
                                 bool *del);

void _xcb_im_handle_unset_ic_focus(xcb_im_t* im,
                                   xcb_im_client_table_t* client,
                                   const xcb_im_proto_header_t* hdr,
                                   uint8_t* data,
                                   bool *del);

void _xcb_im_handle_preedit_caret_reply(xcb_im_t* im,
                                        xcb_im_client_table_t* client,
                                        const xcb_im_proto_header_t* hdr,
                                        uint8_t* data,
                                        bool *del);

void _xcb_im_handle_reset_ic(xcb_im_t* im,
                                        xcb_im_client_table_t* client,
                                        const xcb_im_proto_header_t* hdr,
                                        uint8_t* data,
                                        bool *del);

void _xcb_im_handle_forward_event(xcb_im_t* im,
                                        xcb_im_client_table_t* client,
                                        const xcb_im_proto_header_t* hdr,
                                        uint8_t* data,
                                        bool *del);

void _xcb_im_handle_extension(xcb_im_t* im,
                                        xcb_im_client_table_t* client,
                                        const xcb_im_proto_header_t* hdr,
                                        uint8_t* data,
                                        bool *del);

void _xcb_im_handle_sync_reply(xcb_im_t* im,
                                        xcb_im_client_table_t* client,
                                        const xcb_im_proto_header_t* hdr,
                                        uint8_t* data,
                                        bool *del);

void _xcb_im_handle_trigger_notify(xcb_im_t* im,
                                        xcb_im_client_table_t* client,
                                        const xcb_im_proto_header_t* hdr,
                                        uint8_t* data,
                                        bool *del);

void _xcb_im_handle_preedit_start_reply(xcb_im_t* im,
                                        xcb_im_client_table_t* client,
                                        const xcb_im_proto_header_t* hdr,
                                        uint8_t* data,
                                        bool *del);

#endif // PROTOCOLHANDLER_H
