/**************************************************************************

Copyright 2000, 2001 ATI Technologies Inc., Ontario, Canada, and
                     VA Linux Systems Inc., Fremont, California.
Copyright (C) The Weather Channel, Inc.  2002.  All Rights Reserved.

The Weather Channel (TM) funded Tungsten Graphics to develop the
initial release of the Radeon 8500 driver under the XFree86 license.
This notice must be preserved.

All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Gareth Hughes <gareth@valinux.com>
 *   Keith Whitwell <keith@tungstengraphics.com>
 *   Kevin E. Martin <martin@valinux.com>
 */

#ifndef COMMON_LOCK_H
#define COMMON_LOCK_H

#include "main/colormac.h"
#include "radeon_screen.h"
#include "radeon_common.h"

extern void radeonGetLock(radeonContextPtr rmesa, GLuint flags);

void radeon_lock_hardware(radeonContextPtr rmesa
#ifndef NDEBUG
		,const char* function
		,const char* file
		,const int line
#endif
		);
void radeon_unlock_hardware(radeonContextPtr rmesa);

/* Lock the hardware and validate our state.
 */
#ifdef NDEBUG
#define LOCK_HARDWARE( rmesa )	radeon_lock_hardware(rmesa)
#else
#define LOCK_HARDWARE( rmesa )	radeon_lock_hardware(rmesa, __FUNCTION__, __FILE__, __LINE__)
#endif
#define UNLOCK_HARDWARE( rmesa )  radeon_unlock_hardware(rmesa)

#endif
