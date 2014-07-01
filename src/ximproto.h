/*
 * (C) Copyright 2014 Weng Xuetian <wengxt@gmail.com>
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the GNU Lesser General Public License
 * (LGPL) version 2.1 which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/lgpl-2.1.html
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 */

#ifndef XIMPROTO_H
#define XIMPROTO_H

#include <stdlib.h>
#include "parser.h"
#include "ximproto-gen.h"

#define XCB_IM_HEADER_SIZE 4

/*
 * Client Message data size
 */
#define XIM_CM_DATA_SIZE    20

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

#define XimLookupChars        0x0002
#define XimLookupKeySym       0x0004
#define XimLookupBoth         0x0006

#define frame_opcode(FRAME) _Generic((FRAME), \
    xcb_im_connect_reply_fr_t: XIM_CONNECT_REPLY, \
    xcb_im_open_fr_t: XIM_OPEN, \
    xcb_im_open_reply_fr_t: XIM_OPEN_REPLY, \
    xcb_im_close_fr_t: XIM_CLOSE, \
    xcb_im_close_reply_fr_t: XIM_CLOSE_REPLY, \
    xcb_im_query_extension_fr_t: XIM_QUERY_EXTENSION, \
    xcb_im_query_extension_reply_fr_t: XIM_QUERY_EXTENSION_REPLY, \
    xcb_im_encoding_negotiation_fr_t: XIM_ENCODING_NEGOTIATION, \
    xcb_im_encoding_negotiation_reply_fr_t: XIM_ENCODING_NEGOTIATION_REPLY, \
    xcb_im_get_im_values_fr_t: XIM_GET_IM_VALUES, \
    xcb_im_get_im_values_reply_fr_t: XIM_GET_IM_VALUES_REPLY, \
    xcb_im_set_event_mask_fr_t: XIM_SET_EVENT_MASK, \
    xcb_im_create_ic_fr_t: XIM_CREATE_IC, \
    xcb_im_create_ic_reply_fr_t: XIM_CREATE_IC_REPLY, \
    xcb_im_set_ic_values_fr_t: XIM_SET_IC_VALUES, \
    xcb_im_set_ic_values_reply_fr_t: XIM_SET_IC_VALUES_REPLY, \
    xcb_im_get_ic_values_fr_t: XIM_GET_IC_VALUES, \
    xcb_im_get_ic_values_reply_fr_t: XIM_GET_IC_VALUES_REPLY, \
    xcb_im_register_triggerkeys_fr_t: XIM_REGISTER_TRIGGERKEYS, \
    xcb_im_destroy_ic_fr_t: XIM_DESTROY_IC, \
    xcb_im_destroy_ic_reply_fr_t: XIM_DESTROY_IC_REPLY, \
    xcb_im_reset_ic_fr_t: XIM_RESET_IC, \
    xcb_im_reset_ic_reply_fr_t: XIM_RESET_IC_REPLY, \
    xcb_im_trigger_notify_fr_t: XIM_TRIGGER_NOTIFY, \
    xcb_im_trigger_notify_reply_fr_t: XIM_TRIGGER_NOTIFY_REPLY, \
    xcb_im_preedit_start_fr_t: XIM_PREEDIT_START, \
    xcb_im_preedit_draw_fr_t: XIM_PREEDIT_DRAW, \
    xcb_im_preedit_caret_fr_t: XIM_PREEDIT_CARET, \
    xcb_im_preedit_done_fr_t: XIM_PREEDIT_DONE, \
    xcb_im_status_start_fr_t: XIM_STATUS_START, \
    xcb_im_status_draw_text_fr_t: XIM_STATUS_DRAW, \
    xcb_im_status_draw_bitmap_fr_t: XIM_STATUS_DRAW, \
    xcb_im_status_done_fr_t: XIM_STATUS_DONE, \
    xcb_im_commit_chars_fr_t: XIM_COMMIT, \
    xcb_im_commit_both_fr_t: XIM_COMMIT, \
    xcb_im_connect_fr_t: XIM_CONNECT, \
    xcb_im_disconnect_fr_t: XIM_DISCONNECT, \
    xcb_im_set_ic_focus_fr_t: XIM_SET_IC_FOCUS, \
    xcb_im_unset_ic_focus_fr_t: XIM_UNSET_IC_FOCUS, \
    xcb_im_sync_fr_t: XIM_SYNC, \
    xcb_im_sync_reply_fr_t: XIM_SYNC_REPLY, \
    xcb_im_geometry_fr_t: XIM_GEOMETRY \
    )


typedef struct _xcb_im_ext_list{
    char *name;
    uint8_t major_opcode;
    uint8_t minor_opcode;
} xcb_im_ext_list;

/*
 * Minor Protocol Number for Extension Protocol
 */
#define XIM_EXTENSION               128
#define XIM_EXT_SET_EVENT_MASK          (0x30)
#define XIM_EXT_FORWARD_KEYEVENT        (0x32)
#define XIM_EXT_MOVE                (0x33)


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

/*
 * value for the flag of XIM_FORWARD_EVENT, XIM_COMMIT
 */
#define XimSYNCHRONUS         0x0001


typedef enum _xcb_im_style_t
{
    XCB_IM_PreeditArea      = 0x0001,
    XCB_IM_PreeditCallbacks = 0x0002,
    XCB_IM_PreeditPosition  = 0x0004,
    XCB_IM_PreeditNothing   = 0x0008,
    XCB_IM_PreeditNone      = 0x0010,
    XCB_IM_StatusArea       = 0x0100,
    XCB_IM_StatusCallbacks  = 0x0200,
    XCB_IM_StatusNothing    = 0x0400,
    XCB_IM_StatusNone       = 0x0800,
} xcb_im_style_t;

enum {
    XCB_IM_TextType = 0,
    XCB_IM_BitmapType = 1
};

typedef struct _xcb_im_trigger_keys_t{
    uint16_t nKeys;
    xcb_im_ximtriggerkey_fr_t *keys;
} xcb_im_trigger_keys_t;

#endif // XIMPROTO_H
