#include "protocolhandler.h"
#include "message.h"

void _xcb_im_handle_connect(xcb_im_t* im,
                            xcb_im_client_table_t* client,
                            const xcb_im_proto_header_t* hdr,
                            uint8_t* data)
{
    connect_fr frame;
    _xcb_im_read_frame(im, client, frame);

    if (im->callback) {
        im->callback(im, &client->c, hdr, &frame, im->user_data);
    }
    connect_fr_free(&frame);

    connect_reply_fr reply_frame;
    reply_frame.server_major_protocol_version = frame.client_major_protocol_version;
    reply_frame.server_minor_protocol_version = frame.client_minor_protocol_version;

    _xcb_im_send_frame(im, client, reply_frame);
    return;
}

static void _xcb_im_send_trigger_key(xcb_im_t* im, xcb_im_client_table_t* client)
{
    register_triggerkeys_fr frame;
    /* Right now XIM_OPEN_REPLY hasn't been sent to this new client, so
       the input-method-id is still invalid, and should be set to zero...
       Reter to $(XC)/lib/X11/imDefLkup.c:_XimRegisterTriggerKeysCallback
     */
    frame.input_method_ID = 0;
    frame.on_keys_list.size = im->onKeys.nKeys;
    frame.on_keys_list.items = im->onKeys.keys;
    frame.off_keys_list.size = im->offKeys.nKeys;
    frame.off_keys_list.items = im->offKeys.keys;

    _xcb_im_send_frame(im, client, frame);
}

void _xcb_im_handle_open(xcb_im_t* im,
                         xcb_im_client_table_t* client,
                         const xcb_im_proto_header_t* hdr,
                         uint8_t* data,
                         bool *del)
{
    open_fr frame;
    _xcb_im_read_frame(im, client, frame);

    if (im->callback) {
        im->callback(im, &client->c, hdr, &frame, im->user_data);
    }

    open_fr_free(&frame);
    /*endif*/
    if (im->onKeys.nKeys || im->offKeys.nKeys) {
        _xcb_im_send_trigger_key(im, client);
    }

    open_reply_fr reply_frame;
    reply_frame.input_method_ID = client->c.connect_id;
    reply_frame.IM_attribute_supported.size = ARRAY_SIZE(im->imattr);
    reply_frame.IC_attribute_supported.size = ARRAY_SIZE(im->icattr);
    reply_frame.IM_attribute_supported.items = im->imattr;
    reply_frame.IC_attribute_supported.items = im->icattr;

    _xcb_im_send_frame(im, client, reply_frame);
}

void _xcb_im_handle_close(xcb_im_t* im,
                          xcb_im_client_table_t* client,
                          const xcb_im_proto_header_t* hdr,
                          uint8_t* data,
                          bool *del)
{
    close_fr frame;
    _xcb_im_read_frame(im, client, frame);

    if (im->callback) {
        im->callback(im, &client->c, hdr, &frame, im->user_data);
    }

    close_fr_free(&frame);
    close_reply_fr reply_frame;
    reply_frame.input_method_ID = frame.input_method_ID;
    _xcb_im_send_frame(im, client, reply_frame);
    return;
}

void _xcb_im_handle_query_extension(xcb_im_t* im,
                                    xcb_im_client_table_t* client,
                                    const xcb_im_proto_header_t* hdr,
                                    uint8_t* data,
                                    bool *del)
{
    query_extension_fr frame;
    _xcb_im_read_frame(im, client, frame);

    int nExts = 0;
    ext_fr ext_list[ARRAY_SIZE(Default_Extension)];
    for (size_t i = 0; i < frame.extensions_supported_by_the_IM_library.size; i++) {
        for (size_t j = 0; j < ARRAY_SIZE(Default_Extension); j ++) {
            if (frame.extensions_supported_by_the_IM_library.items[i].length_of_string
                == im->extension[j].length_of_extension_name &&
                strcmp((char*) frame.extensions_supported_by_the_IM_library.items[i].string,
                       (char*) im->extension[j].extension_name) == 0) {
                ext_list[nExts] = im->extension[j];
                nExts ++;
                break;
            }
        }
    }

    query_extension_fr_free(&frame);
    query_extension_reply_fr reply_frame;
    reply_frame.input_method_ID = client->c.connect_id;
    reply_frame.list_of_extensions_supported_by_th.items = ext_list;
    reply_frame.list_of_extensions_supported_by_th.size = nExts;

    _xcb_im_send_frame(im, client, reply_frame);
}

void _xcb_im_handle_encoding_negotiation(xcb_im_t* im,
                                         xcb_im_client_table_t* client,
                                         const xcb_im_proto_header_t* hdr,
                                         uint8_t* data,
                                         bool *del)
{
    encoding_negotiation_fr frame;
    _xcb_im_read_frame(im, client, frame);

    size_t i, j;
    for (i = 0; i < frame.supported_list_of_encoding_in_IM_library.size; i++) {
        for (j = 0; j < im->encodings.nEncodings; j ++) {
            if (strcmp((char*) frame.supported_list_of_encoding_in_IM_library.items[i].string,
                       im->encodings.encodings[j]) == 0) {
                break;
            }
        }
        if (j != im->encodings.nEncodings) {
            break;
        }
    }

    // no match then we use 0.
    if (i == frame.supported_list_of_encoding_in_IM_library.size) {
        i = 0;
    }

    encoding_negotiation_fr_free(&frame);
    encoding_negotiation_reply_fr reply_frame;
    reply_frame.input_method_ID = client->c.connect_id;
    reply_frame.index_of_the_encoding_dterminated = i;
    reply_frame.category_of_the_encoding_determined = 0;

    _xcb_im_send_frame(im, client, reply_frame);
}

void _xcb_im_handle_get_im_values(xcb_im_t* im,
                                  xcb_im_client_table_t* client,
                                  const xcb_im_proto_header_t* hdr,
                                  uint8_t* data,
                                  bool *del)
{
    get_im_values_fr frame;
    _xcb_im_read_frame(im, client, frame);

    get_im_values_reply_fr reply_frame;
    size_t nBuffers = 0;
    ximattribute_fr buffers[ARRAY_SIZE(Default_IMattr)];
    bool met[ARRAY_SIZE(Default_IMattr)];
    memset(met, 0, sizeof(met));
    for (size_t i = 0; i < frame.im_attribute_id.size; i++) {
        if (frame.im_attribute_id.items[i] >= ARRAY_SIZE(im->id2attr)) {
            continue;
        }
        ximattr_fr* attr = im->id2attr[frame.im_attribute_id.items[i]];
        if ((attr < im->imattr) || (attr >= im->imattr + ARRAY_SIZE(Default_IMattr))) {
            continue;
        }

        if (met[attr - im->imattr]) {
            continue;
        }

        if (Default_IMattr[attr - im->imattr].get_value) {
            continue;
        }

        if (!Default_IMattr[attr - im->imattr].get_value(im, client, &buffers[nBuffers])) {
            continue;
        }
        buffers[nBuffers].attribute_ID = frame.im_attribute_id.items[i];
        met[attr - im->imattr] = true;
        nBuffers++;
    }

    reply_frame.input_method_ID = client->c.connect_id;
    reply_frame.im_attribute_returned.items = buffers;
    reply_frame.im_attribute_returned.size = nBuffers;

    get_im_values_fr_free(&frame);
    _xcb_im_send_frame(im, client, reply_frame);

    for (size_t i = 0; i < nBuffers; i++) {
        free(buffers[i].value);
    }
}

void _xcb_im_handle_disconnect(xcb_im_t* im,
                               xcb_im_client_table_t* client,
                               const xcb_im_proto_header_t* hdr,
                               uint8_t* data,
                               bool *del)
{
    // disconnect doesn't come with message, so we skip parse.

    if (im->callback) {
        im->callback(im, &client->c, hdr, NULL, im->user_data);
    }

    HASH_DELETE(hh2, im->clients_by_win, client);
    HASH_DELETE(hh1, im->clients_by_id, client);
    xcb_destroy_window(im->conn, client->c.accept_win);
    im->free_list = client;
}

void _xcb_im_set_ic_values(xcb_im_t* im,
                           xcb_im_client_table_t* client,
                           uint32_t nICAttrs,
                           xicattribute_fr* icattrs)
{
    bool met[ARRAY_SIZE(Default_ICattr)];
    for (uint32_t i = 0;  i < nICAttrs;  i++) {
        if (icattrs[i].attribute_ID >= ARRAY_SIZE(im->id2attr)) {
            continue;
        }
        xicattr_fr* attr = (xicattr_fr*) im->id2attr[icattrs[i].attribute_ID];
        if ((attr < im->icattr) || (attr >= im->icattr + ARRAY_SIZE(Default_ICattr))) {
            continue;
        }

        if (met[attr - im->icattr]) {
            continue;
        }

        if (Default_ICattr[attr - im->icattr].set_value) {
            continue;
        }

        if (Default_ICattr[attr - im->icattr].set_value(im, client, icattrs[i].value, icattrs[i].value_length)) {
            continue;
        }
    }
}

void _xcb_im_handle_create_ic(xcb_im_t* im,
                              xcb_im_client_table_t* client,
                              const xcb_im_proto_header_t* hdr,
                              uint8_t* data,
                              bool *del)
{
    create_ic_fr frame;
    _xcb_im_read_frame(im, client, frame);

    // TODO
    create_ic_fr_free(&frame);
/*

    create_ic_reply_fr reply_frame;
    reply_frame.input_method_ID = client->c.connect_id;

    _xcb_im_send_frame(im, client, reply_frame);
    if (im->onKeys.nKeys == 0 && im->offKeys.nKeys == 0) {
        _xcb_im_set_event_mask(im, client, icid, im->event_mask, ~im->event_mask);
    }*/
}

void
_xcb_im_handle_set_ic_focus(xcb_im_t* im, xcb_im_client_table_t* client,
                            const xcb_im_proto_header_t* hdr,
                            uint8_t* data, bool *del)
{
    set_ic_focus_fr frame;
    _xcb_im_read_frame(im, client, frame);

    if (im->callback) {
        im->callback(im, &client->c, hdr, &frame, im->user_data);
    }
    set_ic_focus_fr_free(&frame);
}

void
_xcb_im_handle_unset_ic_focus(xcb_im_t* im, xcb_im_client_table_t* client,
                              const xcb_im_proto_header_t* hdr,
                              uint8_t* data, bool *del)
{
    unset_ic_focus_fr frame;
    _xcb_im_read_frame(im, client, frame);

    if (im->callback) {
        im->callback(im, &client->c, hdr, &frame, im->user_data);
    }
    unset_ic_focus_fr_free(&frame);
}

void
_xcb_im_handle_preedit_caret_reply(xcb_im_t* im, xcb_im_client_table_t* client,
                                   const xcb_im_proto_header_t* hdr,
                                   uint8_t* data, bool *del)
{
    preedit_caret_reply_fr frame;
    _xcb_im_read_frame(im, client, frame);

    if (im->callback) {
        im->callback(im, &client->c, hdr, &frame, im->user_data);
    }
    preedit_caret_reply_fr_free(&frame);
}
