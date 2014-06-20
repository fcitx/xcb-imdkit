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
        im->callback(im, &client->c, NULL, hdr, &frame, NULL, im->user_data);
    }
    connect_fr_free(&frame);

    connect_reply_fr reply_frame;
    reply_frame.server_major_protocol_version = frame.client_major_protocol_version;
    reply_frame.server_minor_protocol_version = frame.client_minor_protocol_version;

    _xcb_im_send_frame(im, client, reply_frame, true);
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

    _xcb_im_send_frame(im, client, frame, false);
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
        im->callback(im, &client->c, NULL, hdr, &frame, NULL, im->user_data);
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

    _xcb_im_send_frame(im, client, reply_frame, true);
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
        im->callback(im, &client->c, NULL, hdr, &frame, NULL, im->user_data);
    }

    close_fr_free(&frame);
    close_reply_fr reply_frame;
    reply_frame.input_method_ID = frame.input_method_ID;
    _xcb_im_send_frame(im, client, reply_frame, true);
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

    _xcb_im_send_frame(im, client, reply_frame, true);
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

    _xcb_im_send_frame(im, client, reply_frame, true);
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

        if (!Default_IMattr[attr - im->imattr].get_value) {
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
    _xcb_im_send_frame(im, client, reply_frame, true);

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
        im->callback(im, &client->c, NULL, hdr, NULL, NULL, im->user_data);
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

    while (client->input_contexts) {
        xcb_im_input_context_table_t* p = client->input_contexts;
        // TODO: mind need to free more?
        HASH_DEL(client->input_contexts, p);
        free(p);
    }

    im->free_list = client;
}

static void _xcb_im_set_event_mask(xcb_im_t* im, xcb_im_client_table_t* client, uint32_t icid, uint32_t forward_event_mask, uint32_t sync_mask)
{
    set_event_mask_fr frame;
    frame.forward_event_mask = forward_event_mask;
    frame.synchronous_event_mask = sync_mask;
    frame.input_method_ID = client->c.connect_id;
    frame.input_context_ID = icid;
    _xcb_im_send_frame(im, client, frame, false);
}

void _xcb_im_handle_create_ic(xcb_im_t* im,
                              xcb_im_client_table_t* client,
                              const xcb_im_proto_header_t* hdr,
                              uint8_t* data,
                              bool *del)
{
    create_ic_fr frame;
    _xcb_im_read_frame(im, client, frame);

    do {
        if (frame.input_method_ID != client->c.connect_id) {
            break;
        }

        xcb_im_input_context_table_t* ic = _xcb_im_new_input_context(im, client);

        create_ic_reply_fr reply_frame;
        reply_frame.input_method_ID = client->c.connect_id;
        reply_frame.input_context_ID = ic->ic.id;

        if (im->callback) {
            im->callback(im, &client->c, NULL, hdr, &frame, &reply_frame, im->user_data);
        }

        _xcb_im_send_frame(im, client, reply_frame, true);
        if (im->onKeys.nKeys == 0 && im->offKeys.nKeys == 0) {
            _xcb_im_set_event_mask(im, client, reply_frame.input_context_ID, im->event_mask, ~im->event_mask);
        }
        create_ic_fr_free(&frame);
        return;
    } while(0);
    // error
    create_ic_fr_free(&frame);
    _xcb_im_send_error_message(im, client);
    return;
}

void _xcb_im_handle_set_ic_values(xcb_im_t* im,
                                  xcb_im_client_table_t* client,
                                  const xcb_im_proto_header_t* hdr,
                                  uint8_t* data,
                                  bool *del)
{
    set_ic_values_fr frame;
    _xcb_im_read_frame(im, client, frame);

    do {
        if (frame.input_method_ID != client->c.connect_id) {
            break;
        }

        xcb_im_input_context_table_t* ic = NULL;
        HASH_FIND(hh, client->input_contexts, &frame.input_context_ID, sizeof(uint16_t), ic);
        if (!ic) {
            break;
        }

        if (im->callback) {
            im->callback(im, &client->c, &ic->ic, hdr, &frame, NULL, im->user_data);
        }
        set_ic_values_fr_free(&frame);

        set_ic_values_reply_fr reply_frame;
        reply_frame.input_method_ID = client->c.connect_id;
        reply_frame.input_context_ID = frame.input_context_ID;
        _xcb_im_send_frame(im, client, reply_frame, true);
        return;
    } while(0);
    // error
    set_ic_values_fr_free(&frame);
    _xcb_im_send_error_message(im, client);
    return;
}


void _xcb_im_handle_get_ic_values(xcb_im_t* im,
                                  xcb_im_client_table_t* client,
                                  const xcb_im_proto_header_t* hdr,
                                  uint8_t* data,
                                  bool *del)
{
    get_ic_values_fr frame;
    _xcb_im_read_frame(im, client, frame);

    do {
        if (frame.input_method_ID != client->c.connect_id) {
            break;
        }

        xcb_im_input_context_table_t* ic = NULL;
        HASH_FIND(hh, client->input_contexts, &frame.input_context_ID, sizeof(uint16_t), ic);
        if (!ic) {
            break;
        }

        if (im->callback) {
            im->callback(im, &client->c, &ic->ic, hdr, &frame, NULL, im->user_data);
        }
        get_ic_values_fr_free(&frame);

        get_ic_values_reply_fr reply_frame;
        reply_frame.input_method_ID = client->c.connect_id;
        reply_frame.input_context_ID = frame.input_context_ID;
        // TODO
        reply_frame.ic_attribute.size = 0;
        _xcb_im_send_frame(im, client, reply_frame, true);
        return;
    } while(0);
    // error
    get_ic_values_fr_free(&frame);
    _xcb_im_send_error_message(im, client);
    return;
}

void
_xcb_im_handle_set_ic_focus(xcb_im_t* im, xcb_im_client_table_t* client,
                            const xcb_im_proto_header_t* hdr,
                            uint8_t* data, bool *del)
{
    set_ic_focus_fr frame;
    _xcb_im_read_frame(im, client, frame);

    do {
        if (client->c.connect_id != frame.input_method_ID) {
            break;
        }

        xcb_im_input_context_table_t* ic = NULL;
        HASH_FIND(hh, client->input_contexts, &frame.input_context_ID, sizeof(uint16_t), ic);
        if (!ic) {
            break;
        }

        if (im->callback) {
            im->callback(im, &client->c, &ic->ic, hdr, &frame, NULL, im->user_data);
        }
    } while(0);

    set_ic_focus_fr_free(&frame);
}

void
_xcb_im_handle_unset_ic_focus(xcb_im_t* im, xcb_im_client_table_t* client,
                              const xcb_im_proto_header_t* hdr,
                              uint8_t* data, bool *del)
{
    unset_ic_focus_fr frame;
    _xcb_im_read_frame(im, client, frame);

    do {
        if (client->c.connect_id != frame.input_method_ID) {
            break;
        }

        xcb_im_input_context_table_t* ic = NULL;
        HASH_FIND(hh, client->input_contexts, &frame.input_context_ID, sizeof(uint16_t), ic);
        if (!ic) {
            break;
        }

        if (im->callback) {
            im->callback(im, &client->c, &ic->ic, hdr, &frame, NULL, im->user_data);
        }
    } while(0);

    unset_ic_focus_fr_free(&frame);
}

void
_xcb_im_handle_preedit_caret_reply(xcb_im_t* im, xcb_im_client_table_t* client,
                                   const xcb_im_proto_header_t* hdr,
                                   uint8_t* data, bool *del)
{
    preedit_caret_reply_fr frame;
    _xcb_im_read_frame(im, client, frame);

    do {
        if (client->c.connect_id != frame.input_method_ID) {
            break;
        }

        xcb_im_input_context_table_t* ic = NULL;
        HASH_FIND(hh, client->input_contexts, &frame.input_context_ID, sizeof(uint16_t), ic);
        if (!ic) {
            break;
        }

        if (im->callback) {
            im->callback(im, &client->c, &ic->ic, hdr, &frame, NULL, im->user_data);
        }
    } while(0);

    preedit_caret_reply_fr_free(&frame);
}
