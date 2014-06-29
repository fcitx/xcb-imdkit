#include "common.h"
#include "imclient_p.h"
#include "clientprotocolhandler.h"

bool _xcb_xim_send_query_extension(xcb_xim_t* im)
{
    xcb_im_query_extension_fr_t frame;
    xcb_im_str_fr_t extensions[ARRAY_SIZE(Default_Extension)];
    frame.input_method_ID = im->connect_id;
    frame.extensions_supported_by_the_IM_library.size = ARRAY_SIZE(Default_Extension);
    frame.extensions_supported_by_the_IM_library.items = extensions;
    for (uint32_t i = 0; i < frame.extensions_supported_by_the_IM_library.size; i++) {
        extensions[i].length_of_string = strlen(Default_Extension[i].name);
        extensions[i].string = (uint8_t*) Default_Extension[i].name;
    }

    bool fail;
    _xcb_xim_send_frame(im, frame, fail);
    return !fail;
}


bool _xcb_xim_send_encoding_negotiation(xcb_xim_t* im)
{
    xcb_im_encoding_negotiation_fr_t frame;
    xcb_im_str_fr_t compound;
    compound.length_of_string = strlen("COMPOUND_TEXT");
    compound.string = (uint8_t*) "COMPOUND_TEXT";
    frame.input_method_ID = im->connect_id;
    frame.supported_list_of_encoding_in_IM_library.size = 1;
    frame.supported_list_of_encoding_in_IM_library.items = &compound;
    frame.list_of_encodings_supported_in_th.size = 0;
    frame.list_of_encodings_supported_in_th.items = 0;

    bool fail;
    _xcb_xim_send_frame(im, frame, fail);
    return !fail;
}

void _xcb_xim_handle_open_reply(xcb_xim_t* im, const xcb_im_packet_header_fr_t* hdr, uint8_t* data)
{
    xcb_im_open_reply_fr_t frame;
    bool fail;
    _xcb_xim_read_frame(frame, data, XIM_MESSAGE_BYTES(hdr), fail);
    if (fail) {
        return;
    }

    im->connect_id = frame.input_method_ID;
    // construct a name based attr table
    for (uint32_t i = 0; i < frame.IM_attribute_supported.size; i++) {
        xcb_im_ximattr_fr_t* subframe = &frame.IM_attribute_supported.items[i];
        xcb_xim_imattr_table_t* imattr = NULL;
        HASH_FIND(hh, im->imattr, subframe->im_attribute, subframe->length_of_im_attribute, imattr);
        if (imattr) {
            continue;
        }

        imattr = malloc(sizeof(xcb_xim_imattr_table_t));
        imattr->attr.attribute_ID = subframe->attribute_ID;
        imattr->attr.type_of_the_value = subframe->type_of_the_value;
        imattr->attr.length_of_im_attribute = subframe->length_of_im_attribute;
        imattr->attr.im_attribute = malloc((imattr->attr.length_of_im_attribute + 1) * sizeof(char));
        imattr->attr.im_attribute[imattr->attr.length_of_im_attribute] = 0;
        memcpy(imattr->attr.im_attribute, subframe->im_attribute, imattr->attr.length_of_im_attribute);

        HASH_ADD(hh, im->imattr, attr.im_attribute, imattr->attr.length_of_im_attribute, imattr);
    }
    for (uint32_t i = 0; i < frame.IC_attribute_supported.size; i++) {
        xcb_im_xicattr_fr_t* subframe = &frame.IC_attribute_supported.items[i];
        xcb_xim_icattr_table_t* icattr = NULL;
        HASH_FIND(hh, im->icattr, subframe->ic_attribute, subframe->length_of_ic_attribute, icattr);
        if (icattr) {
            continue;
        }

        icattr = malloc(sizeof(xcb_xim_icattr_table_t));
        icattr->attr.attribute_ID = subframe->attribute_ID;
        icattr->attr.type_of_the_value = subframe->type_of_the_value;
        icattr->attr.length_of_ic_attribute = subframe->length_of_ic_attribute;
        icattr->attr.ic_attribute = malloc((icattr->attr.length_of_ic_attribute + 1) * sizeof(char));
        icattr->attr.ic_attribute[icattr->attr.length_of_ic_attribute] = 0;
        memcpy(icattr->attr.ic_attribute, subframe->ic_attribute, icattr->attr.length_of_ic_attribute);

        HASH_ADD(hh, im->icattr, attr.ic_attribute, icattr->attr.length_of_ic_attribute, icattr);
    }
    xcb_im_open_reply_fr_free(&frame);

    _xcb_xim_send_query_extension(im);
}

void _xcb_xim_handle_query_extension_reply(xcb_xim_t* im, const xcb_im_packet_header_fr_t* hdr, uint8_t* data)
{
    xcb_im_query_extension_reply_fr_t frame;
    bool fail;
    _xcb_xim_read_frame(frame, data, XIM_MESSAGE_BYTES(hdr), fail);
    if (fail) {
        return;
    }

    do {
        if (frame.input_method_ID != im->connect_id) {
            break;
        }

        // TODO, save extensions here

        _xcb_xim_send_encoding_negotiation(im);
    } while(0);
    xcb_im_query_extension_reply_fr_free(&frame);
}

void _xcb_xim_handle_encoding_negotiation_reply(xcb_xim_t* im, const xcb_im_packet_header_fr_t* hdr, uint8_t* data)
{
    xcb_im_encoding_negotiation_reply_fr_t frame;
    bool fail;
    _xcb_xim_read_frame(frame, data, XIM_MESSAGE_BYTES(hdr), fail);
    if (fail) {
        return;
    }

    do {
        if (frame.input_method_ID != im->connect_id) {
            break;
        }

        // we only send compound
        if (frame.index_of_the_encoding_dterminated != 0) {
            break;
        }

        im->opened = true;

        if (im->open.callback) {
            im->open.callback(im, im->open.user_data);
        }

        _xcb_change_event_mask(im->conn, im->accept_win, XCB_EVENT_MASK_STRUCTURE_NOTIFY, true);
    } while(0);
    xcb_im_encoding_negotiation_reply_fr_free(&frame);
}

void _xcb_xim_handle_register_triggerkeys(xcb_xim_t* im, const xcb_im_packet_header_fr_t* hdr, uint8_t* data)
{
    xcb_im_register_triggerkeys_fr_t frame;
    bool fail;
    _xcb_xim_read_frame(frame, data, XIM_MESSAGE_BYTES(hdr), fail);
    if (fail) {
        return;
    }

    do {
        // we dont check (frame.input_method_ID != im->connect_id);
        // here because this is send with open_reply

        // TODO save register trigger keys here
    } while(0);
    xcb_im_register_triggerkeys_fr_free(&frame);

}

void _xcb_xim_handle_create_ic_reply(xcb_xim_t* im, const xcb_im_packet_header_fr_t* hdr, uint8_t* data)
{
    xcb_im_create_ic_reply_fr_t frame;
    bool fail;
    _xcb_xim_read_frame(frame, data, XIM_MESSAGE_BYTES(hdr), fail);
    if (fail) {
        return;
    }

    do {
        if (!im->current) {
            break;
        }

        if (im->current->major_code != XIM_CREATE_IC) {
            break;
        }

        xcb_xim_request_queue_t* request = im->current;
        im->current = NULL;

        if (request->callback.create_ic) {
            request->callback.create_ic(im, true, frame.input_context_ID, request->user_data);
        }

        _xcb_xim_request_free(request);
    } while(0);
    xcb_im_create_ic_reply_fr_free(&frame);
}

void _xcb_xim_handle_destroy_ic_reply(xcb_xim_t* im, const xcb_im_packet_header_fr_t* hdr, uint8_t* data)
{
    xcb_im_destroy_ic_reply_fr_t frame;
    bool fail;
    _xcb_xim_read_frame(frame, data, XIM_MESSAGE_BYTES(hdr), fail);
    if (fail) {
        return;
    }

    do {
        if (!im->current) {
            break;
        }

        if (im->current->major_code != XIM_DESTROY_IC) {
            break;
        }

        if (im->current->frame.destroy_ic.input_context_ID != frame.input_context_ID) {
            break;
        }

        xcb_xim_request_queue_t* request = im->current;
        im->current = NULL;

        if (request->callback.destroy_ic) {
            request->callback.destroy_ic(im, frame.input_context_ID, request->user_data);
        }

        _xcb_xim_request_free(request);
    } while(0);
    xcb_im_destroy_ic_reply_fr_free(&frame);
}

void _xcb_xim_handle_get_im_values_reply(xcb_xim_t* im, const xcb_im_packet_header_fr_t* hdr, uint8_t* data)
{
    xcb_im_get_im_values_reply_fr_t frame;
    bool fail;
    _xcb_xim_read_frame(frame, data, XIM_MESSAGE_BYTES(hdr), fail);
    if (fail) {
        return;
    }

    do {
        if (!im->current) {
            break;
        }

        if (im->current->major_code != XIM_GET_IM_VALUES) {
            break;
        }

        xcb_xim_request_queue_t* request = im->current;
        im->current = NULL;

        if (request->callback.get_im_values) {
            request->callback.get_im_values(im, &frame, request->user_data);
        }

        _xcb_xim_request_free(request);
    } while(0);
    xcb_im_get_im_values_reply_fr_free(&frame);
}


void _xcb_xim_handle_get_ic_values_reply(xcb_xim_t* im, const xcb_im_packet_header_fr_t* hdr, uint8_t* data)
{
    xcb_im_get_ic_values_reply_fr_t frame;
    bool fail;
    _xcb_xim_read_frame(frame, data, XIM_MESSAGE_BYTES(hdr), fail);
    if (fail) {
        return;
    }

    do {
        if (!im->current) {
            break;
        }

        if (im->current->major_code != XIM_GET_IC_VALUES) {
            break;
        }

        if (im->current->frame.get_ic_values.input_context_ID != frame.input_context_ID) {
            break;
        }

        xcb_xim_request_queue_t* request = im->current;
        im->current = NULL;

        if (request->callback.get_ic_values) {
            request->callback.get_ic_values(im, frame.input_context_ID, &frame, request->user_data);
        }

        _xcb_xim_request_free(request);
    } while(0);
    xcb_im_get_ic_values_reply_fr_free(&frame);
}



void _xcb_xim_handle_set_ic_values_reply(xcb_xim_t* im, const xcb_im_packet_header_fr_t* hdr, uint8_t* data)
{
    xcb_im_set_ic_values_reply_fr_t frame;
    bool fail;
    _xcb_xim_read_frame(frame, data, XIM_MESSAGE_BYTES(hdr), fail);
    if (fail) {
        return;
    }

    do {
        if (!im->current) {
            break;
        }

        if (im->current->major_code != XIM_SET_IC_VALUES) {
            break;
        }

        if (im->current->frame.set_ic_values.input_context_ID != frame.input_context_ID) {
            break;
        }

        xcb_xim_request_queue_t* request = im->current;
        im->current = NULL;

        if (request->callback.set_ic_values) {
            request->callback.set_ic_values(im, frame.input_context_ID, request->user_data);
        }

        _xcb_xim_request_free(request);
    } while(0);
    xcb_im_set_ic_values_reply_fr_free(&frame);
}

