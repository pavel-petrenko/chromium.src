/*
 * Copyright (C) 2004, 2005, 2006, 2007 Apple Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

/* This prefix file should contain only:
 *    1) files to precompile for faster builds
 *    2) in one case at least: OS-X-specific performance bug workarounds
 *    3) the special trick to catch us using new or delete without including "config.h"
 * The project should be able to build without this header, although we rarely test that.
 */

/* Things that need to be defined globally should go into "config.h". */

#if defined(__APPLE__)
#ifdef __cplusplus
#define NULL __null
#else
#define NULL ((void *)0)
#endif
#endif

#if defined(WIN32) || defined(_WIN32)

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif

#ifndef WINVER
#define WINVER 0x0500
#endif

#ifndef WTF_USE_CURL
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_ // Prevent inclusion of winsock.h in windows.h
#endif
#endif

// If we don't define these, they get defined in windef.h. 
// We want to use std::min and std::max
#ifdef __cplusplus
#define max max
#define min min
#endif

#else
#include <pthread.h>
#endif // defined(WIN32) || defined(_WIN32)

#if defined(ANDROID)
#ifdef __cplusplus
// Must come before include of algorithm.
#define PREFIX_FOR_WEBCORE 1
#define EXPORT __attribute__((visibility("default")))
#endif
// Android uses a single set of include directories when building WebKit and
// JavaScriptCore. Since WebCore/ is included before JavaScriptCore/, Android
// includes JavaScriptCore/config.h explicitly here to make sure it gets picked
// up.
#include <JavaScriptCore/config.h>
#endif

#include <sys/types.h>
#include <fcntl.h>
#if defined(__APPLE__)
#include <regex.h>
#endif

// On Linux this causes conflicts with libpng because there are two impls. of
// longjmp - see here: https://bugs.launchpad.net/ubuntu/+source/libpng/+bug/218409
#ifndef BUILDING_WX__
#include <setjmp.h>
#endif

#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#if defined(__APPLE__)
#include <unistd.h>
#endif

#ifdef __cplusplus

#include <algorithm>
#include <cstddef>
#include <new>

#endif

#include <sys/types.h>
#if defined(__APPLE__)
#include <sys/param.h>
#endif
#include <sys/stat.h>
#if defined(__APPLE__)
#include <sys/time.h>
#include <sys/resource.h>
#endif

#include <time.h>

#if !defined(BUILDING_WX__) && !defined(ANDROID)
#include <CoreFoundation/CoreFoundation.h>
#ifdef WIN_CAIRO
#include <ConditionalMacros.h>
#include <windows.h>
#include <stdio.h>
#else
#include <CoreServices/CoreServices.h>

#if defined(WIN32) || defined(_WIN32)
/* Including CoreServices.h on Windows doesn't include CFNetwork.h, so we do
   it explicitly here to make Windows more consistent with Mac. */
#include <CFNetwork/CFNetwork.h>
#endif

#endif
#endif  // !defined(BUILDING_WX__) && !defined(ANDROID)

#ifdef __OBJC__
#import <Cocoa/Cocoa.h>
#endif

#ifdef __cplusplus
#define new ("if you use new/delete make sure to include config.h at the top of the file"()) 
#define delete ("if you use new/delete make sure to include config.h at the top of the file"()) 
#endif

/* When C++ exceptions are disabled, the C++ library defines |try| and |catch|
 * to allow C++ code that expects exceptions to build. These definitions
 * interfere with Objective-C++ uses of Objective-C exception handlers, which
 * use |@try| and |@catch|. As a workaround, undefine these macros. */
#ifdef __OBJC__
#undef try
#undef catch
#endif
