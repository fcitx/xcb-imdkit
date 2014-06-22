#ifndef IMDKIT_H
#define IMDKIT_H

#include <xcb/xcb.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ximproto.h"

#define XCB_IMDKIT_EXPORT __attribute__ ((visibility("default")))

/*
 * `C' and `no' are additional one which cannot be obtained from modern
 * locale.gen. `no' is obsolete, but we keep it here for compatible reason.
 */
#define XCB_IM_ALL_LOCALES \
    "aa,af,am,an,ar,as,ast,az,be,bem,ber,bg,bho,bn,bo,br,brx,bs,byn," \
    "C,ca,crh,cs,csb,cv,cy,da,de,dv,dz,el,en,es,et,eu,fa,ff,fi,fil,fo," \
    "fr,fur,fy,ga,gd,gez,gl,gu,gv,ha,he,hi,hne,hr,hsb,ht,hu,hy,id,ig," \
    "ik,is,it,iu,iw,ja,ka,kk,kl,km,kn,ko,kok,ks,ku,kw,ky,lb,lg,li,lij," \
    "lo,lt,lv,mag,mai,mg,mhr,mi,mk,ml,mn,mr,ms,mt,my,nan,nb,nds,ne,nl," \
    "nn,no,nr,nso,oc,om,or,os,pa,pap,pl,ps,pt,ro,ru,rw,sa,sc,sd,se,shs," \
    "si,sid,sk,sl,so,sq,sr,ss,st,sv,sw,ta,te,tg,th,ti,tig,tk,tl,tn,tr," \
    "ts,tt,ug,uk,unm,ur,uz,ve,vi,wa,wae,wal,wo,xh,yi,yo,yue,zh,zu"

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
typedef struct _xcb_im_styles_t
{
    uint32_t nStyles;
    uint32_t* styles;
} xcb_im_styles_t;

typedef xcb_im_packet_header_fr_t xcb_im_proto_header_t;
typedef xcb_im_ximtriggerkey_fr_t xcb_im_trigger_key_t;

typedef struct _xcb_im_trigger_keys_t{
    uint16_t nKeys;
    xcb_im_trigger_key_t *keys;
} xcb_im_trigger_keys_t;

typedef char *xcb_im_encoding_t;

typedef struct _xcb_im_encodings_t {
    unsigned short nEncodings;
    xcb_im_encoding_t *encodings;
} xcb_im_encodings_t;

typedef struct _xcb_im_t xcb_im_t;

typedef struct _xcb_im_client_t
{
    xcb_window_t accept_win;
    int connect_id;
    xcb_window_t client_win;
    uint8_t byte_order;
    bool sync;
} xcb_im_client_t;

typedef struct _xcb_im_preedit_attr_t {
    xcb_rectangle_t area;
    xcb_rectangle_t area_needed;
    xcb_point_t spot_location;
    xcb_colormap_t colormap;
    uint32_t foreground;
    uint32_t background;
    xcb_window_t bg_pixmap;
    uint32_t line_space;
} xcb_im_preedit_attr_t;

typedef struct _xcb_im_status_attr_t {
    xcb_rectangle_t area;
    xcb_rectangle_t area_needed;
    xcb_point_t spot_location;
    xcb_colormap_t colormap;
    uint32_t foreground;
    uint32_t background;
    xcb_window_t bg_pixmap;
    uint32_t line_space;
} xcb_im_status_attr_t;

typedef struct _xcb_im_input_context_t
{
    uint16_t id;
    xcb_im_client_t* client;
    uint32_t input_style;
    xcb_window_t client_win;
    xcb_window_t focus_win;

    xcb_im_preedit_attr_t preedit;
    xcb_im_status_attr_t status;
} xcb_im_input_context_t;

typedef bool (*xcb_im_callback)(xcb_im_t* im, xcb_im_client_t* client, xcb_im_input_context_t* ic,
                                const xcb_im_proto_header_t* hdr,
                                void* frame, void* arg, void* user_data);

XCB_IMDKIT_EXPORT xcb_im_t* xcb_im_create(xcb_connection_t* conn,
                                          int screen,
                                          xcb_window_t serverWindow,
                                          const char* serverName,
                                          const char* locale,
                                          const xcb_im_styles_t* inputStyles,
                                          const xcb_im_trigger_keys_t* onKeysList,
                                          const xcb_im_trigger_keys_t* offKeysList,
                                          const xcb_im_encodings_t* encodingList,
                                          uint32_t event_mask,
                                          xcb_im_callback callback,
                                          void* user_data);
XCB_IMDKIT_EXPORT bool xcb_im_open_im(xcb_im_t* im);
XCB_IMDKIT_EXPORT bool xcb_im_filter_event(xcb_im_t* im, xcb_generic_event_t* event);
XCB_IMDKIT_EXPORT void xcb_im_close_im(xcb_im_t* im);
XCB_IMDKIT_EXPORT void xcb_im_destroy(xcb_im_t* im);
XCB_IMDKIT_EXPORT void xcb_im_forward_event(xcb_im_t* im, xcb_im_input_context_t* ic, xcb_key_press_event_t* event);
XCB_IMDKIT_EXPORT void xcb_im_commit_string(xcb_im_t* im, xcb_im_input_context_t* ic, uint32_t flag, char* str, uint32_t length, uint32_t keysym);
XCB_IMDKIT_EXPORT void xcb_im_geometry_callback(xcb_im_t* im, xcb_im_input_context_t* ic);
XCB_IMDKIT_EXPORT void xcb_im_preedit_start_callback(xcb_im_t* im, xcb_im_input_context_t* ic);
XCB_IMDKIT_EXPORT void xcb_im_preedit_draw_callback(xcb_im_t* im, xcb_im_input_context_t* ic, xcb_im_preedit_draw_fr_t* frame);
XCB_IMDKIT_EXPORT void xcb_im_preedit_caret_callback(xcb_im_t* im, xcb_im_input_context_t* ic, xcb_im_preedit_caret_fr_t* frame);
XCB_IMDKIT_EXPORT void xcb_im_preedit_done_callback(xcb_im_t* im, xcb_im_input_context_t* ic);
XCB_IMDKIT_EXPORT void xcb_im_status_start_callback(xcb_im_t* im, xcb_im_input_context_t* ic);
XCB_IMDKIT_EXPORT void xcb_im_status_draw_text_callback(xcb_im_t* im, xcb_im_input_context_t* ic, xcb_im_status_draw_text_fr_t* frame);
XCB_IMDKIT_EXPORT void xcb_im_status_draw_bitmap_callback(xcb_im_t* im, xcb_im_input_context_t* ic, xcb_im_status_draw_bitmap_fr_t* frame);
XCB_IMDKIT_EXPORT void xcb_im_status_done_callback(xcb_im_t* im, xcb_im_input_context_t* ic);
XCB_IMDKIT_EXPORT void xcb_im_preedit_start(xcb_im_t* im, xcb_im_input_context_t* ic);
XCB_IMDKIT_EXPORT void xcb_im_preedit_end(xcb_im_t* im, xcb_im_input_context_t* ic);
XCB_IMDKIT_EXPORT void xcb_im_sync_xlib(xcb_im_t* im, xcb_im_input_context_t* ic);


#endif // IMDKIT_H
