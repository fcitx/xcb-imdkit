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
#ifndef _XCB_IMDKIT_IMDKIT_H_
#define _XCB_IMDKIT_IMDKIT_H_

#include "ximcommon.h"
#include "ximproto.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xcb/xcb.h>

XCB_IMDKIT_DECL_BEGIN

/*
 * `C' and `no' are additional one which cannot be obtained from modern
 * locale.gen. `no' is obsolete, but we keep it here for compatible reason.
 */
#define XCB_IM_ALL_LOCALES                                                     \
    "aa,af,am,an,ar,as,ast,az,be,bem,ber,bg,bho,bn,bo,br,brx,bs,byn,"          \
    "C,ca,crh,cs,csb,cv,cy,da,de,dv,dz,el,en,es,et,eu,fa,ff,fi,fil,fo,"        \
    "fr,fur,fy,ga,gd,gez,gl,gu,gv,ha,he,hi,hne,hr,hsb,ht,hu,hy,id,ig,"         \
    "ik,is,it,iu,iw,ja,ka,kk,kl,km,kn,ko,kok,ks,ku,kw,ky,lb,lg,li,lij,"        \
    "lo,lt,lv,mag,mai,mg,mhr,mi,mk,ml,mn,mr,ms,mt,my,nan,nb,nds,ne,nl,"        \
    "nn,no,nr,nso,oc,om,or,os,pa,pap,pl,ps,pt,ro,ru,rw,sa,sc,sd,se,shs,"       \
    "si,sid,sk,sl,so,sq,sr,ss,st,sv,sw,ta,te,tg,th,ti,tig,tk,tl,tn,tr,"        \
    "ts,tt,ug,uk,unm,ur,uz,ve,vi,wa,wae,wal,wo,xh,yi,yo,yue,zh,zu"

typedef struct _xcb_im_styles_t {
    uint32_t nStyles;
    uint32_t *styles;
} xcb_im_styles_t;

typedef char *xcb_im_encoding_t;

typedef struct _xcb_im_encodings_t {
    unsigned short nEncodings;
    xcb_im_encoding_t *encodings;
} xcb_im_encodings_t;

typedef struct _xcb_im_t xcb_im_t;
typedef struct _xcb_im_input_context_t xcb_im_input_context_t;
typedef struct _xcb_im_client_t xcb_im_client_t;

// Mask to check if the attribute is set.
typedef enum _xcb_im_attr_mask_t {
    XCB_XIM_XNArea_MASK = (1 << 0),
    XCB_XIM_XNAreaNeeded_MASK = (1 << 1),
    XCB_XIM_XNSpotLocation_MASK = (1 << 2),
    XCB_XIM_XNColormap_MASK = (1 << 3),
    XCB_XIM_XNForeground_MASK = (1 << 4),
    XCB_XIM_XNBackground_MASK = (1 << 5),
    XCB_XIM_XNBackgroundPixmap_MASK = (1 << 7),
    XCB_XIM_XNLineSpace_MASK = (1 << 8),
} xcb_im_attr_mask_t;

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

typedef void (*xcb_im_callback)(xcb_im_t *im, xcb_im_client_t *client,
                                xcb_im_input_context_t *ic,
                                const xcb_im_packet_header_fr_t *hdr,
                                void *frame, void *arg, void *user_data);

typedef void (*xcb_im_free_function)(void *memory);

XCB_IMDKIT_EXPORT xcb_im_t *
xcb_im_create(xcb_connection_t *conn, int screen, xcb_window_t serverWindow,
              const char *serverName, const char *locale,
              const xcb_im_styles_t *inputStyles,
              const xcb_im_trigger_keys_t *onKeysList,
              const xcb_im_trigger_keys_t *offKeysList,
              const xcb_im_encodings_t *encodingList, uint32_t event_mask,
              xcb_im_callback callback, void *user_data);
XCB_IMDKIT_EXPORT void
xcb_im_set_log_handler(xcb_im_t *im, void (*logger)(const char *, ...));
XCB_IMDKIT_EXPORT void xcb_im_set_use_sync_mode(xcb_im_t *im, bool sync);
XCB_IMDKIT_EXPORT bool xcb_im_open_im(xcb_im_t *im);
XCB_IMDKIT_EXPORT bool xcb_im_filter_event(xcb_im_t *im,
                                           xcb_generic_event_t *event);
XCB_IMDKIT_EXPORT void xcb_im_close_im(xcb_im_t *im);
XCB_IMDKIT_EXPORT void xcb_im_destroy(xcb_im_t *im);
XCB_IMDKIT_EXPORT void xcb_im_forward_event(xcb_im_t *im,
                                            xcb_im_input_context_t *ic,
                                            xcb_key_press_event_t *event);
XCB_IMDKIT_EXPORT void xcb_im_commit_string(xcb_im_t *im,
                                            xcb_im_input_context_t *ic,
                                            uint32_t flag, const char *str,
                                            uint32_t length, uint32_t keysym);
XCB_IMDKIT_EXPORT void xcb_im_geometry_callback(xcb_im_t *im,
                                                xcb_im_input_context_t *ic);
XCB_IMDKIT_EXPORT void
xcb_im_preedit_start_callback(xcb_im_t *im, xcb_im_input_context_t *ic);
XCB_IMDKIT_EXPORT void
xcb_im_preedit_draw_callback(xcb_im_t *im, xcb_im_input_context_t *ic,
                             xcb_im_preedit_draw_fr_t *frame);
XCB_IMDKIT_EXPORT void
xcb_im_preedit_caret_callback(xcb_im_t *im, xcb_im_input_context_t *ic,
                              xcb_im_preedit_caret_fr_t *frame);
XCB_IMDKIT_EXPORT void xcb_im_preedit_done_callback(xcb_im_t *im,
                                                    xcb_im_input_context_t *ic);
XCB_IMDKIT_EXPORT void xcb_im_status_start_callback(xcb_im_t *im,
                                                    xcb_im_input_context_t *ic);
XCB_IMDKIT_EXPORT void
xcb_im_status_draw_text_callback(xcb_im_t *im, xcb_im_input_context_t *ic,
                                 xcb_im_status_draw_text_fr_t *frame);
XCB_IMDKIT_EXPORT void
xcb_im_status_draw_bitmap_callback(xcb_im_t *im, xcb_im_input_context_t *ic,
                                   xcb_im_status_draw_bitmap_fr_t *frame);
XCB_IMDKIT_EXPORT void xcb_im_status_done_callback(xcb_im_t *im,
                                                   xcb_im_input_context_t *ic);
XCB_IMDKIT_EXPORT void xcb_im_preedit_start(xcb_im_t *im,
                                            xcb_im_input_context_t *ic);
XCB_IMDKIT_EXPORT void xcb_im_preedit_end(xcb_im_t *im,
                                          xcb_im_input_context_t *ic);
XCB_IMDKIT_EXPORT void xcb_im_sync_xlib(xcb_im_t *im,
                                        xcb_im_input_context_t *ic);
XCB_IMDKIT_EXPORT bool xcb_im_support_extension(xcb_im_t *im,
                                                uint16_t major_code,
                                                uint16_t minor_code);
XCB_IMDKIT_EXPORT void
xcb_im_input_context_set_data(xcb_im_input_context_t *ic, void *data,
                              xcb_im_free_function free_data_function);
XCB_IMDKIT_EXPORT void *
xcb_im_input_context_get_data(xcb_im_input_context_t *ic);
XCB_IMDKIT_EXPORT uint32_t
xcb_im_input_context_get_input_style(xcb_im_input_context_t *ic);
XCB_IMDKIT_EXPORT xcb_window_t
xcb_im_input_context_get_client_window(xcb_im_input_context_t *ic);
XCB_IMDKIT_EXPORT xcb_window_t
xcb_im_input_context_get_focus_window(xcb_im_input_context_t *ic);
XCB_IMDKIT_EXPORT const xcb_im_preedit_attr_t *
xcb_im_input_context_get_preedit_attr(xcb_im_input_context_t *ic);
XCB_IMDKIT_EXPORT const xcb_im_status_attr_t *
xcb_im_input_context_get_status_attr(xcb_im_input_context_t *ic);
XCB_IMDKIT_EXPORT uint32_t
xcb_im_input_context_get_preedit_attr_mask(xcb_im_input_context_t *ic);
XCB_IMDKIT_EXPORT uint32_t
xcb_im_input_context_get_status_attr_mask(xcb_im_input_context_t *ic);

XCB_IMDKIT_DECL_END

#endif // _XCB_IMDKIT_IMDKIT_H_
