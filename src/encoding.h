/*
 * (C) SPDX-FileCopyrightText: 2014 Weng Xuetian <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 *
 */
#ifndef _XCB_IMDKIT_ENCODING_H_
#define _XCB_IMDKIT_ENCODING_H_

#include "ximcommon.h"
#include <stdlib.h>

XCB_IMDKIT_DECL_BEGIN

XCB_IMDKIT_EXPORT void xcb_compound_text_init();
XCB_IMDKIT_EXPORT char *
xcb_utf8_to_compound_text(const char *utf8, size_t length, size_t *lenghtOut);
XCB_IMDKIT_EXPORT char *xcb_compound_text_to_utf8(const char *compound_text,
                                                  size_t length,
                                                  size_t *lenghtOut);

XCB_IMDKIT_DECL_END

#endif // _XCB_IMDKIT_ENCODING_H_
