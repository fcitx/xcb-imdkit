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

uint8_t* parse_binary_with_format(uint8_t* data, bool swap, const xcb_im_frame_t* format, ...)
{
    va_list var;
    va_start(var, &format);

    for (int i = 0; format[i].type != EOL; i++) {
#define BIT_CASE(N) \
    case BIT##N: \
    { \
        void* attr = va_arg(var, void*); \
        if (!attr) { \
            return data; \
        } \
        uint##N##_t* result = attr; \
        *result = Swap##N(*(uint##N##_t*) data); \
        data += (N / 8); \
        break; \
    } \
    case UNUSED_BIT##N: \
    { \
        data += (N / 8); \
        break; \
    }

        switch(format[i].type) {
            BIT_CASE(8)
            BIT_CASE(16)
            BIT_CASE(32)
            BIT_CASE(64)
            default:
                return NULL;
        }
    }

#undef BIT_CASE

    va_end(var);
    return data;
}

size_t format_size(const xcb_im_frame_t* format)
{
    size_t result = 0;
    for (int i = 0; format[i].type != EOL; i++) {
#define BIT_CASE(N) \
    case BIT##N: \
    { \
        result += (N / 8); \
        break; \
    } \
    case UNUSED_BIT##N: \
    { \
        result += (N / 8); \
        break; \
    }
        switch(format[i].type) {
            BIT_CASE(8)
            BIT_CASE(16)
            BIT_CASE(32)
            BIT_CASE(64)
            default:
                return 0;
        }
#undef BIT_CASE
    }

    return result;
}

uint8_t* write_binary_with_format(uint8_t* data, bool swap, const xcb_im_frame_t* format, ...)
{
    va_list var;
    va_start(var, &format);

    for (int i = 0; format[i].type != EOL; i++) {
        void* attr = va_arg(var, void*);
        if (!attr) {
            break;
        }
#define BIT_CASE(N) \
    case BIT##N: \
    { \
        uint##N##_t* result = attr; \
        *(uint##N##_t*) data = Swap##N(*result); \
        data += (N / 8); \
        break; \
    } \
    case UNUSED_BIT##N: \
    { \
        data += (N / 8); \
        break; \
    }

        switch(format[i].type) {
            BIT_CASE(8)
            BIT_CASE(16)
            BIT_CASE(32)
            BIT_CASE(64)
            default:
                return NULL;
        }
#undef BIT_CASE
    }

    va_end(var);
    return data;
}
