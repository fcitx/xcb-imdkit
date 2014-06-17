#include <stdarg.h>
#include <stdlib.h>
#include "parser.h"
/* For byte swapping */
#define Swap8(n) (n)

#define Swap16(n) (swap ?       \
                      (((n) << 8 & 0xFF00) | \
                       ((n) >> 8 & 0xFF)     \
                      ) : n)
#define Swap32(n) (swap ?            \
                      (((n) << 24 & 0xFF000000) | \
                       ((n) <<  8 & 0xFF0000) |   \
                       ((n) >>  8 & 0xFF00) |     \
                       ((n) >> 24 & 0xFF)         \
                      ) : n)
#define Swap64(n) (swap ?            \
                      (((n) << 56 & 0xFF00000000000000) | \
                       ((n) << 40 & 0xFF000000000000) |   \
                       ((n) << 24 & 0xFF0000000000) |     \
                       ((n) <<  8 & 0xFF00000000) |       \
                       ((n) >>  8 & 0xFF000000) |         \
                       ((n) >> 24 & 0xFF0000) |           \
                       ((n) >> 40 & 0xFF00) |             \
                       ((n) >> 56 & 0xFF)                 \
                      ) : n)

uint8_t* parse_binary_with_format(uint8_t* data, bool swap, const xcb_im_frame_type_t* format, ...)
{
    va_list var;
    va_start(var, &format);

    for (int i = 0; format[i] != EOL; i++) {
        void* attr = va_arg(var, void*);
#define BIT_CASE(N) \
    case BIT##N: \
    { \
        uint##N##_t* result = attr; \
        *result = Swap##N(*(uint##N##_t*) data); \
        data += (N / 8); \
        break; \
    }

        switch(format[i]) {
            BIT_CASE(8)
            BIT_CASE(16)
            BIT_CASE(32)
            BIT_CASE(64)
            default:
                return NULL;
        }
    }

    va_end(var);
    return data;
}
