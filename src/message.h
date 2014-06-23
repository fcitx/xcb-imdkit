#ifndef MESSAGE_H
#define MESSAGE_H
#include <xcb/xcb.h>
#include <stdbool.h>

uint8_t* _xcb_new_xim_message(uint8_t major_opcode,
                              uint8_t minor_opcode,
                              size_t length,
                              bool swap);

void _xcb_write_xim_message_header(uint8_t* message,
                                   uint8_t major_opcode,
                                   uint8_t minor_opcode,
                                   size_t length,
                                   bool swap);

// length is the body without header size in byte
bool _xcb_send_xim_message(xcb_connection_t* conn,
                           xcb_atom_t protocol_atom,
                           xcb_window_t window,
                           uint8_t* data, size_t length,
                           const char* name, size_t len);

void _xcb_send_xim_error_message(xcb_connection_t* conn,
                                 xcb_atom_t protocol_atom,
                                 xcb_window_t window,
                                 bool swap);

#define _xcb_send_xim_frame(CONN, PROTOCOL_ATOM, WINDOW, FRAME, SWAP, NAME, LEN, SEND_ERROR) \
    do { \
        bool fail = true; \
        bool swap = SWAP; \
        size_t length = frame_size_func(FRAME); \
        uint8_t* reply = NULL; \
        uint8_t* alloc_reply = NULL; \
        uint8_t static_reply[frame_has_static_size(FRAME) ? frame_size_func(FRAME) : 1]; \
        if (frame_has_static_size(FRAME)) { \
            reply = static_reply; \
        } else { \
            reply = _xcb_new_xim_message(frame_opcode(FRAME), 0, length, swap); \
            alloc_reply = reply; \
        } \
        do { \
            if (!reply) { \
                break; \
            } \
            frame_write_func(FRAME)(&(FRAME), reply + XCB_IM_HEADER_SIZE, swap); \
            if (!_xcb_send_xim_message((CONN), (PROTOCOL_ATOM), (WINDOW), reply, length, NAME, LEN)) { \
                break; \
            } \
            fail = false; \
        } while(0); \
        free(alloc_reply); \
        if ((SEND_ERROR) && fail) { \
            _xcb_send_xim_error_message((CONN), (PROTOCOL_ATOM), (WINDOW), swap); \
        } \
    } while(0)


#define _xcb_send_fixed_length_xim_frame(CONN, PROTOCOL_ATOM, WINDOW, FRAME, SWAP, SEND_ERROR) \
    do { \
        _Static_assert((frame_has_static_size(FRAME) ? frame_size_func(FRAME) : XIM_CM_DATA_SIZE + 1) <= XIM_CM_DATA_SIZE, "frame should have static size"); \
        _xcb_send_xim_frame(CONN, PROTOCOL_ATOM, WINDOW, FRAME, SWAP, NAME, LEN, SEND_ERROR); \
    } while(0)

#endif // MESSAGE_H
