#ifndef XIMPROTO_H
#define XIMPROTO_H

#include "parser.h"

static const xcb_im_frame_type_t packet_header_fr[] = {
    BIT8,       /* major-opcode */
    BIT8,       /* minor-opcode */
    BIT16,      /* length */
    EOL,
};

#endif // XIMPROTO_H
