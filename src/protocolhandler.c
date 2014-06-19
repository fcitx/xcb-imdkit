#include "protocolhandler.h"
#include "message.h"

void _xcb_im_handle_connect(xcb_im_t* im,
                            xcb_im_client_table_t* client,
                            const xcb_im_proto_header_t* hdr,
                            uint8_t* data)
{
    size_t len = XIM_MESSAGE_BYTES(hdr);
    connect_fr frame;
    connect_fr_read(&frame, &data, &len, client->c.byte_order != im->byte_order);
    if (!data) {
        return;
    }

    if (im->callback) {
        im->callback(im, &client->c, hdr, &frame, im->user_data);
    }

    connect_reply_fr reply_frame;
    reply_frame.server_major_protocol_version = frame.client_major_protocol_version;
    reply_frame.server_minor_protocol_version = frame.client_minor_protocol_version;

    xim_send_frame(reply_frame, connect_reply_fr, XIM_CONNECT_REPLY);
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

    xim_send_frame(frame, register_triggerkeys_fr, XIM_REGISTER_TRIGGERKEYS);
}

void _xcb_im_handle_open(xcb_im_t* im,
                         xcb_im_client_table_t* client,
                         const xcb_im_proto_header_t* hdr,
                         uint8_t* data,
                         bool *del)
{
    size_t len = XIM_MESSAGE_BYTES(hdr);
    open_fr frame;
    open_fr_read(&frame, &data, &len, client->c.byte_order != im->byte_order);
    if (!data) {
        open_fr_free(&frame);
        return;
    }

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

    xim_send_frame(reply_frame, open_reply_fr, XIM_OPEN_REPLY);
}

void _xcb_im_handle_close(xcb_im_t* im,
                          xcb_im_client_table_t* client,
                          const xcb_im_proto_header_t* hdr,
                          uint8_t* data,
                          bool *del)
{
    size_t len = XIM_MESSAGE_BYTES(hdr);
    close_fr frame;
    close_fr_read(&frame, &data, &len, client->c.byte_order != im->byte_order);
    if (!data) {
        return;
    }

    if (im->callback) {
        im->callback(im, &client->c, hdr, &frame, im->user_data);
    }

    close_reply_fr reply_frame;
    reply_frame.input_method_ID = frame.input_method_ID;
    bool fail = true;
    size_t length = close_reply_fr_size(&reply_frame);
    uint8_t* reply = _xcb_im_new_message(im, client, XIM_CLOSE_REPLY, 0, length);
    do {
        if (!reply) {
            break;
        }
        close_reply_fr_write(&reply_frame, reply + XCB_IM_HEADER_SIZE, client->c.byte_order != im->byte_order);
        if (!_xcb_im_send_message(im, client, reply, length)) {
            break;
        }

        fail = false;
    } while(0);
    free(reply);

    if (fail) {
        _xcb_im_send_error_message(im, client);
    }
    return;
}

void _xcb_im_handle_query_extension(xcb_im_t* im,
                                    xcb_im_client_table_t* client,
                                    const xcb_im_proto_header_t* hdr,
                                    uint8_t* data,
                                    bool *del)
{
    size_t len = XIM_MESSAGE_BYTES(hdr);
    query_extension_fr frame;
    query_extension_fr_read(&frame, &data, &len, client->c.byte_order != im->byte_order);
    if (!data) {
        return;
    }

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

    xim_send_frame(reply_frame, query_extension_reply_fr, XIM_QUERY_EXTENSION_REPLY);
}

void _xcb_im_handle_encoding_negotiation(xcb_im_t* im,
                                         xcb_im_client_table_t* client,
                                         const xcb_im_proto_header_t* hdr,
                                         uint8_t* data,
                                         bool *del)
{
    size_t len = XIM_MESSAGE_BYTES(hdr);
    encoding_negotiation_fr frame;
    encoding_negotiation_fr_read(&frame, &data, &len, client->c.byte_order != im->byte_order);
    if (!data) {
        return;
    }

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

    xim_send_frame(reply_frame, encoding_negotiation_reply_fr, XIM_ENCODING_NEGOTIATION_REPLY);
}

void _xcb_im_handle_get_im_values(xcb_im_t* im,
                                  xcb_im_client_table_t* client,
                                  const xcb_im_proto_header_t* hdr,
                                  uint8_t* data,
                                  bool *del)
{
    size_t len = XIM_MESSAGE_BYTES(hdr);
    get_im_values_fr frame;
    get_im_values_fr_read(&frame, &data, &len, client->c.byte_order != im->byte_order);
    if (!data) {
        return;
    }

    get_im_values_reply_fr reply_frame;
    size_t nBuffers = 0;
    ximattribute_fr buffers[ARRAY_SIZE(Default_IMattr)];
    for (size_t i = 0; i < frame.im_attribute_id.size; i++) {
        if (frame.im_attribute_id.items[i] >= ARRAY_SIZE(im->id2attr)) {
            continue;
        }
        ximattr_fr* attr = im->id2attr[frame.im_attribute_id.items[i]];
        if ((attr < im->imattr) || (attr >= im->imattr + ARRAY_SIZE(Default_IMattr))) {
            continue;
        }
        // TODO, now we only have one attribute, so no check is needed here
        // in the future this can be a function pointer

        input_styles_fr fr;
        fr.XIMStyle_list.size = im->inputStyles.nStyles;
        fr.XIMStyle_list.items = calloc(im->inputStyles.nStyles, sizeof(inputstyle_fr));
        for (size_t j = 0; j < im->inputStyles.nStyles; j ++) {
            fr.XIMStyle_list.items[j].inputstyle = im->inputStyles.styles[j];
        }

        buffers[nBuffers].attribute_ID = frame.im_attribute_id.items[i];
        buffers[nBuffers].value = malloc(input_styles_fr_size(&fr));
        buffers[nBuffers].value_length = input_styles_fr_size(&fr);
        input_styles_fr_write(&fr, buffers[nBuffers].value, client->c.byte_order != im->byte_order);
        input_styles_fr_free(&fr);
        nBuffers++;
    }

    reply_frame.input_method_ID = client->c.connect_id;
    reply_frame.im_attribute_returned.items = buffers;
    reply_frame.im_attribute_returned.size = nBuffers;

    get_im_values_fr_free(&frame);
    xim_send_frame(reply_frame, get_im_values_reply_fr, XIM_GET_IM_VALUES_REPLY);

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

void _xcb_im_handle_create_ic(xcb_im_t* im,
                              xcb_im_client_table_t* client,
                              const xcb_im_proto_header_t* hdr,
                              uint8_t* data,
                              bool *del)
{
    size_t len = XIM_MESSAGE_BYTES(hdr);
    create_ic_fr frame;
    create_ic_fr_read(&frame, &data, &len, client->c.byte_order != im->byte_order);
    if (!data) {
        create_ic_fr_free(&frame);
        return;
    }

    // TODO
/*

    create_ic_reply_fr reply_frame;
    reply_frame.input_method_ID = client->c.connect_id;

    xim_send_frame(reply_frame, create_ic_reply_fr, XIM_CREATE_IC_REPLY);
    if (im->onKeys.nKeys == 0 && im->offKeys.nKeys == 0) {
        _xcb_im_set_event_mask(im, client, icid, im->event_mask, ~im->event_mask);
    }*/
}
