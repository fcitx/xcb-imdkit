/*
 * SPDX-FileCopyrightText: 2014 Weng Xuetian <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 *
 */
#ifndef _XCB_IMDKIT_ENCODING_H_
#define _XCB_IMDKIT_ENCODING_H_

#include "xcbimdkit_export.h" // IWYU pragma: export
#include "ximcommon.h"        // IWYU pragma: export
#include <stdlib.h>

XCBIMDKIT_DECL_BEGIN

/**
 * Initialize global compound text encoding tables.
 *
 * This function is not thread-safe and must be called once before calling
 * xcb_utf8_to_compound_text or xcb_compound_text_to_utf8.
 */
XCBIMDKIT_EXPORT void xcb_compound_text_init();
/**
 * Convert a UTF-8 string to Compound Text.
 *
 * xcb_compound_text_init must be called before using this function.
 */
XCBIMDKIT_EXPORT char *
xcb_utf8_to_compound_text(const char *utf8, size_t length, size_t *lenghtOut);
/**
 * Convert a Compound Text string to UTF-8.
 *
 * xcb_compound_text_init must be called before using this function.
 */
XCBIMDKIT_EXPORT char *xcb_compound_text_to_utf8(const char *compound_text,
                                                 size_t length,
                                                 size_t *lenghtOut);

XCBIMDKIT_DECL_END

#endif // _XCB_IMDKIT_ENCODING_H_
