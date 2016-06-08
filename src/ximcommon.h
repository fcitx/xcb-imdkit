/*
 * (C) Copyright 2016 Weng Xuetian <wengxt@gmail.com>
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

#ifndef XIMCOMMON_H
#define XIMCOMMON_H

#define XCB_IMDKIT_EXPORT __attribute__ ((visibility("default")))

#ifndef XCB_IMDKIT_DECL_BEGIN
#  ifdef __cplusplus
#    define XCB_IMDKIT_DECL_BEGIN extern "C" {
#  else
#    define XCB_IMDKIT_DECL_BEGIN
#  endif
#endif

#ifndef XCB_IMDKIT_DECL_END
#  ifdef __cplusplus
#    define XCB_IMDKIT_DECL_END }
#  else
#    define XCB_IMDKIT_DECL_END
#  endif
#endif

#endif
