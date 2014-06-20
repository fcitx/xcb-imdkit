#ifndef IMDKIT_P_H
#define IMDKIT_P_H

#include "imdkit.h"
#include "uthash.h"

#define ARRAY_SIZE(X) (sizeof(X) / sizeof(X[0]))

#define XIM_MESSAGE_BYTES(hdr) ((hdr)->length * 4)


#define XIM_DEBUG
#ifdef XIM_DEBUG
#define DebugLog(S...) fprintf(stderr, S)
#else
#define DebugLog(S...) ((void) (S))
#endif

#define LOCALES_BUFSIZE (sizeof(XCB_IM_ALL_LOCALES) + 32)

#define XIM_SERVERS     "XIM_SERVERS"
#define XIM_LOCALES     "LOCALES"
#define XIM_TRANSPORT       "TRANSPORT"
#define _XIM_PROTOCOL           "_XIM_PROTOCOL"
#define _XIM_XCONNECT           "_XIM_XCONNECT"

/*
 * categories in XIM_SERVERS
 */
#define XIM_SERVER_CATEGORY "@server="
#define XIM_LOCAL_CATEGORY  "@locale="
#define XIM_TRANSPORT_CATEGORY  "@transport="

/*
 * Xim implementation revision
 */
#define PROTOCOLMAJORVERSION        0
#define PROTOCOLMINORVERSION        0

/*
 * Major Protocol number
 */
#define XIM_CONNECT           1
#define XIM_CONNECT_REPLY         2
#define XIM_DISCONNECT            3
#define XIM_DISCONNECT_REPLY          4

#define XIM_AUTH_REQUIRED        10
#define XIM_AUTH_REPLY           11
#define XIM_AUTH_NEXT            12
#define XIM_AUTH_SETUP           13
#define XIM_AUTH_NG          14

#define XIM_ERROR            20

#define XIM_OPEN             30
#define XIM_OPEN_REPLY           31
#define XIM_CLOSE            32
#define XIM_CLOSE_REPLY          33
#define XIM_REGISTER_TRIGGERKEYS     34
#define XIM_TRIGGER_NOTIFY       35
#define XIM_TRIGGER_NOTIFY_REPLY     36
#define XIM_SET_EVENT_MASK       37
#define XIM_ENCODING_NEGOTIATION     38
#define XIM_ENCODING_NEGOTIATION_REPLY   39
#define XIM_QUERY_EXTENSION      40
#define XIM_QUERY_EXTENSION_REPLY    41
#define XIM_SET_IM_VALUES        42
#define XIM_SET_IM_VALUES_REPLY      43
#define XIM_GET_IM_VALUES        44
#define XIM_GET_IM_VALUES_REPLY      45

#define XIM_CREATE_IC            50
#define XIM_CREATE_IC_REPLY      51
#define XIM_DESTROY_IC           52
#define XIM_DESTROY_IC_REPLY         53
#define XIM_SET_IC_VALUES        54
#define XIM_SET_IC_VALUES_REPLY      55
#define XIM_GET_IC_VALUES        56
#define XIM_GET_IC_VALUES_REPLY      57
#define XIM_SET_IC_FOCUS         58
#define XIM_UNSET_IC_FOCUS       59
#define XIM_FORWARD_EVENT        60
#define XIM_SYNC             61
#define XIM_SYNC_REPLY           62
#define XIM_COMMIT           63
#define XIM_RESET_IC             64
#define XIM_RESET_IC_REPLY       65

#define XIM_GEOMETRY             70
#define XIM_STR_CONVERSION       71
#define XIM_STR_CONVERSION_REPLY     72
#define XIM_PREEDIT_START        73
#define XIM_PREEDIT_START_REPLY      74
#define XIM_PREEDIT_DRAW         75
#define XIM_PREEDIT_CARET        76
#define XIM_PREEDIT_CARET_REPLY      77
#define XIM_PREEDIT_DONE         78
#define XIM_STATUS_START         79
#define XIM_STATUS_DRAW          80
#define XIM_STATUS_DONE          81

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
 * values for the type of XIMATTR & XICATTR
 */
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
#define XimLookupChars        0x0002
#define XimLookupKeySym       0x0004
#define XimLookupBoth         0x0006


/*
 * Client Message data size
 */
#define XIM_CM_DATA_SIZE    20

#define XCM_DATA_LIMIT      20

#define XNVaNestedList "XNVaNestedList"
#define XNQueryInputStyle "queryInputStyle"
#define XNClientWindow "clientWindow"
#define XNInputStyle "inputStyle"
#define XNFocusWindow "focusWindow"
#define XNResourceName "resourceName"
#define XNResourceClass "resourceClass"
#define XNGeometryCallback "geometryCallback"
#define XNDestroyCallback "destroyCallback"
#define XNFilterEvents "filterEvents"
#define XNPreeditStartCallback "preeditStartCallback"
#define XNPreeditDoneCallback "preeditDoneCallback"
#define XNPreeditDrawCallback "preeditDrawCallback"
#define XNPreeditCaretCallback "preeditCaretCallback"
#define XNPreeditStateNotifyCallback "preeditStateNotifyCallback"
#define XNPreeditAttributes "preeditAttributes"
#define XNStatusStartCallback "statusStartCallback"
#define XNStatusDoneCallback "statusDoneCallback"
#define XNStatusDrawCallback "statusDrawCallback"
#define XNStatusAttributes "statusAttributes"
#define XNArea "area"
#define XNAreaNeeded "areaNeeded"
#define XNSpotLocation "spotLocation"
#define XNColormap "colorMap"
#define XNStdColormap "stdColorMap"
#define XNForeground "foreground"
#define XNBackground "background"
#define XNBackgroundPixmap "backgroundPixmap"
#define XNFontSet "fontSet"
#define XNLineSpace "lineSpace"
#define XNCursor "cursor"

#define XNQueryIMValuesList "queryIMValuesList"
#define XNQueryICValuesList "queryICValuesList"
#define XNVisiblePosition "visiblePosition"
#define XNR6PreeditCallback "r6PreeditCallback"
#define XNStringConversionCallback "stringConversionCallback"
#define XNStringConversion "stringConversion"
#define XNResetState "resetState"
#define XNHotKey "hotKey"
#define XNHotKeyState "hotKeyState"
#define XNPreeditState "preeditState"
#define XNSeparatorofNestedList "separatorofNestedList"

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

#define _xcb_im_read_frame(IM, CLIENT, FRAME) \
    do { \
        size_t len = XIM_MESSAGE_BYTES(hdr); \
        frame_read_func(FRAME)(&FRAME, &data, &len, (CLIENT)->c.byte_order != (IM)->byte_order); \
        if (!data) { \
            frame_free_func(FRAME)(&FRAME); \
            _xcb_im_send_error_message((IM), (CLIENT)); \
            return; \
        } \
    } while(0)

#define frame_opcode(FRAME) _Generic((FRAME), \
    connect_reply_fr: XIM_CONNECT_REPLY, \
    open_reply_fr: XIM_OPEN_REPLY, \
    close_reply_fr: XIM_CLOSE_REPLY, \
    query_extension_reply_fr: XIM_QUERY_EXTENSION_REPLY, \
    encoding_negotiation_reply_fr: XIM_ENCODING_NEGOTIATION_REPLY, \
    get_im_values_reply_fr: XIM_GET_IM_VALUES_REPLY, \
    register_triggerkeys_fr: XIM_REGISTER_TRIGGERKEYS \
    )

#define _xcb_im_send_frame(IM, CLIENT, FRAME) \
    do { \
        bool fail; \
        size_t length = frame_size_func(FRAME)(&FRAME); \
        uint8_t* reply = _xcb_im_new_message((IM), (CLIENT), frame_opcode(FRAME), 0, length); \
        do { \
            if (!reply) { \
                break; \
            } \
            frame_write_func(FRAME)(&FRAME, reply + XCB_IM_HEADER_SIZE, (CLIENT)->c.byte_order != (IM)->byte_order); \
            if (!_xcb_im_send_message((IM), (CLIENT), reply, length)) { \
                break; \
            } \
            fail = false; \
        } while(0); \
        free(reply); \
        if (fail) { \
            _xcb_im_send_error_message((IM), (CLIENT)); \
        } \
    } while(0)

typedef struct _xcb_im_client_table_t
{
    xcb_im_client_t c;
    UT_hash_handle hh1;
    UT_hash_handle hh2;
} xcb_im_client_table_t;

enum {
    XIM_ATOM_SERVER_NAME,
    XIM_ATOM_XIM_SERVERS,
    XIM_ATOM_LOCALES,
    XIM_ATOM_TRANSPORT,
    XIM_ATOM_XIM_PROTOCOL,
    XIM_ATOM_XIM_CONNECT,
    XIM_ATOM_LAST
};


typedef struct _IMListOfAttr {
    char *name;
    uint16_t type;
    bool (*get_value)(xcb_im_t* im, xcb_im_client_table_t* client, ximattribute_fr* attr);
    bool (*set_value)(xcb_im_t* im, xcb_im_client_table_t* client, uint8_t* data, size_t length);
} IMListOfAttr;

bool _xcb_im_get_input_styles_attr(xcb_im_t* im, xcb_im_client_table_t* client, ximattribute_fr* attr);

static const IMListOfAttr Default_IMattr[] = {
    {XNQueryInputStyle,   XimType_XIMStyles, _xcb_im_get_input_styles_attr, NULL},
    /*    {XNQueryIMValuesList, XimType_XIMValuesList}, */
};

static const IMListOfAttr Default_ICattr[] = {
    {XNInputStyle,              XimType_CARD32, NULL, NULL},
    {XNClientWindow,            XimType_Window, NULL, NULL},
    {XNFocusWindow,             XimType_Window, NULL, NULL},
    {XNFilterEvents,            XimType_CARD32, NULL, NULL},
    {XNPreeditAttributes,       XimType_NEST, NULL, NULL},
    {XNStatusAttributes,        XimType_NEST, NULL, NULL},
    {XNFontSet,                 XimType_XFontSet, NULL, NULL},
    {XNArea,                    XimType_XRectangle, NULL, NULL},
    {XNAreaNeeded,              XimType_XRectangle, NULL, NULL},
    {XNColormap,                XimType_CARD32, NULL, NULL},
    {XNStdColormap,             XimType_CARD32, NULL, NULL},
    {XNForeground,              XimType_CARD32, NULL, NULL},
    {XNBackground,              XimType_CARD32, NULL, NULL},
    {XNBackgroundPixmap,        XimType_CARD32, NULL, NULL},
    {XNSpotLocation,            XimType_XPoint, NULL, NULL},
    {XNLineSpace,               XimType_CARD32, NULL, NULL},
    {XNPreeditState,            XimType_CARD32, NULL, NULL},
    {XNSeparatorofNestedList,   XimType_SeparatorOfNestedList, NULL, NULL},
};

typedef struct {
    char *name;
    uint8_t major_opcode;
    uint8_t minor_opcode;
} IMExtList;

/*
 * Minor Protocol Number for Extension Protocol
 */
#define XIM_EXTENSION               128
#define XIM_EXT_SET_EVENT_MASK          (0x30)
#define XIM_EXT_FORWARD_KEYEVENT        (0x32)
#define XIM_EXT_MOVE                (0x33)

static const IMExtList Default_Extension[] = {
    {"XIM_EXT_MOVE", XIM_EXTENSION, XIM_EXT_MOVE},
    {"XIM_EXT_SET_EVENT_MASK", XIM_EXTENSION, XIM_EXT_SET_EVENT_MASK},
    {"XIM_EXT_FORWARD_KEYEVENT", XIM_EXTENSION, XIM_EXT_FORWARD_KEYEVENT},
};

struct _xcb_im_t
{
    xcb_connection_t* conn;
    char byte_order;
    ximattr_fr imattr[ARRAY_SIZE(Default_IMattr)];
    xicattr_fr icattr[ARRAY_SIZE(Default_ICattr)];
    ext_fr  extension[ARRAY_SIZE(Default_Extension)];
    uint16_t preeditAttr_id;
    uint16_t statusAttr_id;
    uint16_t separatorAttr_id;
    ximattr_fr* id2attr[ARRAY_SIZE(Default_IMattr) + ARRAY_SIZE(Default_ICattr)];
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
    int connect_id;
    xcb_screen_t* screen;
    uint32_t sequence;
    bool init;
    xcb_im_callback callback;
    void* user_data;
};

#endif // IMDKIT_P_H
