#ifndef IMCLIENT_P_H
#define IMCLIENT_P_H
#include "common.h"
#include "ximproto.h"
#include "uthash.h"
#include "message.h"
#include "imclient.h"



// this phase is basically a directly mapping from _XimSOMETHING function in Xlib
// state machine is more suitable for xcb asynchronous nature.
typedef enum _xcb_xim_open_phase_t {
    XIM_OPEN_PHASE_DONE,
    XIM_OPEN_PHASE_FAIL,
    XIM_OPEN_PHASE_CHECK_SERVER,
    XIM_OPEN_PHASE_CONNECT,
} xcb_xim_open_phase_t;

typedef enum _xcb_xim_open_check_server_phase_t {
    XIM_OPEN_PHASE_CHECK_SERVER_PREPARE,
    XIM_OPEN_PHASE_CHECK_SERVER_LOCALE,
    XIM_OPEN_PHASE_CHECK_SERVER_LOCALE_WAIT,
    XIM_OPEN_PHASE_CHECK_SERVER_TRANSPORT,
    XIM_OPEN_PHASE_CHECK_SERVER_TRANSPORT_WAIT,
} xcb_xim_open_check_server_phase_t;

typedef enum _xcb_xim_open_connect_phase_t {
    XIM_OPEN_PHASE_CONNECT_PREPARE,
    XIM_OPEN_PHASE_CONNECT_WAIT,
    XIM_OPEN_PHASE_CONNECT_WAIT_REPLY,
    XIM_OPEN_PHASE_CONNECT_WAIT_OPEN_REPLY
} xcb_xim_open_connect_phase_t;

typedef struct _xcb_xim_open_t
{
    xcb_xim_open_phase_t phase;
    union {
        struct {
            int index;
            xcb_xim_open_check_server_phase_t subphase;
            xcb_window_t window;
            xcb_window_t requestor_window;
        } check_server;

        struct {
            xcb_xim_open_connect_phase_t subphase;
        } connect;
    };
} xcb_xim_open_t;

typedef struct _xcb_xim_imattr_table_t
{
    xcb_im_ximattr_fr_t attr;
    UT_hash_handle hh;
} xcb_xim_imattr_table_t;

typedef struct _xcb_xim_icattr_table_t
{
    xcb_im_xicattr_fr_t attr;
    UT_hash_handle hh;
} xcb_xim_icattr_table_t;

struct _xcb_xim_t
{
    // basic data which should always be valid
    xcb_connection_t* conn;
    char* server_name;
    int screen_id;

    // some global data
    uint32_t sequence;
    uint8_t byte_order;

    // set by _xcb_xim_init
    xcb_screen_t* screen;
    xcb_atom_t atoms[XIM_ATOM_LAST]; // add one for SERVER_NAME
    bool init;

    // set by _xcb_xim_get_servers
    xcb_atom_t* server_atoms;
    int n_server_atoms;

    // used by _xcb_xim_check_server / _xcb_xim_connect
    xcb_xim_open_t open;

    // _xcb_xim_check_server
    char* trans_addr;
    xcb_window_t im_window;
    // and also server_atom

    xcb_window_t client_window;

    // _xcb_xim_connect_wait
    int major_code;
    int minor_code;
    uint32_t accept_win;

    // xim open
    bool opened;
    uint16_t connect_id;
    xcb_xim_imattr_table_t* imattr;
    xcb_xim_icattr_table_t* icattr;
};

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

bool _xcb_xim_send_message(xcb_xim_t* im,
                           uint8_t* data, size_t length);

#endif // IMCLIENT_P_H
