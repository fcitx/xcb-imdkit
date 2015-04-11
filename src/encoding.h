/*
 * (C) Copyright 2014 Weng Xuetian <wengxt@gmail.com>
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the GNU Lesser General Public License
 * (LGPL) version 2.1 which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/lgpl-2.1.html
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 */
#ifndef _XCB_IMDKIT_ENCODING_H_
#define _XCB_IMDKIT_ENCODING_H_

#include <stdlib.h>

void xcb_compound_text_init();
char* xcb_utf8_to_compound_text(const char* utf8, size_t length, size_t *lenghtOut);
char* xcb_compound_text_to_utf8(const char* compound_text, size_t length, size_t *lenghtOut);

#endif // _XCB_IMDKIT_ENCODING_H_
