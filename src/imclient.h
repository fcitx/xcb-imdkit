#ifndef IMCLIENT_H
#define IMCLIENT_H
#include <xcb/xcb.h>
#include <stdbool.h>

#define XCB_XIM_EXPORT __attribute__ ((visibility("default")))

typedef struct _xcb_xim_t xcb_xim_t;
typedef uint16_t xcb_xic_t;

XCB_XIM_EXPORT xcb_xim_t* xcb_xim_create(xcb_connection_t* conn, int screen_id, const char* imname);
XCB_XIM_EXPORT bool xcb_xim_open(xcb_xim_t* im);
XCB_XIM_EXPORT bool xcb_xim_filter_event(xcb_xim_t* im, xcb_generic_event_t* event);
XCB_XIM_EXPORT xcb_xic_t xcb_xim_create_ic(xcb_xim_t* im);
XCB_XIM_EXPORT void xcb_xim_get_ic_values(xcb_xim_t* im, xcb_xic_t ic, ...);
XCB_XIM_EXPORT void xcb_xim_set_ic_values(xcb_xim_t* im, xcb_xic_t ic, ...);
XCB_XIM_EXPORT void xcb_xim_set_ic_focus(xcb_xim_t* im, xcb_xic_t ic);
XCB_XIM_EXPORT void xcb_xim_unset_ic_focus(xcb_xim_t* im, xcb_xic_t ic);
XCB_XIM_EXPORT void xcb_xim_destroy_ic(xcb_xim_t* im, xcb_xic_t ic);
XCB_XIM_EXPORT void xcb_xim_close(xcb_xim_t* im);

#endif // IMCLIENT_H
