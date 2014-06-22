#include <string.h>
#include <stdlib.h>
#include "common.h"

bool _xcb_im_init_atoms(xcb_connection_t* conn, size_t n, const char** atom_names, xcb_atom_t* atoms)
{
    // here we alloc some array on stack, but since we only use this function internally
    // and all atom_names size are small (less than 10), so it doesn't matter.
    xcb_intern_atom_cookie_t atom_cookies[n];
    for (size_t i = 0; i < n; i ++) {
        atom_cookies[i] = xcb_intern_atom(conn, false, strlen(atom_names[i]), atom_names[i]);
    }
    size_t i;
    for (i = 0; i < n; i ++) {
        xcb_intern_atom_reply_t* atom_reply = xcb_intern_atom_reply(conn, atom_cookies[i], NULL);
        if (atom_reply) {
            atoms[i] = atom_reply->atom;
            free(atom_reply);
        } else {
            break;
        }
    }
    return (i == n);
}
