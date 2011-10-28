/*
 * Copyright 2000-2001 VA Linux Systems, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Keith Whitwell <keith@tungstengraphics.com>
 */

#ifndef MGATRIS_INC
#define MGATRIS_INC

#include "main/mtypes.h"

extern void mgaDDInitTriFuncs( GLcontext *ctx );
extern void mgaChooseRenderState( GLcontext *ctx );
extern void mgaRasterPrimitive( GLcontext *ctx, GLenum prim, GLuint hwprim );

extern void mgaFallback( GLcontext *ctx, GLuint bit, GLboolean mode );
#define FALLBACK( ctx, bit, mode ) mgaFallback( ctx, bit, mode )

#define _MGA_NEW_RENDERSTATE (_DD_NEW_POINT_SMOOTH |		\
			      _DD_NEW_LINE_SMOOTH |		\
			      _DD_NEW_LINE_STIPPLE |		\
			      _DD_NEW_TRI_SMOOTH |		\
			      _DD_NEW_FLATSHADE |		\
			      _DD_NEW_TRI_LIGHT_TWOSIDE |	\
			      _DD_NEW_TRI_OFFSET |		\
			      _DD_NEW_TRI_UNFILLED |		\
			      _DD_NEW_TRI_STIPPLE |		\
			      _NEW_POLYGONSTIPPLE)

#endif
