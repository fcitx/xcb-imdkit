#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/xcb_aux.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uthash.h>
#include "imdkit.h"
#include "parser.h"
#include "ximproto.h"

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
 * request packet header size
 */
#define XIM_HEADER_SIZE                     \
    sizeof(CARD8)     /* sizeof mejor-opcode */   \
    + sizeof(CARD8)     /* sizeof minor-opcode */   \
    + sizeof(INT16)     /* sizeof length */

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

typedef struct _xcb_im_attr_t {
    uint16_t  attribute_id;
    uint16_t  type;
    uint16_t  length;
    const char    *name;
} xcb_im_attr_t;

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
    xcb_im_attr_t imattr[ARRAY_SIZE(Default_IMattr)];
    xcb_im_attr_t icattr[ARRAY_SIZE(Default_ICattr)];
    xcb_im_ext_t  extension[ARRAY_SIZE(Default_Extension)];
    uint16_t preeditAttr_id;
    uint16_t statusAttr_id;
    uint16_t separatorAttr_id;
    xcb_im_attr_t* id2attr[ARRAY_SIZE(Default_IMattr) + ARRAY_SIZE(Default_ICattr)];
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
        im->imattr[i].name = Default_IMattr[i].name;
        im->imattr[i].length = strlen(Default_IMattr[i].name);
        im->imattr[i].type = Default_IMattr[i].type;
        im->imattr[i].attribute_id = id++;
        im->id2attr[id] = &im->imattr[i];
    }

    for (size_t i = 0; i < ARRAY_SIZE(Default_ICattr); i++) {
        im->icattr[i].name = Default_ICattr[i].name;
        im->icattr[i].length = strlen(Default_ICattr[i].name);
        im->icattr[i].type = Default_ICattr[i].type;
        im->icattr[i].attribute_id = id++;
        if (strcmp(im->imattr[i].name, XNPreeditAttributes) == 0) {
            im->preeditAttr_id = im->icattr[i].attribute_id;
        } else if (strcmp(im->imattr[i].name, XNStatusAttributes) == 0) {
            im->statusAttr_id = im->icattr[i].attribute_id;
        } else if (strcmp(im->imattr[i].name, XNSeparatorofNestedList) == 0) {
            im->separatorAttr_id = im->icattr[i].attribute_id;
        }
        im->id2attr[id] = &im->icattr[i];
    }

    for (size_t i = 0;  i < ARRAY_SIZE(Default_Extension);  i++) {
        im->extension[i].major_opcode = Default_Extension[i].major_opcode;
        im->extension[i].minor_opcode = Default_Extension[i].minor_opcode;
        im->extension[i].name = Default_Extension[i].name;
        im->extension[i].length = strlen(Default_Extension[i].name);
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
        long* data = xcb_get_property_value(reply);
        if (!data) {
            break;
        }
        int length = xcb_get_property_value_length(reply);
        for (int i = 0; i < length; i++) {
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
        new_connect_id = im->connect_id++;
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

static uint8_t* ReadXIMMessage(xcb_im_t* im,
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
            client->byte_order = ev->data.data8[4];
        }
        /* ClientMessage only */
        uint8_t* rec = parse_binary_with_format(ev->data.data8, client->byte_order != im->byte_order,
                                                packet_header_fr,
                                                &hdr->major_opcode,
                                                &hdr->minor_opcode,
                                                &hdr->length);

        p = malloc(hdr->length * 4);
        if (p) {
            memcpy(p, rec, hdr->length * 4);
        }
    } else if (ev->format == 32) {
        /* ClientMessage and WindowProperty */
        uint32_t length = ev->data.data32[0];
        xcb_atom_t atom = ev->data.data32[1];

        xcb_get_property_cookie_t cookie = xcb_get_property(im->conn,
                                                           true,
                                                           client->accept_win,
                                                           atom,
                                                           XCB_ATOM_ANY,
                                                           0L,
                                                           length);

        xcb_get_property_reply_t* reply = xcb_get_property_reply(im->conn, cookie, NULL);

        do {
            if (!reply || reply->format == 0 || reply->length == 0) {
                free(reply);
                return (unsigned char *) NULL;
            }
            if (length != reply->length)
                length = reply->length;
            if (reply->format == 16)
                length *= 2;
            else if (reply->format == 32)
                length *= 4;

            /* if hit, it might be an error */
            p = malloc(length);
        } while(0);

        if (p) {
            uint8_t* rec = xcb_get_property_value(reply);
            rec = parse_binary_with_format(rec, client->byte_order != im->byte_order,
                                           packet_header_fr,
                                           &hdr->major_opcode,
                                           &hdr->minor_opcode,
                                           &hdr->length);
            memcpy(p, rec, length);
        }
        free(reply);
    }
    return (unsigned char *) p;
}
void _xcb_im_handle_message(xcb_im_t* im,
                            xcb_im_client_t* client,
                            const xcb_im_proto_header_t* hdr,
                            uint8_t* data,
                            bool *del)
{
    unsigned char *p1 = data;

    switch (hdr->major_opcode) {
    case XIM_CONNECT:
        DebugLog("-- XIM_CONNECT\n");
        break;

    case XIM_DISCONNECT:
        DebugLog("-- XIM_DISCONNECT\n");
        break;

    case XIM_OPEN:
        DebugLog("-- XIM_OPEN\n");
        break;

    case XIM_CLOSE:
        DebugLog("-- XIM_CLOSE\n");
        break;

    case XIM_QUERY_EXTENSION:
        DebugLog("-- XIM_QUERY_EXTENSION\n");
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
        uint8_t* message = ReadXIMMessage(im, clientmessage, client, &hdr);
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
        int length = xcb_get_property_value_length(reply);
        int i;
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
