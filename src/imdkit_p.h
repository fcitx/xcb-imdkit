#ifndef IMDKIT_P_H
#define IMDKIT_P_H

#include "imdkit.h"
#include "uthash.h"
#include "list.h"
#include "common.h"
#include "message.h"

#define LOCALES_BUFSIZE (sizeof(XCB_IM_ALL_LOCALES) + 32)

/*
 * values for the flag of XIM_ERROR
 */
#define XIM_IMID_VALID          0x0001
#define XIM_ICID_VALID          0x0002

/*
 * XIM Error Code
 */
#define XIM_BadAlloc            1
#define XIM_BadStyle            2
#define XIM_BadClientWindow     3
#define XIM_BadFocusWindow      4
#define XIM_BadArea         5
#define XIM_BadSpotLocation     6
#define XIM_BadColormap         7
#define XIM_BadAtom         8
#define XIM_BadPixel            9
#define XIM_BadPixmap           10
#define XIM_BadName         11
#define XIM_BadCursor           12
#define XIM_BadProtocol         13
#define XIM_BadForeground       14
#define XIM_BadBackground       15
#define XIM_LocaleNotSupported      16
#define XIM_BadSomething        999

/*
 * byte order
 */
#define BIGENDIAN   (CARD8) 0x42    /* MSB first */
#define LITTLEENDIAN    (CARD8) 0x6c    /* LSB first */

/*
 * values for the category of XIM_ENCODING_NEGOTIATON_REPLY
 */
#define XIM_Encoding_NameCategory   0
#define XIM_Encoding_DetailCategory 1

/*
 * value for the index of XIM_ENCODING_NEGOTIATON_REPLY
 */
#define XIM_Default_Encoding_IDX    -1

/*
 * value for the flag of XIM_FORWARD_EVENT, XIM_COMMIT
 */
#define XimSYNCHRONUS         0x0001

#define XimType_SeparatorOfNestedList   0
#define XimType_CARD8           1
#define XimType_CARD16          2
#define XimType_CARD32          3
#define XimType_STRING8         4
#define XimType_Window          5
#define XimType_XIMStyles       10
#define XimType_XRectangle      11
#define XimType_XPoint          12
#define XimType_XFontSet        13
#define XimType_XIMOptions      14
#define XimType_XIMHotKeyTriggers   15
#define XimType_XIMHotKeyState      16
#define XimType_XIMStringConversion 17
#define XimType_XIMValuesList       18
#define XimType_NEST            0x7FFF

#define _xcb_im_read_frame_with_error(IM, CLIENT, FRAME, DATA, LEN) \
    do { \
        size_t len = (LEN); \
        frame_read_func(FRAME)(&FRAME, &DATA, &len, (CLIENT)->c.byte_order != (IM)->byte_order); \
        if (!DATA) { \
            frame_free_func(FRAME)(&FRAME); \
            _xcb_im_send_error_message((IM), (CLIENT)); \
            return; \
        } \
    } while(0)

#define _xcb_im_read_frame(IM, CLIENT, FRAME, DATA, LEN) \
    do { \
        size_t len = (LEN); \
        frame_read_func(FRAME)(&FRAME, &DATA, &len, (CLIENT)->c.byte_order != (IM)->byte_order); \
        if (!DATA) { \
            frame_free_func(FRAME)(&FRAME); \
            return; \
        } \
    } while(0)

#define _xcb_im_send_frame(IM, CLIENT, FRAME, SEND_ERROR) \
    do { \
        bool fail = true; \
        bool swap = (CLIENT)->c.byte_order != (IM)->byte_order; \
        size_t length = frame_size_func(FRAME); \
        uint8_t* reply; \
        uint8_t* alloc_reply = NULL; \
        uint8_t static_reply[frame_has_static_size(FRAME) ? frame_size_func(FRAME) : 1]; \
        if (frame_has_static_size(FRAME)) { \
            reply = static_reply; \
            _xcb_write_xim_message_header(reply, frame_opcode(FRAME), 0, length, swap); \
        } else { \
            reply = _xcb_new_xim_message(frame_opcode(FRAME), 0, length, swap); \
            alloc_reply = reply; \
        } \
        do { \
            if (!reply) { \
                break; \
            } \
            frame_write_func(FRAME)(&(FRAME), reply + XCB_IM_HEADER_SIZE, swap); \
            if (!_xcb_im_send_message((IM), (CLIENT), reply, length)) { \
                break; \
            } \
            fail = false; \
        } while(0); \
        free(alloc_reply); \
        if ((SEND_ERROR) && fail) { \
            _xcb_im_send_error_message((IM), (CLIENT)); \
        } \
    } while(0)

typedef struct _xcb_im_input_context_table_t
{
    xcb_im_input_context_t ic;
    UT_hash_handle hh;
} xcb_im_input_context_table_t;

typedef struct _xcb_im_client_table_t
{
    xcb_im_client_t c;
    uint16_t icid;
    xcb_im_input_context_table_t* ic_free_list;
    xcb_im_input_context_table_t* input_contexts;
    list_head queue;

    UT_hash_handle hh1;
    UT_hash_handle hh2;
} xcb_im_client_table_t;

typedef struct _xcb_im_default_im_attr_t {
    char *name;
    uint16_t type;
    bool (*get_value)(xcb_im_t* im, xcb_im_client_table_t* client, xcb_im_ximattribute_fr_t* attr);
} xcb_im_default_im_attr_t;

bool _xcb_im_get_input_styles_attr(xcb_im_t* im, xcb_im_client_table_t* client, xcb_im_ximattribute_fr_t* attr);

static const xcb_im_default_im_attr_t Default_IMattr[] = {
    {XNQueryInputStyle,   XimType_XIMStyles, _xcb_im_get_input_styles_attr},
    /*    {XNQueryIMValuesList, XimType_XIMValuesList}, */
};

typedef struct _xcb_im_default_ic_attr_t {
    char *name;
    uint16_t type;
    bool read;
} xcb_im_default_ic_attr_t;

static const xcb_im_default_ic_attr_t Default_ICattr[] = {
    {XNInputStyle,              XimType_CARD32, false},
    {XNClientWindow,            XimType_Window, false},
    {XNFocusWindow,             XimType_Window, false},
    {XNFilterEvents,            XimType_CARD32, false},
    {XNPreeditAttributes,       XimType_NEST, false},
    {XNStatusAttributes,        XimType_NEST, false},
    {XNFontSet,                 XimType_XFontSet, false},
    {XNArea,                    XimType_XRectangle, true},
    {XNAreaNeeded,              XimType_XRectangle, true},
    {XNColormap,                XimType_CARD32, false},
    {XNStdColormap,             XimType_CARD32, false},
    {XNForeground,              XimType_CARD32, true},
    {XNBackground,              XimType_CARD32, true},
    {XNBackgroundPixmap,        XimType_CARD32, false},
    {XNSpotLocation,            XimType_XPoint, true},
    {XNLineSpace,               XimType_CARD32, true},
    {XNSeparatorofNestedList,   XimType_SeparatorOfNestedList, false},
};

struct _xcb_im_t
{
    xcb_connection_t* conn;
    char byte_order;
    xcb_im_ximattr_fr_t imattr[ARRAY_SIZE(Default_IMattr)];
    xcb_im_xicattr_fr_t icattr[ARRAY_SIZE(Default_ICattr)];
    xcb_im_ext_fr_t  extension[ARRAY_SIZE(Default_Extension)];
    uint16_t preeditAttr_id;
    uint16_t statusAttr_id;
    uint16_t separatorAttr_id;
    xcb_im_ximattr_fr_t* id2attr[ARRAY_SIZE(Default_IMattr) + ARRAY_SIZE(Default_ICattr)];
    ssize_t id2preeditoffset[ARRAY_SIZE(Default_IMattr) + ARRAY_SIZE(Default_ICattr)];
    ssize_t id2statusoffset[ARRAY_SIZE(Default_IMattr) + ARRAY_SIZE(Default_ICattr)];
    ssize_t id2icoffset[ARRAY_SIZE(Default_IMattr) + ARRAY_SIZE(Default_ICattr)];
    uint32_t event_mask;
    xcb_im_trigger_keys_t onKeys;
    xcb_im_trigger_keys_t offKeys;
    xcb_im_styles_t inputStyles;
    xcb_im_encodings_t encodings;
    char* locale;
    char* serverName;
    xcb_window_t serverWindow;
    int screen_id;
    xcb_atom_t atoms[XIM_ATOM_LAST];
    xcb_window_t root;
    xcb_im_client_table_t* free_list;
    xcb_im_client_table_t* clients_by_id;
    xcb_im_client_table_t* clients_by_win;
    uint16_t connect_id;
    xcb_screen_t* screen;
    uint32_t sequence;
    bool init;
    xcb_im_callback callback;
    void* user_data;
    bool sync;
};

typedef union _xcb_im_ic_attr_value_t
{
    xcb_rectangle_t rect;
    xcb_point_t point;
    uint32_t byte4;
    struct {
        uint8_t* data;
        uint8_t len;
    } raw;
} xcb_im_ic_attr_value_t;

typedef struct _xcb_im_queue_t
{
    uint16_t icid;
    xcb_im_packet_header_fr_t hdr;
    xcb_im_forward_event_fr_t frame;
    xcb_key_press_event_t event;
    list_head list;
} xcb_im_queue_t;

xcb_im_input_context_table_t* _xcb_im_new_input_context(xcb_im_t* im,
                                                        xcb_im_client_table_t* client);
const xcb_im_default_ic_attr_t* _xcb_im_default_ic_attr_entry(xcb_im_t* im,
                                                              uint32_t id);

bool _xcb_im_send_message(xcb_im_t* im,
                          xcb_im_client_table_t* client,
                          uint8_t* data, size_t length);


void _xcb_im_send_error_message(xcb_im_t* im,
                                xcb_im_client_table_t* client);

void _xcb_im_destroy_client(xcb_im_t* im,
                            xcb_im_client_table_t* client);
void _xcb_im_destroy_ic(xcb_im_t* im,
                        xcb_im_input_context_table_t* ic);
void _xcb_im_set_event_mask(xcb_im_t* im, xcb_im_client_table_t* client, uint32_t icid, uint32_t forward_event_mask, uint32_t sync_mask);
void _xcb_im_add_queue(xcb_im_t* im, xcb_im_client_table_t* client, uint16_t icid, const xcb_im_packet_header_fr_t* hdr, xcb_im_forward_event_fr_t* frame, uint8_t* data);
void _xcb_im_process_queue(xcb_im_t* im, xcb_im_client_table_t* client);
#endif // IMDKIT_P_H
