#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_atom.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uthash.h>
#include "imdkit.h"
#include "parser.h"
#include "ximproto.h"

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

#define ARRAY_SIZE(X) (sizeof(X) / sizeof(X[0]))

#define xim_send_frame(FRAME, frame_type, message_type) \
    do { \
        bool fail; \
        size_t length = frame_type##_size(&FRAME); \
        uint8_t* reply = _xcb_im_new_message(im, client, message_type, 0, length); \
        do { \
            if (!reply) { \
                break; \
            } \
            frame_type##_write(&FRAME, reply + XCB_IM_HEADER_SIZE, client->byte_order != im->byte_order); \
            if (!_xcb_im_send_message(im, client, reply, length)) { \
                break; \
            } \
            fail = false; \
        } while(0); \
        free(reply); \
        if (fail) { \
            _xcb_im_send_error_message(im, client); \
        } \
    } while(0)

typedef struct
{
    xcb_window_t accept_win;
    int connect_id;
    xcb_window_t client_win;
    UT_hash_handle hh1;
    UT_hash_handle hh2;
    uint8_t byte_order;
} xcb_im_client_t;

typedef struct _IMListOfAttr {
    char *name;
    uint16_t type;
} IMListOfAttr;

IMListOfAttr Default_IMattr[] = {
    {XNQueryInputStyle,   XimType_XIMStyles},
    /*    {XNQueryIMValuesList, XimType_XIMValuesList}, */
};

IMListOfAttr Default_ICattr[] = {
    {XNInputStyle,              XimType_CARD32},
    {XNClientWindow,            XimType_Window},
    {XNFocusWindow,             XimType_Window},
    {XNFilterEvents,            XimType_CARD32},
    {XNPreeditAttributes,       XimType_NEST},
    {XNStatusAttributes,        XimType_NEST},
    {XNFontSet,                 XimType_XFontSet},
    {XNArea,                    XimType_XRectangle},
    {XNAreaNeeded,              XimType_XRectangle},
    {XNColormap,                XimType_CARD32},
    {XNStdColormap,             XimType_CARD32},
    {XNForeground,              XimType_CARD32},
    {XNBackground,              XimType_CARD32},
    {XNBackgroundPixmap,        XimType_CARD32},
    {XNSpotLocation,            XimType_XPoint},
    {XNLineSpace,               XimType_CARD32},
    {XNPreeditState,            XimType_CARD32},
    {XNSeparatorofNestedList,   XimType_SeparatorOfNestedList},
};

typedef struct _xcb_im_proto_header_t {
    uint8_t   major_opcode;
    uint8_t   minor_opcode;
    uint16_t  length;
} xcb_im_proto_header_t;

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

IMExtList Default_Extension[] = {
    {"XIM_EXT_MOVE", XIM_EXTENSION, XIM_EXT_MOVE},
    {"XIM_EXT_SET_EVENT_MASK", XIM_EXTENSION, XIM_EXT_SET_EVENT_MASK},
    {"XIM_EXT_FORWARD_KEYEVENT", XIM_EXTENSION, XIM_EXT_FORWARD_KEYEVENT},
};

typedef struct _xcb_im_ext_t {
    uint16_t  major_opcode;
    uint16_t  minor_opcode;
    uint16_t  length;
    const char    *name;
} xcb_im_ext_t;

enum {
    XIM_ATOM_SERVER_NAME,
    XIM_ATOM_XIM_SERVERS,
    XIM_ATOM_LOCALES,
    XIM_ATOM_TRANSPORT,
    XIM_ATOM_XIM_PROTOCOL,
    XIM_ATOM_XIM_CONNECT,
    XIM_ATOM_LAST
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
    xcb_im_client_t* free_list;
    xcb_im_client_t* clients_by_id;
    xcb_im_client_t* clients_by_win;
    int connect_id;
    xcb_screen_t* screen;
    uint32_t sequence;
    bool init;
};

#define SIMPLE_ARRAY_FUNC(NAME, ARRAY_TYPE, ELEM_TYPE, NUM, PTR) \
void _copy_##NAME(ARRAY_TYPE* to, const ARRAY_TYPE* from) \
{ \
    if (!from || !from->NUM) { \
        to->NUM = 0; \
        to->PTR = NULL; \
        return; \
    } \
    to->PTR = malloc(from->NUM * sizeof(ELEM_TYPE)); \
    if (!to->PTR) { \
        return; \
    } \
    to->NUM = from->NUM; \
    memcpy(to->PTR, from->PTR, from->NUM * sizeof(ELEM_TYPE)); \
} \
void _free_##NAME(ARRAY_TYPE* to) \
{ \
    free(to->PTR); \
}

SIMPLE_ARRAY_FUNC(trigger_keys,
                    xcb_im_trigger_keys_t,
                    xcb_im_trigger_key_t,
                    nKeys,
                    keys)

SIMPLE_ARRAY_FUNC(input_styles,
                    xcb_im_styles_t,
                    xcb_im_style_t,
                    nStyles,
                    styles)

void _copy_encodings (xcb_im_encodings_t * to, const xcb_im_encodings_t * from)
{
    to->encodings  = malloc(from->nEncodings * sizeof(xcb_im_encoding_t));
    if (!to->encodings) {
        return;
    }
    to->nEncodings = from->nEncodings;
    for (int i = 0;i < to->nEncodings; i++) {
        to->encodings[i] = strdup(from->encodings[i]);
    }
}

void _free_encodings (xcb_im_encodings_t * to)
{
    for (int i = 0;i < to->nEncodings; i++) {
        free(to->encodings[i]);
    }
    free(to->encodings);
}

xcb_im_t* xcb_im_create(xcb_connection_t* conn,
                        int screen,
                        xcb_window_t serverWindow,
                        const char* serverName,
                        const char* locale,
                        const xcb_im_styles_t* inputStyles,
                        const xcb_im_trigger_keys_t* onKeysList,
                        const xcb_im_trigger_keys_t* offKeysList,
                        const xcb_im_encodings_t* encodingList,
                        uint32_t event_mask)
{
    xcb_im_t* im = calloc(1, sizeof(xcb_im_t));
    im->conn = conn;
    im->screen_id = screen;

    if (event_mask) {
        im->event_mask = XCB_EVENT_MASK_KEY_PRESS;
    } else {
        im->event_mask = event_mask;
    }

    _copy_trigger_keys(&im->onKeys, onKeysList);
    _copy_trigger_keys(&im->offKeys, offKeysList);
    _copy_input_styles(&im->inputStyles, inputStyles);
    _copy_encodings(&im->encodings, encodingList);
    im->locale = strdup(locale);
    im->serverName = strdup(serverName);
    im->serverWindow = serverWindow;

    uint16_t endian = 1;
    if (*(char *) &endian) {
        im->byte_order = 'l';
    } else {
        im->byte_order = 'B';
    }

    int id = 0;
    for (size_t i = 0; i < ARRAY_SIZE(Default_IMattr); i++) {
        im->imattr[i].im_attribute = (uint8_t*) Default_IMattr[i].name;
        im->imattr[i].length_of_im_attribute = strlen(Default_IMattr[i].name);
        im->imattr[i].type_of_the_value = Default_IMattr[i].type;
        im->imattr[i].attribute_ID = id++;
        im->id2attr[id] = &im->imattr[i];
    }

    for (size_t i = 0; i < ARRAY_SIZE(Default_ICattr); i++) {
        im->icattr[i].ic_attribute = (uint8_t*) Default_ICattr[i].name;
        im->icattr[i].length_of_ic_attribute = strlen(Default_ICattr[i].name);
        im->icattr[i].type_of_the_value = Default_ICattr[i].type;
        im->icattr[i].attribute_ID = id++;
        if (strcmp(Default_ICattr[i].name, XNPreeditAttributes) == 0) {
            im->preeditAttr_id = im->icattr[i].attribute_ID;
        } else if (strcmp(Default_ICattr[i].name, XNStatusAttributes) == 0) {
            im->statusAttr_id = im->icattr[i].attribute_ID;
        } else if (strcmp(Default_ICattr[i].name, XNSeparatorofNestedList) == 0) {
            im->separatorAttr_id = im->icattr[i].attribute_ID;
        }
        im->id2attr[id] = (ximattr_fr*) &im->icattr[i];
    }

    for (size_t i = 0;  i < ARRAY_SIZE(Default_Extension);  i++) {
        im->extension[i].extension_major_opcode = Default_Extension[i].major_opcode;
        im->extension[i].extension_minor_opcode = Default_Extension[i].minor_opcode;
        im->extension[i].extension_name = (uint8_t*) Default_Extension[i].name;
        im->extension[i].length_of_extension_name = strlen(Default_Extension[i].name);
    }

    return im;
}

bool _xcb_im_init(xcb_im_t* im)
{
    if (im->init) {
        return true;
    }
    xcb_screen_t* screen = xcb_aux_get_screen(im->conn, im->screen_id);

    if (!screen) {
        return false;
    }

    im->screen = screen;
    xcb_window_t root = screen->root;
    im->root = root;
    char* buf;
    asprintf(&buf, "@server=%s", im->serverName);
    const char* atom_names[] = {buf, XIM_SERVERS, XIM_LOCALES, XIM_TRANSPORT, _XIM_PROTOCOL, _XIM_XCONNECT};
    xcb_intern_atom_cookie_t atom_cookies[ARRAY_SIZE(atom_names)];
    for (size_t i = 0; i < ARRAY_SIZE(atom_names); i ++) {
        atom_cookies[i] = xcb_intern_atom(im->conn, false, strlen(atom_names[i]), atom_names[i]);
    }
    xcb_atom_t* atoms = im->atoms;
    size_t i;
    for (i = 0; i < ARRAY_SIZE(atom_names); i ++) {
        xcb_intern_atom_reply_t* atom_reply = xcb_intern_atom_reply(im->conn, atom_cookies[i], NULL);
        if (atom_reply) {
            atoms[i] = atom_reply->atom;
            free(atom_reply);
        } else {
            break;
        }
    }

    free(buf);
    if (i == ARRAY_SIZE(atom_names)) {
        im->init = true;
    }

    return im->init;
}

bool _xcb_im_set_selection_owner(xcb_im_t* im)
{
    xcb_atom_t* atoms = im->atoms;
    xcb_get_property_cookie_t cookie = xcb_get_property(im->conn,
                                                        false,
                                                        im->root,
                                                        atoms[XIM_ATOM_XIM_SERVERS],
                                                        XCB_ATOM_ATOM,
                                                        0L,
                                                        1000000L);

    xcb_get_property_reply_t* reply = xcb_get_property_reply(im->conn, cookie, NULL);
    bool found = false;
    bool conflict = false;
    bool result = false;
    do {
        if (!reply) {
            break;
        }
        if (reply->type != XCB_ATOM_NONE && (reply->type != XCB_ATOM_ATOM || reply->format != 32)) {
            break;
        }
        uint32_t* data = xcb_get_property_value(reply);
        if (!data) {
            break;
        }
        uint32_t length = xcb_get_property_value_length(reply) / sizeof(uint32_t);
        for (uint32_t i = 0; i < length ; i++) {
            if (data[i] == atoms[XIM_ATOM_SERVER_NAME]) {
                found = true;
                xcb_get_selection_owner_reply_t* owner_reply = xcb_get_selection_owner_reply(im->conn,
                                                                                             xcb_get_selection_owner(im->conn, atoms[XIM_ATOM_SERVER_NAME]),
                                                                                             NULL);
                if (owner_reply) {
                    xcb_window_t owner = owner_reply->owner;
                    free(owner_reply);
                    if (owner != im->serverWindow) {
                        if (owner == XCB_WINDOW_NONE) {
                            xcb_set_selection_owner(im->conn, im->serverWindow, atoms[XIM_ATOM_SERVER_NAME], XCB_CURRENT_TIME);
                        } else {
                            conflict = true;
                        }
                    }
                }
                break;
            }
        }

        if (conflict) {
            break;
        }

        if (!found) {
            xcb_set_selection_owner(im->conn, im->serverWindow, atoms[XIM_ATOM_SERVER_NAME], XCB_CURRENT_TIME);
            xcb_change_property(im->conn,
                                XCB_PROP_MODE_PREPEND,
                                im->root,
                                atoms[XIM_ATOM_XIM_SERVERS],
                                XCB_ATOM_ATOM,
                                32,
                                1,
                                &atoms[XIM_ATOM_SERVER_NAME]);
        } else {
            /*
            * We always need to generate the PropertyNotify to the Root Window
            */
            xcb_change_property(im->conn,
                                XCB_PROP_MODE_PREPEND,
                                im->root,
                                atoms[XIM_ATOM_XIM_SERVERS],
                                XCB_ATOM_ATOM,
                                32,
                                0,
                                data);
        }

        xcb_get_selection_owner_reply_t* owner_reply = xcb_get_selection_owner_reply(im->conn,
                                                                                     xcb_get_selection_owner(im->conn, atoms[XIM_ATOM_SERVER_NAME]),
                                                                                     NULL);
        if (owner_reply) {
            xcb_window_t owner = owner_reply->owner;
            free(owner_reply);
            result = owner == im->serverWindow;
        }
    } while(0);
    free(reply);

    return result;
}

bool xcb_im_open_im(xcb_im_t* im)
{
    if (!_xcb_im_init(im)) {
        return false;
    }

    if (!_xcb_im_set_selection_owner(im)) {
        return false;
    }

    return true;
}

xcb_im_client_t* _xcb_im_new_client(xcb_im_t* im, xcb_window_t client_window)
{
    xcb_im_client_t* client;
    int new_connect_id;
    if (im->free_list) {
        client = im->free_list;
        im->free_list = im->free_list->hh1.next;
        new_connect_id = client->connect_id;
    } else {
        client = calloc(1, sizeof(xcb_im_client_t));
        new_connect_id = ++im->connect_id;
        client->connect_id = new_connect_id;
        HASH_ADD(hh1, im->clients_by_id, connect_id, sizeof(int), client);
    }

    xcb_window_t w = xcb_generate_id (im->conn);
    xcb_create_window (im->conn, XCB_COPY_FROM_PARENT, w, im->root,
                       0, 0, 1, 1, 1,
                       XCB_WINDOW_CLASS_INPUT_OUTPUT,
                       im->screen->root_visual,
                       0, NULL);

    client->client_win = client_window;
    client->accept_win = w;
    client->byte_order = '?'; // initial value
    HASH_ADD(hh2, im->clients_by_win, accept_win, sizeof(xcb_window_t), client);

    return client;
}

bool _xcb_im_filter_xconnect_message(xcb_im_t* im, xcb_generic_event_t* event)
{
    do {
        if ((event->response_type & ~0x80) != XCB_CLIENT_MESSAGE) {
            break;
        }

        xcb_client_message_event_t* clientmessage = (xcb_client_message_event_t*) event;
        if (clientmessage->window != im->serverWindow) {
            break;
        }

        if (clientmessage->type != im->atoms[XIM_ATOM_XIM_CONNECT]) {
            break;
        }

        xcb_window_t client_window = clientmessage->data.data32[0];
        uint32_t major_version = clientmessage->data.data32[1];
        uint32_t minor_version = clientmessage->data.data32[2];

        xcb_im_client_t *client = _xcb_im_new_client(im, client_window);
        if (major_version != 0  ||  minor_version != 0) {
            major_version = minor_version = 0;
            /* Only supporting only-CM & Property-with-CM method */
        }

        xcb_client_message_event_t ev;
        ev.response_type = XCB_CLIENT_MESSAGE;
        ev.window = client_window;
        ev.type = im->atoms[XIM_ATOM_XIM_CONNECT];
        ev.sequence = 0;
        ev.format = 32;
        ev.data.data32[0] = client->accept_win;
        ev.data.data32[1] = major_version;
        ev.data.data32[2] = minor_version;
        ev.data.data32[3] = XCM_DATA_LIMIT;
        xcb_send_event(im->conn, false, client_window, XCB_EVENT_MASK_NO_EVENT, (char*) &ev);
        xcb_flush(im->conn);

        return true;
    } while(0);

    return false;
}

bool _xcb_im_filter_selection_request(xcb_im_t* im, xcb_generic_event_t* event)
{
    do {
        if ((event->response_type & ~0x80) != XCB_SELECTION_REQUEST) {
            break;
        }

        xcb_selection_request_event_t* selection_request = (xcb_selection_request_event_t*) event;
        if (selection_request->owner != im->serverWindow) {
            break;
        }

        if (selection_request->selection != im->atoms[XIM_ATOM_SERVER_NAME]) {
            break;
        }
        char buf[LOCALES_BUFSIZE];
        if (selection_request->target == im->atoms[XIM_ATOM_LOCALES]) {
            snprintf(buf, LOCALES_BUFSIZE - 2, "@locale=%s", im->locale);
            buf[LOCALES_BUFSIZE-1] = 0;
        } else if (selection_request->target == im->atoms[XIM_ATOM_TRANSPORT]) {
            strcpy(buf, "@transport=X/");
        } else {
            return true;
        }
        DebugLog("requestor:%u\n", selection_request->requestor);
        DebugLog("buf:%s\n", buf);
        xcb_change_property(im->conn,
                            XCB_PROP_MODE_REPLACE,
                            selection_request->requestor,
                            selection_request->target,
                            selection_request->target,
                            8,
                            strlen(buf),
                            (unsigned char*)buf);

        xcb_selection_notify_event_t ev;
        ev.sequence = 0;
        ev.response_type = XCB_SELECTION_NOTIFY;
        ev.requestor = selection_request->requestor;
        ev.selection = selection_request->selection;
        ev.target = selection_request->target;
        ev.time = selection_request->time;
        ev.property = selection_request->property;
        xcb_send_event(im->conn, false, ev.requestor, XCB_EVENT_MASK_NO_EVENT, (char*) &ev);
        xcb_flush(im->conn);

        return true;
    } while(0);

    return false;
}

static uint8_t* _xcb_im_read_message(xcb_im_t* im,
                                     xcb_client_message_event_t *ev,
                                     xcb_im_client_t* client,
                                     xcb_im_proto_header_t* hdr)
{
    uint8_t *p = NULL;

    if (ev->format == 8) {
        if (client->byte_order == '?') {
            // major_opcode
            if (ev->data.data8[0] != XIM_CONNECT) {
                return (unsigned char *) NULL;  /* can do nothing */
            }
            client->byte_order = ev->data.data8[XCB_IM_HEADER_SIZE];
        }
        /* ClientMessage only */
        uint8_t* rec = ev->data.data8;
        size_t len  = sizeof(ev->data.data8);
        uint8_t_read(&hdr->major_opcode, &rec, &len, client->byte_order != im->byte_order);
        uint8_t_read(&hdr->minor_opcode, &rec, &len, client->byte_order != im->byte_order);
        uint16_t_read(&hdr->length, &rec, &len, client->byte_order != im->byte_order);

        // check message is well formed
        if (len >= hdr->length * 4) {
            p = malloc(hdr->length * 4);
        }
        if (p) {
            memcpy(p, rec, hdr->length * 4);
        }
    } else if (ev->format == 32) {
        /* ClientMessage and WindowProperty */
        size_t length = ev->data.data32[0];
        xcb_atom_t atom = ev->data.data32[1];

        xcb_get_property_cookie_t cookie = xcb_get_property(im->conn,
                                                           true,
                                                           client->accept_win,
                                                           atom,
                                                           XCB_ATOM_ANY,
                                                           0L,
                                                           length);

        xcb_get_property_reply_t* reply = xcb_get_property_reply(im->conn, cookie, NULL);
        uint8_t* rec;

        do {
            if (!reply || reply->format == 0 || reply->length == 0) {
                free(reply);
                return (unsigned char *) NULL;
            }

            rec = xcb_get_property_value(reply);

            if (length != reply->value_len)
                length = reply->value_len;

            // make length into byte
            if (reply->format == 16)
                length *= 2;
            else if (reply->format == 32)
                length *= 4;

            uint8_t_read(&hdr->major_opcode, &rec, &length, client->byte_order != im->byte_order);
            uint8_t_read(&hdr->minor_opcode, &rec, &length, client->byte_order != im->byte_order);
            uint16_t_read(&hdr->length, &rec, &length, client->byte_order != im->byte_order);

            // check message is well formed
            if (hdr->length * 4 <= length) {
                /* if hit, it might be an error */
                p = malloc(hdr->length * 4);
            }
        } while(0);

        if (p) {
            memcpy(p, rec, hdr->length * 4);
        }
        free(reply);
    }
    return (unsigned char *) p;
}

void _xcb_im_write_message_header(xcb_im_t* im,
                                  xcb_im_client_t* client,
                                  uint8_t* message,
                                  uint8_t major_opcode,
                                  uint8_t minor_opcode,
                                  size_t length)
{
    uint16_t p_len = length / 4;
    message = uint8_t_write(&major_opcode, message, client->byte_order != im->byte_order);
    message = uint8_t_write(&minor_opcode, message, client->byte_order != im->byte_order);
    message = uint16_t_write(&p_len, message, client->byte_order != im->byte_order);
}

uint8_t* _xcb_im_new_message(xcb_im_t* im,
                             xcb_im_client_t* client,
                             uint8_t major_opcode,
                             uint8_t minor_opcode,
                             size_t length)
{
    uint8_t* message = calloc(length + XCB_IM_HEADER_SIZE, 1);
    if (message) {
        _xcb_im_write_message_header(im, client, message, major_opcode, minor_opcode, length);
    }

    return message;
}

// length is the body without header size in byte
bool _xcb_im_send_message(xcb_im_t* im,
                          xcb_im_client_t* client,
                          uint8_t* data, size_t length)
{
    if (!data) {
        return false;
    }

    // add header size
    length += XCB_IM_HEADER_SIZE;

    xcb_client_message_event_t event;

    event.response_type = XCB_CLIENT_MESSAGE;
    event.sequence = 0;
    event.window = client->client_win;
    event.type = im->atoms[XIM_ATOM_XIM_PROTOCOL];

    if (length > XCM_DATA_LIMIT) {
        xcb_atom_t atom;
        char atomName[64];

        int len = sprintf(atomName, "_server%u_%u", client->connect_id, im->sequence++);

        xcb_intern_atom_cookie_t atom_cookie = xcb_intern_atom(im->conn, false, len, atomName);
        xcb_intern_atom_reply_t* atom_reply = xcb_intern_atom_reply(im->conn, atom_cookie, NULL);
        if (!atom_reply) {
            return false;
        }
        atom = atom_reply->atom;
        free(atom_reply);
        xcb_get_property_cookie_t get_property_cookie = xcb_get_property(im->conn,
                                                                         false,
                                                                         client->client_win,
                                                                         atom,
                                                                         XCB_ATOM_STRING,
                                                                         0L,
                                                                         10000L);

        xcb_get_property_reply_t* get_property_reply = xcb_get_property_reply(im->conn, get_property_cookie, NULL);
        if (!get_property_reply) {
            return false;
        }
        free(get_property_reply);
        xcb_void_cookie_t cookie = xcb_change_property_checked(im->conn,
                                            XCB_PROP_MODE_APPEND,
                                            client->client_win,
                                            atom,
                                            XCB_ATOM_STRING,
                                            8,
                                            length,
                                            data);
        xcb_generic_error_t* error = NULL;
        if ((error = xcb_request_check(im->conn, cookie)) != NULL) {
            DebugLog("Error code: %d", error->error_code);
            free(error);
        }
        event.format = 32;
        event.data.data32[0] = length;
        event.data.data32[1] = atom;
        for (size_t i = 2; i < ARRAY_SIZE(event.data.data32); i++)
            event.data.data32[i] = 0;
    } else {
        event.format = 8;

        memcpy(event.data.data8, data, length);
        /* Clear unused field with NULL */
        for (size_t i = length; i < XCM_DATA_LIMIT; i++)
            event.data.data8[i] = 0;
    }
    xcb_send_event(im->conn, false, client->client_win, XCB_EVENT_MASK_NO_EVENT, (const char*) &event);
    xcb_flush(im->conn);
    return true;
}

void _xcb_im_send_error_message(xcb_im_t* im,
                                xcb_im_client_t* client)
{
    // use stack to avoid alloc fails
    uint8_t message[XCB_IM_HEADER_SIZE];
    _xcb_im_write_message_header(im, client, message, XIM_ERROR, 0, 0);
    _xcb_im_send_message(im, client, message, 0);
}

void _xcb_im_handle_connect(xcb_im_t* im,
                            xcb_im_client_t* client,
                            const xcb_im_proto_header_t* hdr,
                            uint8_t* data)
{
    size_t len = XIM_MESSAGE_BYTES(hdr);
    connect_fr frame;
    connect_fr_read(&frame, &data, &len, client->byte_order != im->byte_order);
    if (!data) {
        return;
    }

    connect_reply_fr reply_frame;
    reply_frame.server_major_protocol_version = frame.client_major_protocol_version;
    reply_frame.server_minor_protocol_version = frame.client_minor_protocol_version;

    xim_send_frame(reply_frame, connect_reply_fr, XIM_CONNECT_REPLY);
    return;
}

void _xcb_im_send_trigger_key(xcb_im_t* im, xcb_im_client_t* client)
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
                         xcb_im_client_t* client,
                         const xcb_im_proto_header_t* hdr,
                         uint8_t* data,
                         bool *del)
{
    size_t len = XIM_MESSAGE_BYTES(hdr);
    open_fr frame;
    memset(&frame, 0, sizeof(open_fr));
    open_fr_read(&frame, &data, &len, client->byte_order != im->byte_order);
    if (!data) {
        open_fr_free(&frame);
        return;
    }

    open_fr_free(&frame);
    /*endif*/
    if (im->onKeys.nKeys || im->offKeys.nKeys) {
        _xcb_im_send_trigger_key(im, client);
    }

    open_reply_fr reply_frame;
    reply_frame.input_method_ID = client->connect_id;
    reply_frame.IM_attribute_supported.size = ARRAY_SIZE(im->imattr);
    reply_frame.IC_attribute_supported.size = ARRAY_SIZE(im->icattr);
    reply_frame.IM_attribute_supported.items = im->imattr;
    reply_frame.IC_attribute_supported.items = im->icattr;

    xim_send_frame(reply_frame, open_reply_fr, XIM_OPEN_REPLY);
}

void _xcb_im_handle_close(xcb_im_t* im,
                          xcb_im_client_t* client,
                          const xcb_im_proto_header_t* hdr,
                          uint8_t* data,
                          bool *del)
{
    size_t len = XIM_MESSAGE_BYTES(hdr);
    close_fr frame;
    close_fr_read(&frame, &data, &len, client->byte_order != im->byte_order);
    if (!data) {
        return;
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
        close_reply_fr_write(&reply_frame, reply + XCB_IM_HEADER_SIZE, client->byte_order != im->byte_order);
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
                                    xcb_im_client_t* client,
                                    const xcb_im_proto_header_t* hdr,
                                    uint8_t* data,
                                    bool *del)
{
    size_t len = XIM_MESSAGE_BYTES(hdr);
    query_extension_fr frame;
    query_extension_fr_read(&frame, &data, &len, client->byte_order != im->byte_order);
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
    reply_frame.input_method_ID = client->connect_id;
    reply_frame.list_of_extensions_supported_by_th.items = ext_list;
    reply_frame.list_of_extensions_supported_by_th.size = nExts;

    xim_send_frame(reply_frame, query_extension_reply_fr, XIM_QUERY_EXTENSION_REPLY);
}

void _xcb_im_handle_encoding_negotiation(xcb_im_t* im,
                                         xcb_im_client_t* client,
                                         const xcb_im_proto_header_t* hdr,
                                         uint8_t* data,
                                         bool *del)
{
    size_t len = XIM_MESSAGE_BYTES(hdr);
    encoding_negotiation_fr frame;
    memset(&frame, 0, sizeof(frame));
    encoding_negotiation_fr_read(&frame, &data, &len, client->byte_order != im->byte_order);
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
    reply_frame.input_method_ID = client->connect_id;
    reply_frame.index_of_the_encoding_dterminated = i;
    reply_frame.category_of_the_encoding_determined = 0;

    xim_send_frame(reply_frame, encoding_negotiation_reply_fr, XIM_ENCODING_NEGOTIATION_REPLY);
}

void _xcb_im_handle_message(xcb_im_t* im,
                            xcb_im_client_t* client,
                            const xcb_im_proto_header_t* hdr,
                            uint8_t* data,
                            bool *del)
{
    switch (hdr->major_opcode) {
    case XIM_CONNECT:
        DebugLog("-- XIM_CONNECT\n");
        _xcb_im_handle_connect(im, client, hdr, data);
        break;

    case XIM_DISCONNECT:
        DebugLog("-- XIM_DISCONNECT\n");
        break;

    case XIM_OPEN:
        DebugLog("-- XIM_OPEN\n");
        _xcb_im_handle_open(im, client, hdr, data, del);
        break;

    case XIM_CLOSE:
        DebugLog("-- XIM_CLOSE\n");
        _xcb_im_handle_close(im, client, hdr, data, del);
        break;

    case XIM_QUERY_EXTENSION:
        DebugLog("-- XIM_QUERY_EXTENSION\n");
        _xcb_im_handle_query_extension(im, client, hdr, data, del);
        break;

    case XIM_GET_IM_VALUES:
        DebugLog("-- XIM_GET_IM_VALUES\n");
        break;

    case XIM_CREATE_IC:
        DebugLog("-- XIM_CREATE_IC\n");
        break;

    case XIM_SET_IC_VALUES:
        DebugLog("-- XIM_SET_IC_VALUES\n");
        break;

    case XIM_GET_IC_VALUES:
        DebugLog("-- XIM_GET_IC_VALUES\n");
        break;

    case XIM_SET_IC_FOCUS:
        DebugLog("-- XIM_SET_IC_FOCUS\n");
        break;

    case XIM_UNSET_IC_FOCUS:
        DebugLog("-- XIM_UNSET_IC_FOCUS\n");
        break;

    case XIM_DESTROY_IC:
        DebugLog("-- XIM_DESTROY_IC\n");
        break;

    case XIM_RESET_IC:
        DebugLog("-- XIM_RESET_IC\n");
        break;

    case XIM_FORWARD_EVENT:
        DebugLog("-- XIM_FORWARD_EVENT\n");
        break;

    case XIM_EXTENSION:
        DebugLog("-- XIM_EXTENSION\n");
        break;

    case XIM_SYNC:
        DebugLog("-- XIM_SYNC\n");
        break;

    case XIM_SYNC_REPLY:
        DebugLog("-- XIM_SYNC_REPLY\n");
        break;

    case XIM_TRIGGER_NOTIFY:
        DebugLog("-- XIM_TRIGGER_NOTIFY\n");
        break;

    case XIM_ENCODING_NEGOTIATION:
        DebugLog("-- XIM_ENCODING_NEGOTIATION\n");
        _xcb_im_handle_encoding_negotiation(im, client, hdr, data, del);
        break;

    case XIM_PREEDIT_START_REPLY:
        DebugLog("-- XIM_PREEDIT_START_REPLY\n");
        break;

    case XIM_PREEDIT_CARET_REPLY:
        DebugLog("-- XIM_PREEDIT_CARET_REPLY\n");
        break;

    case XIM_STR_CONVERSION_REPLY:
        DebugLog("-- XIM_STR_CONVERSION_REPLY\n");
        break;
    }
    /*endswitch*/
}

bool _xcb_im_filter_client(xcb_im_t* im, xcb_generic_event_t* event)
{
    do {
        if ((event->response_type & ~0x80) != XCB_CLIENT_MESSAGE) {
            break;
        }

        xcb_client_message_event_t* clientmessage = (xcb_client_message_event_t*) event;

        if (clientmessage->type != im->atoms[XIM_ATOM_XIM_PROTOCOL]) {
            break;
        }

        xcb_im_client_t* client = NULL;
        HASH_FIND(hh2, im->clients_by_win, &clientmessage->window, sizeof(xcb_window_t), client);
        if (!client) {
            break;
        }

        bool del = true;
        xcb_im_proto_header_t hdr;
        uint8_t* message = _xcb_im_read_message(im, clientmessage, client, &hdr);
        if (message) {
            _xcb_im_handle_message(im, client, &hdr, message, &del);
            if (del) {
                free(message);
            }
        }

        return true;

    } while(0);

    return false;
}

bool xcb_im_filter_event(xcb_im_t* im, xcb_generic_event_t* event)
{
    return _xcb_im_filter_xconnect_message(im, event)
        || _xcb_im_filter_selection_request(im, event)
        || _xcb_im_filter_client(im, event);
}

void xcb_im_close_im(xcb_im_t* im)
{
    xcb_atom_t* atoms = im->atoms;
    xcb_get_property_cookie_t cookie = xcb_get_property(im->conn,
                                                        false,
                                                        im->root,
                                                        atoms[XIM_ATOM_XIM_SERVERS],
                                                        XCB_ATOM_ATOM,
                                                        0L,
                                                        1000000L);

    xcb_get_property_reply_t* reply = xcb_get_property_reply(im->conn, cookie, NULL);
    bool found = false;
    do {
        if (!reply) {
            break;
        }
        if (reply->type != XCB_ATOM_NONE && (reply->type != XCB_ATOM_ATOM || reply->format != 32)) {
            break;
        }
        long* data = xcb_get_property_value(reply);
        if (!data) {
            break;
        }
        uint32_t length = xcb_get_property_value_length(reply) / sizeof(uint32_t);
        uint32_t i;
        for (i = 0; i < length; i++) {
            if (data[i] == atoms[XIM_ATOM_SERVER_NAME]) {
                found = true;
                break;
            }
        }

        if (found) {
            for (i = i + 1; i < length; i++) {
                data[i - 1] = data[i];
            }
            xcb_change_property(im->conn,
                                XCB_PROP_MODE_REPLACE,
                                im->root,
                                atoms[XIM_ATOM_XIM_SERVERS],
                                XCB_ATOM_ATOM,
                                32,
                                length - 1,
                                data);
        } else {
            /*
            * We always need to generate the PropertyNotify to the Root Window
            */
            xcb_change_property(im->conn,
                                XCB_PROP_MODE_PREPEND,
                                im->root,
                                atoms[XIM_ATOM_XIM_SERVERS],
                                XCB_ATOM_ATOM,
                                32,
                                0,
                                data);
        }
    } while(0);
    free(reply);
    while (im->clients_by_win) {
        HASH_DELETE(hh2, im->clients_by_win, im->clients_by_win);
    }
    while (im->clients_by_id) {
        xcb_im_client_t* client = im->clients_by_id;
        HASH_DELETE(hh1, im->clients_by_id, im->clients_by_id);
        xcb_destroy_window(im->conn, client->accept_win);
        free(client);
    }
    im->connect_id = 0;
}

void xcb_im_destory(xcb_im_t* im)
{
    _free_encodings(&im->encodings);
    _free_trigger_keys(&im->onKeys);
    _free_trigger_keys(&im->offKeys);
    _free_input_styles(&im->inputStyles);
    free(im);
}

void xcb_im_forward_event(xcb_im_t* im, xcb_key_press_event_t* event)
{
}


void xcb_im_comming_string(xcb_im_t* im)
{
}

void xcb_im_preedit_start(xcb_im_t* im)
{
}

void xcb_im_preedit_end(xcb_im_t* im)
{
}

void xcb_im_sync_xlib(xcb_im_t* im)
{
}
