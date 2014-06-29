#ifndef IMCLIENT_H
#define IMCLIENT_H
#include <xcb/xcb.h>
#include <stdbool.h>
#include "ximproto.h"

#define XCB_XIM_EXPORT __attribute__ ((visibility("default")))

typedef struct _xcb_xim_t xcb_xim_t;
typedef uint16_t xcb_xic_t;

typedef struct _xcb_xim_nested_list
{
    uint8_t* data;
    size_t length;
} xcb_xim_nested_list;

typedef void (*xcb_xim_open_callback)(xcb_xim_t* im, void* user_data);
typedef void (*xcb_xim_create_ic_callback)(xcb_xim_t* im, bool success, xcb_xic_t ic, void* user_data);
typedef void (*xcb_xim_get_im_values_callback)(xcb_xim_t* im, xcb_im_get_im_values_reply_fr_t* reply, void* user_data);
typedef void (*xcb_xim_get_ic_values_callback)(xcb_xim_t* im, xcb_xic_t ic, xcb_im_get_ic_values_reply_fr_t* reply, void* user_data);
typedef void (*xcb_xim_set_ic_values_callback)(xcb_xim_t* im, xcb_xic_t ic, void* user_data);
typedef void (*xcb_xim_destroy_ic_callback)(xcb_xim_t* im, xcb_xic_t ic, void* user_data);
typedef void (*xcb_xim_reset_ic_callback)(xcb_xim_t* im, xcb_xic_t ic, void* user_data);

XCB_XIM_EXPORT xcb_xim_t* xcb_xim_create(xcb_connection_t* conn,
                                         int screen_id,
                                         const char* imname);
XCB_XIM_EXPORT void xcb_xim_destroy(xcb_xim_t* im);
XCB_XIM_EXPORT bool xcb_xim_open(xcb_xim_t* im,
                                 xcb_xim_open_callback callback,
                                 void* user_data);
XCB_XIM_EXPORT bool xcb_xim_filter_event(xcb_xim_t* im, xcb_generic_event_t* event);
XCB_XIM_EXPORT bool xcb_xim_get_im_values(xcb_xim_t* im, xcb_xim_get_im_values_callback callback, void* user_data, ...);
XCB_XIM_EXPORT xcb_xim_nested_list xcb_xim_create_nested_list(xcb_xim_t* im, ...);
XCB_XIM_EXPORT bool xcb_xim_create_ic(xcb_xim_t* im, xcb_xim_create_ic_callback callback, void* user_data, ...);
XCB_XIM_EXPORT bool xcb_xim_get_ic_values(xcb_xim_t* im, xcb_xic_t ic, xcb_xim_get_ic_values_callback callback, void* user_data, ...);
XCB_XIM_EXPORT bool xcb_xim_set_ic_values(xcb_xim_t* im, xcb_xic_t ic, xcb_xim_set_ic_values_callback callback, void* user_data, ...);
XCB_XIM_EXPORT void xcb_xim_set_ic_focus(xcb_xim_t* im, xcb_xic_t ic);
XCB_XIM_EXPORT void xcb_xim_unset_ic_focus(xcb_xim_t* im, xcb_xic_t ic);
XCB_XIM_EXPORT bool xcb_xim_forward_event(xcb_xim_t* im, xcb_xic_t ic, xcb_key_press_event_t* event);
XCB_XIM_EXPORT void xcb_xim_reset_ic(xcb_xim_t* im, xcb_xic_t ic, xcb_xim_reset_ic_callback callback, void* user_data);
XCB_XIM_EXPORT bool xcb_xim_destroy_ic(xcb_xim_t* im, xcb_xic_t ic, xcb_xim_destroy_ic_callback callback, void* user_data);
XCB_XIM_EXPORT void xcb_xim_close(xcb_xim_t* im);
#endif // IMCLIENT_H
