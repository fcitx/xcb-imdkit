#include "protocolhandler.h"
#include "message.h"

void _xcb_im_handle_connect(xcb_im_t* im,
                            xcb_im_client_table_t* client,
                            const xcb_im_proto_header_t* hdr,
                            uint8_t* data)
{
    xcb_im_connect_fr_t frame;
    _xcb_im_read_frame_with_error(im, client, frame, data, XIM_MESSAGE_BYTES(hdr));

    if (im->callback) {
        im->callback(im, &client->c, NULL, hdr, &frame, NULL, im->user_data);
    }
    xcb_im_connect_fr_free(&frame);

    xcb_im_connect_reply_fr_t reply_frame;
    reply_frame.server_major_protocol_version = frame.client_major_protocol_version;
    reply_frame.server_minor_protocol_version = frame.client_minor_protocol_version;

    _xcb_im_send_frame(im, client, reply_frame, true);
    return;
}

static void _xcb_im_send_trigger_key(xcb_im_t* im, xcb_im_client_table_t* client)
{
    xcb_im_register_triggerkeys_fr_t frame;
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
    xcb_im_open_fr_t frame;
    _xcb_im_read_frame_with_error(im, client, frame, data, XIM_MESSAGE_BYTES(hdr));

    if (im->callback) {
        im->callback(im, &client->c, NULL, hdr, &frame, NULL, im->user_data);
    }

    xcb_im_open_fr_free(&frame);
    /*endif*/
    if (im->onKeys.nKeys || im->offKeys.nKeys) {
        _xcb_im_send_trigger_key(im, client);
    }

    xcb_im_open_reply_fr_t reply_frame;
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
    xcb_im_close_fr_t frame;
    _xcb_im_read_frame_with_error(im, client, frame, data, XIM_MESSAGE_BYTES(hdr));

    if (im->callback) {
        im->callback(im, &client->c, NULL, hdr, &frame, NULL, im->user_data);
    }

    xcb_im_close_fr_free(&frame);
    xcb_im_close_reply_fr_t reply_frame;
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
    xcb_im_query_extension_fr_t frame;
    _xcb_im_read_frame_with_error(im, client, frame, data, XIM_MESSAGE_BYTES(hdr));

    int nExts = 0;
    xcb_im_ext_fr_t ext_list[ARRAY_SIZE(Default_Extension)];
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

    xcb_im_query_extension_fr_free(&frame);
    xcb_im_query_extension_reply_fr_t reply_frame;
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
    xcb_im_encoding_negotiation_fr_t frame;
    _xcb_im_read_frame_with_error(im, client, frame, data, XIM_MESSAGE_BYTES(hdr));

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

    xcb_im_encoding_negotiation_fr_free(&frame);
    xcb_im_encoding_negotiation_reply_fr_t reply_frame;
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
    xcb_im_get_im_values_fr_t frame;
    _xcb_im_read_frame_with_error(im, client, frame, data, XIM_MESSAGE_BYTES(hdr));

    xcb_im_get_im_values_reply_fr_t reply_frame;
    size_t nBuffers = 0;
    xcb_im_ximattribute_fr_t buffers[ARRAY_SIZE(Default_IMattr)];
    bool met[ARRAY_SIZE(Default_IMattr)];
    memset(met, 0, sizeof(met));
    for (size_t i = 0; i < frame.im_attribute_id.size; i++) {
        if (frame.im_attribute_id.items[i] >= ARRAY_SIZE(im->id2attr)) {
            continue;
        }
        xcb_im_ximattr_fr_t* attr = im->id2attr[frame.im_attribute_id.items[i]];
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

    xcb_im_get_im_values_fr_free(&frame);
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

    client->hh1.next = im->free_list;
    im->free_list = client;
}

static void _xcb_im_set_event_mask(xcb_im_t* im, xcb_im_client_table_t* client, uint32_t icid, uint32_t forward_event_mask, uint32_t sync_mask)
{
    xcb_im_set_event_mask_fr_t frame;
    frame.forward_event_mask = forward_event_mask;
    frame.synchronous_event_mask = sync_mask;
    frame.input_method_ID = client->c.connect_id;
    frame.input_context_ID = icid;
    _xcb_im_send_frame(im, client, frame, false);
}

void _xcb_im_parse_ic_value(xcb_im_t* im,
                            xcb_im_input_context_table_t* ic,
                            void* p,
                            const xcb_im_default_ic_attr_t* icattr,
                            uint8_t* data, size_t length)
{
    switch (icattr->type) {
        case XimType_CARD32:
        case XimType_Window:
        {
            uint32_t* result = p;
            uint32_t_read(result, &data, &length, im->byte_order != ic->ic.client->byte_order);
            break;
        }
        case XimType_XRectangle:
        {
            xcb_rectangle_t* result = p;
            xcb_im_xrectangle_fr_t fr;
            _xcb_im_read_frame(im, (xcb_im_client_table_t*) ic->ic.client, fr, data, length);
            result->x = fr.x;
            result->y = fr.y;
            result->height = fr.height;
            result->width = fr.width;
            break;
        }
        case XimType_XPoint:
        {
            xcb_point_t* result = p;
            xcb_im_xpoint_fr_t fr;
            _xcb_im_read_frame(im, (xcb_im_client_table_t*) ic->ic.client, fr, data, length);
            result->x = fr.x;
            result->y = fr.y;
            break;
        }
        case XimType_XFontSet:
        {
            break;
        }
    }
}

void _xcb_im_parse_nested_ic_values(xcb_im_t* im,
                                    xcb_im_input_context_table_t* ic,
                                    void* p,
                                    ssize_t* offsets,
                                    uint8_t* data, size_t length
                                   )
{
    while (length != 0) {
        xcb_im_xicattribute_fr_t fr;
        xcb_im_xicattribute_fr_read(&fr, &data, &length, im->byte_order != ic->ic.client->byte_order);
        if (!data) {
            return;
        }

        const xcb_im_default_ic_attr_t* entry = _xcb_im_default_ic_attr_entry(im, fr.attribute_ID);
        if (!entry || offsets[fr.attribute_ID] < 0) {
            continue;
        }

        _xcb_im_parse_ic_value(im, ic, (((uint8_t*)p) + offsets[fr.attribute_ID]), entry, fr.value, fr.value_length);
    }
}

void _xcb_im_parse_ic_values(xcb_im_t* im,
                             xcb_im_input_context_table_t* ic,
                             uint32_t nAttr,
                             xcb_im_xicattribute_fr_t* icattrs)
{
    for (uint32_t i = 0; i < nAttr; i++) {
        if (icattrs[i].attribute_ID == im->separatorAttr_id) {
            continue;
        }

        if (icattrs[i].attribute_ID == im->statusAttr_id) {
            _xcb_im_parse_nested_ic_values(im, ic, &ic->ic.status, im->id2statusoffset, icattrs[i].value, icattrs[i].value_length);
        } else if (icattrs[i].attribute_ID == im->preeditAttr_id) {
            _xcb_im_parse_nested_ic_values(im, ic, &ic->ic.preedit, im->id2preeditoffset, icattrs[i].value, icattrs[i].value_length);
        } else {
            const xcb_im_default_ic_attr_t* entry = _xcb_im_default_ic_attr_entry(im, icattrs[i].attribute_ID);
            if (!entry || im->id2icoffset[icattrs[i].attribute_ID] < 0) {
                continue;
            }
            _xcb_im_parse_ic_value(im,
                                   ic,
                                   (((uint8_t*)&ic->ic) + im->id2icoffset[icattrs[i].attribute_ID]),
                                   entry,
                                   icattrs[i].value,
                                   icattrs[i].value_length);
        }
    }
}


void _xcb_im_handle_create_ic(xcb_im_t* im,
                              xcb_im_client_table_t* client,
                              const xcb_im_proto_header_t* hdr,
                              uint8_t* data,
                              bool *del)
{
    xcb_im_create_ic_fr_t frame;
    _xcb_im_read_frame_with_error(im, client, frame, data, XIM_MESSAGE_BYTES(hdr));

    do {
        if (frame.input_method_ID != client->c.connect_id) {
            break;
        }

        xcb_im_input_context_table_t* ic = _xcb_im_new_input_context(im, client);

        _xcb_im_parse_ic_values(im, ic, frame.ic_attributes.size, frame.ic_attributes.items);

        xcb_im_create_ic_reply_fr_t reply_frame;
        reply_frame.input_method_ID = client->c.connect_id;
        reply_frame.input_context_ID = ic->ic.id;

        if (im->callback) {
            im->callback(im, &client->c, NULL, hdr, &frame, &reply_frame, im->user_data);
        }

        _xcb_im_send_frame(im, client, reply_frame, true);
        if (im->onKeys.nKeys == 0 && im->offKeys.nKeys == 0) {
            _xcb_im_set_event_mask(im, client, reply_frame.input_context_ID, im->event_mask, ~im->event_mask);
        }
        xcb_im_create_ic_fr_free(&frame);
        return;
    } while(0);
    // error
    xcb_im_create_ic_fr_free(&frame);
    _xcb_im_send_error_message(im, client);
    return;
}


void _xcb_im_handle_destroy_ic(xcb_im_t* im,
                               xcb_im_client_table_t* client,
                               const xcb_im_proto_header_t* hdr,
                               uint8_t* data,
                               bool *del)
{
    xcb_im_destroy_ic_fr_t frame;
    _xcb_im_read_frame_with_error(im, client, frame, data, XIM_MESSAGE_BYTES(hdr));

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
        xcb_im_destroy_ic_fr_free(&frame);

        xcb_im_destroy_ic_reply_fr_t reply_frame;
        reply_frame.input_method_ID = client->c.connect_id;
        reply_frame.input_context_ID = frame.input_context_ID;

        // Destroy ic
        HASH_DEL(client->input_contexts, ic);
        ic->hh.next = client->ic_free_list;
        client->ic_free_list = ic;

        _xcb_im_send_frame(im, client, reply_frame, true);
        return;
    } while(0);
    // error
    xcb_im_destroy_ic_fr_free(&frame);
    _xcb_im_send_error_message(im, client);
    return;
}

void _xcb_im_handle_set_ic_values(xcb_im_t* im,
                                  xcb_im_client_table_t* client,
                                  const xcb_im_proto_header_t* hdr,
                                  uint8_t* data,
                                  bool *del)
{
    xcb_im_set_ic_values_fr_t frame;
    _xcb_im_read_frame_with_error(im, client, frame, data, XIM_MESSAGE_BYTES(hdr));

    do {
        if (frame.input_method_ID != client->c.connect_id) {
            break;
        }

        xcb_im_input_context_table_t* ic = NULL;
        HASH_FIND(hh, client->input_contexts, &frame.input_context_ID, sizeof(uint16_t), ic);
        if (!ic) {
            break;
        }

        _xcb_im_parse_ic_values(im, ic, frame.ic_attribute.size, frame.ic_attribute.items);

        if (im->callback) {
            im->callback(im, &client->c, &ic->ic, hdr, &frame, NULL, im->user_data);
        }
        xcb_im_set_ic_values_fr_free(&frame);

        xcb_im_set_ic_values_reply_fr_t reply_frame;
        reply_frame.input_method_ID = client->c.connect_id;
        reply_frame.input_context_ID = frame.input_context_ID;
        _xcb_im_send_frame(im, client, reply_frame, true);
        return;
    } while(0);
    // error
    xcb_im_set_ic_values_fr_free(&frame);
    _xcb_im_send_error_message(im, client);
    return;
}

uint8_t* _xcb_im_get_ic_value(xcb_im_t* im,
                              xcb_im_input_context_table_t* ic,
                              void* p,
                              const xcb_im_default_ic_attr_t* icattr,
                              uint8_t* data)
{
    switch (icattr->type) {
        case XimType_CARD32:
        case XimType_Window:
        {
            uint32_t* result = p;
            data = uint32_t_write(result, data, im->byte_order != ic->ic.client->byte_order);
            break;
        }
        case XimType_XRectangle:
        {
            xcb_rectangle_t* result = p;
            xcb_im_xrectangle_fr_t fr;
            fr.x = result->x;
            fr.y = result->y;
            fr.width = result->width;
            fr.height = result->height;
            data = xcb_im_xrectangle_fr_write(&fr, data, im->byte_order != ic->ic.client->byte_order);
            break;
        }
        case XimType_XPoint:
        {
            xcb_point_t* result = p;
            xcb_im_xpoint_fr_t fr;
            fr.x = result->x;
            fr.y = result->y;
            data = xcb_im_xpoint_fr_write(&fr, data, im->byte_order != ic->ic.client->byte_order);
            break;
        }
        case XimType_XFontSet:
        {
            break;
        }
    }
    return data;
}

size_t _xcb_im_ic_attr_size(uint32_t type)
{
    switch (type) {
        case XimType_CARD32:
        case XimType_Window:
        {
            return sizeof(uint32_t);
        }
        case XimType_XRectangle:
        {
            xcb_im_xrectangle_fr_t fr;
            return xcb_im_xrectangle_fr_size(&fr);
        }
        case XimType_XPoint:
        {
            xcb_im_xpoint_fr_t fr;
            return xcb_im_xpoint_fr_size(&fr);
        }
    }
    return 0;
}

uint32_t _xcb_im_get_nested_ic_values(xcb_im_t* im,
                                        xcb_im_input_context_table_t* ic,
                                        void* p,
                                        ssize_t* offsets,
                                        uint16_t* attrIDs,
                                        uint32_t i,
                                        uint32_t size,
                                        xcb_im_xicattribute_fr_t* attr)
{
    size_t totalSize = 0;
    uint8_t* data = NULL;
    uint32_t start = i;
    for (int round = 0; round < 2; round ++) {
        i = start;
        while (i < size && attrIDs[i] != im->separatorAttr_id) {
            const xcb_im_default_ic_attr_t* entry = _xcb_im_default_ic_attr_entry(im, attrIDs[i]);
            if (!entry || !entry->read || offsets[attrIDs[i]] < 0) {
                continue;
            }

            if (round == 0) {
                xcb_im_xicattribute_fr_t fr;
                fr.value_length = _xcb_im_ic_attr_size(entry->type);
                totalSize += xcb_im_xicattribute_fr_size(&fr);
            } else {
                uint8_t* start = data;
                uint16_t value_length = _xcb_im_ic_attr_size(entry->type);
                bool swap = im->byte_order != ic->ic.client->byte_order;
                data = uint16_t_write(&attrIDs[i], data, swap);
                data = uint16_t_write(&value_length, data, swap);
                data = _xcb_im_get_ic_value(im, ic, (((uint8_t*)p) + offsets[attrIDs[i]]), entry, data);
                data = (uint8_t*) align_to_4((uintptr_t) data, data - start, NULL);
            }
        }
        if (round == 0) {
            data = malloc(totalSize);
            if (!data) {
                break;
            }
            attr->value_length = totalSize;
            attr->value = data;
        }
    }

    return i;
}


void _xcb_im_handle_get_ic_values(xcb_im_t* im,
                                  xcb_im_client_table_t* client,
                                  const xcb_im_proto_header_t* hdr,
                                  uint8_t* data,
                                  bool *del)
{
    xcb_im_get_ic_values_fr_t frame;
    _xcb_im_read_frame_with_error(im, client, frame, data, XIM_MESSAGE_BYTES(hdr));

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


        xcb_im_get_ic_values_reply_fr_t reply_frame;
        reply_frame.input_method_ID = client->c.connect_id;
        reply_frame.input_context_ID = frame.input_context_ID;
        reply_frame.ic_attribute.size = 0;
        uint32_t nBuffers = 0;
        xcb_im_xicattribute_fr_t buffers[ARRAY_SIZE(Default_ICattr)];
        memset(buffers, 0, sizeof(buffers));
        uint32_t i = 0;
        while(i < frame.ic_attribute.size) {
            buffers[nBuffers].attribute_ID = frame.ic_attribute.items[i];

            if (frame.ic_attribute.items[i] == im->statusAttr_id) {
                i += _xcb_im_get_nested_ic_values(im, ic, &ic->ic.status, im->id2statusoffset, frame.ic_attribute.items, i + 1, frame.ic_attribute.size, &buffers[nBuffers]);
            } else if (frame.ic_attribute.items[i] == im->preeditAttr_id) {
                i += _xcb_im_get_nested_ic_values(im, ic, &ic->ic.preedit, im->id2preeditoffset, frame.ic_attribute.items, i + 1, frame.ic_attribute.size, &buffers[nBuffers]);
            } else {
                const xcb_im_default_ic_attr_t* entry = _xcb_im_default_ic_attr_entry(im, frame.ic_attribute.items[i]);
                i++;
                if (!entry) {
                    continue;
                }

                if (strcmp(entry->name, XNFilterEvents) == 0) {
                    uint32_t result = XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE;
                    buffers[nBuffers].value = malloc(sizeof(uint32_t));
                    buffers[nBuffers].value_length = sizeof(uint32_t);
                    if (data) {
                        uint32_t_write(&result, buffers[nBuffers].value, im->byte_order != ic->ic.client->byte_order);
                    }
                }
            }

            if (buffers[nBuffers].value) {
                nBuffers ++;
            }
        }

        reply_frame.ic_attribute.size = nBuffers;
        reply_frame.ic_attribute.items = buffers;
        xcb_im_get_ic_values_fr_free(&frame);
        _xcb_im_send_frame(im, client, reply_frame, true);
        for (uint32_t i = 0; i < nBuffers; i++) {
            free(buffers[i].value);
        }
        return;
    } while(0);
    // error
    xcb_im_get_ic_values_fr_free(&frame);
    _xcb_im_send_error_message(im, client);
    return;
}

void
_xcb_im_handle_set_ic_focus(xcb_im_t* im, xcb_im_client_table_t* client,
                            const xcb_im_proto_header_t* hdr,
                            uint8_t* data, bool *del)
{
    xcb_im_set_ic_focus_fr_t frame;
    _xcb_im_read_frame_with_error(im, client, frame, data, XIM_MESSAGE_BYTES(hdr));

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

    xcb_im_set_ic_focus_fr_free(&frame);
}

void
_xcb_im_handle_unset_ic_focus(xcb_im_t* im, xcb_im_client_table_t* client,
                              const xcb_im_proto_header_t* hdr,
                              uint8_t* data, bool *del)
{
    xcb_im_unset_ic_focus_fr_t frame;
    _xcb_im_read_frame_with_error(im, client, frame, data, XIM_MESSAGE_BYTES(hdr));

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

    xcb_im_unset_ic_focus_fr_free(&frame);
}

void
_xcb_im_handle_preedit_caret_reply(xcb_im_t* im, xcb_im_client_table_t* client,
                                   const xcb_im_proto_header_t* hdr,
                                   uint8_t* data, bool *del)
{
    xcb_im_preedit_caret_reply_fr_t frame;
    _xcb_im_read_frame_with_error(im, client, frame, data, XIM_MESSAGE_BYTES(hdr));

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

    xcb_im_preedit_caret_reply_fr_free(&frame);
}
