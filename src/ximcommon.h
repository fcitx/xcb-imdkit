/*
 * (C) SPDX-FileCopyrightText: 2016 Weng Xuetian <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 *
 */

#ifndef XIMCOMMON_H
#define XIMCOMMON_H

#define XCB_IMDKIT_EXPORT __attribute__((visibility("default")))

#ifndef XCB_IMDKIT_DECL_BEGIN
#ifdef __cplusplus
#define XCB_IMDKIT_DECL_BEGIN extern "C" {
#else
#define XCB_IMDKIT_DECL_BEGIN
#endif
#endif

#ifndef XCB_IMDKIT_DECL_END
#ifdef __cplusplus
#define XCB_IMDKIT_DECL_END }
#else
#define XCB_IMDKIT_DECL_END
#endif
#endif

#endif
