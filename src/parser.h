#ifndef IMDKIT_PARSE_H
#define IMDKIT_PARSE_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef enum {
    BIT8     = 0x1,       /* {CARD8* | INT8*}   */
    BIT16    = 0x2,       /* {CARD16* | INT16*} */
    BIT32    = 0x3,       /* {CARD32* | INT32*} */
    BIT64    = 0x4,   /* {CARD64* | INT64*} */
    BARRAY   = 0x5,       /* int*, void*        */
    ITER     = 0x6,       /* int*               */
    POINTER  = 0x7,       /* specifies next item is a PTR_ITEM */
    PTR_ITEM = 0x8,       /* specifies the item has a pointer */
    /* BOGUS - POINTER and PTR_ITEM
     *   In the current implementation, PTR_ITEM should be lead by
     *   POINTER.  But actually, it's just redundant logically.  Someone
     *   may remove this redundancy and POINTER from the enum member but he
     *   should also modify the logic in xcb_im_frame_manager_t* program.
     */
    PADDING  = 0x9,       /* specifies that a padding is needed.
                   * This requires extra data in data field.
                   */
    EOL      = 0xA,       /* specifies the end of list */

    COUNTER_MASK = 0x10,
    UNUSED_MASK = 0x20,
    COUNTER_BIT8  = COUNTER_MASK | 0x1,
    COUNTER_BIT16 = COUNTER_MASK | 0x2,
    COUNTER_BIT32 = COUNTER_MASK | 0x3,
    COUNTER_BIT64 = COUNTER_MASK | 0x4,
    UNUSED_BIT8  = UNUSED_MASK | 0x1,
    UNUSED_BIT16 = UNUSED_MASK | 0x2,
    UNUSED_BIT32 = UNUSED_MASK | 0x3,
    UNUSED_BIT64 = UNUSED_MASK | 0x4,
} xcb_im_frame_type_t;

typedef struct _xcb_im_frame_t {
    xcb_im_frame_type_t type;
    union {
        const struct _xcb_im_frame_t* subframe;
        uintptr_t aux;
    };
} xcb_im_frame_t;

uint8_t* parse_binary_with_args(void* data, ...);

uint8_t* parse_binary_with_format(uint8_t* data, bool swap, const xcb_im_frame_t* format, ...);

size_t format_size(const xcb_im_frame_t* format);
uint8_t* write_binary_with_format(uint8_t* data, bool swap, const xcb_im_frame_t* format, ...);

#endif // IMDKIT_PARSE_H
