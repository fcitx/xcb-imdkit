#ifndef COMMON_H
#define COMMON_H

#include <xcb/xcb.h>
#include <stdbool.h>
#include <stdio.h>

// some atom name
#define XIM_SERVERS     "XIM_SERVERS"
#define XIM_LOCALES     "LOCALES"
#define XIM_TRANSPORT       "TRANSPORT"
#define _XIM_PROTOCOL           "_XIM_PROTOCOL"
#define _XIM_XCONNECT           "_XIM_XCONNECT"

#define ARRAY_SIZE(X) (sizeof(X) / sizeof(X[0]))

#define XIM_MESSAGE_BYTES(hdr) ((hdr)->length * 4)

/*
 * categories in XIM_SERVERS
 */
#define XIM_SERVER_CATEGORY "@server="
#define XIM_LOCAL_CATEGORY  "@locale="
#define XIM_TRANSPORT_CATEGORY  "@transport="

#define XIM_DEBUG
#ifdef XIM_DEBUG
#define DebugLog(S...) fprintf(stderr, S)
#else
#define DebugLog(S...) ((void) (S))
#endif

enum {
    XIM_ATOM_SERVER_NAME,
    XIM_ATOM_XIM_SERVERS,
    XIM_ATOM_LOCALES,
    XIM_ATOM_TRANSPORT,
    XIM_ATOM_XIM_PROTOCOL,
    XIM_ATOM_XIM_CONNECT,
    XIM_ATOM_LAST
};

bool _xcb_im_init_atoms(xcb_connection_t* conn, size_t n, const char** atom_names, xcb_atom_t* atoms);

#endif // COMMON_H
