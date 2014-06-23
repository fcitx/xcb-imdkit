#ifndef IMCLIENT_P_H
#define IMCLIENT_P_H



#define _xcb_xim_read_frame(FRAME, DATA, LEN, FAIL) \
    do { \
        FAIL = false; \
        uint8_t* _data = DATA; \
        size_t len = (LEN); \
        frame_read_func(FRAME)(&FRAME, &_data, &len, false); \
        if (!_data) { \
            frame_free_func(FRAME)(&FRAME); \
            FAIL = true; \
        } \
    } while(0)

#define _xcb_xim_send_frame(IM, FRAME, FAIL) \
    do { \
        FAIL = true; \
        size_t length = frame_size_func(FRAME); \
        uint8_t* reply = NULL; \
        uint8_t* alloc_reply = NULL; \
        uint8_t static_reply[frame_has_static_size(FRAME) ? frame_size_func(FRAME) : 1]; \
        if (frame_has_static_size(FRAME)) { \
            reply = static_reply; \
            _xcb_write_xim_message_header(reply, frame_opcode(FRAME), 0, length, false); \
        } else { \
            reply = _xcb_new_xim_message(frame_opcode(FRAME), 0, length, false); \
            alloc_reply = reply; \
        } \
        do { \
            if (!reply) { \
                break; \
            } \
            frame_write_func(FRAME)(&(FRAME), reply + XCB_IM_HEADER_SIZE, false); \
            if (!_xcb_xim_send_message((IM), reply, length)) { \
                break; \
            } \
            FAIL = false; \
        } while(0); \
        free(alloc_reply); \
    } while(0)


#endif // IMCLIENT_P_H
