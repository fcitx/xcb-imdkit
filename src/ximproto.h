#ifndef XIMPROTO_H
#define XIMPROTO_H

#include "parser.h"

#define XCB_IM_HEADER_SIZE 4

#define _FRAME(TYPE) {.type = TYPE}
#define _PAD2(n)   {.type = PADDING, .aux = ((n)<<8|2)}
#define _PAD4(n)   {.type = PADDING, .aux = ((n)<<8|4)}

#define _BYTE_UNUSED(TYPE) \
    {.type = (UNUSED_MASK|TYPE)}
#define _BYTE_COUNTER(TYPE, OFFSET) \
    {.type = (COUNTER_MASK|TYPE), .aux = ((OFFSET)<<8|0)}
#define _PTR(p)   {.type = PTR_ITEM, .subframe=p}

static const xcb_im_frame_t xpcs_fr[] = {
    _FRAME(BIT16),      /* length of string in bytes */
    _FRAME(BARRAY),         /* string */
    _PAD4(2),
};

static const xcb_im_frame_t packet_header_fr[] = {
    _FRAME(BIT8),       /* major-opcode */
    _FRAME(BIT8),       /* minor-opcode */
    _FRAME(BIT16),      /* length */
    _FRAME(EOL),
};

static const xcb_im_frame_t connect_fr[] = {
    _FRAME(BIT8),       /* byte order */
    _BYTE_UNUSED(BIT8),           /* unused */
    _FRAME(BIT16),      /* client-major-protocol-version */
    _FRAME(BIT16),      /* client-minor-protocol-version */
    _BYTE_COUNTER(BIT16, 1),    /* length of client-auth-protocol-names */
    _FRAME(ITER),       /* client-auth-protocol-names */
    _FRAME(POINTER),
    _PTR(xpcs_fr),
    _FRAME(EOL),
};

static const xcb_im_frame_t connect_reply_fr[] = {
    _FRAME(BIT16),      /* server-major-protocol-version */
    _FRAME(BIT16),      /* server-minor-protocol-version */
    _FRAME(EOL),
};

#endif // XIMPROTO_H
