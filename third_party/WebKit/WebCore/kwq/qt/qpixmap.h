/*
 * Copyright (C) 2001 Apple Computer, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef QPIXMAP_H_
#define QPIXMAP_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "qpaintdevice.h"
#include "qcolor.h"
#include "qstring.h"
#include "qnamespace.h"
#include "qimage.h"
#include "qsize.h"
#include "qrect.h"
#include "qpainter.h"

#if (defined(__APPLE__) && defined(__OBJC__) && defined(__cplusplus))
#import <Cocoa/Cocoa.h>
#endif

class QBitmap;
class QWMatrix;

// class QPixmap ===============================================================

class QPixmap : public QPaintDevice, public Qt {
friend class QPainter;
public:

    // typedefs ----------------------------------------------------------------
    // enums -------------------------------------------------------------------
    // constants ---------------------------------------------------------------
    // static member functions -------------------------------------------------

    // constructors, copy constructors, and destructors ------------------------

    QPixmap();
    QPixmap(const QSize&);
    QPixmap(const QByteArray&);
    QPixmap(int,int);
    QPixmap(const QPixmap &);
    ~QPixmap();

    // member functions --------------------------------------------------------

    void setMask(const QBitmap &);
    const QBitmap *mask() const;
    
    bool isNull() const;

    QSize size() const;
    QRect rect() const;
    int width() const;
    int height() const;
    void resize(const QSize &);

    QPixmap xForm(const QWMatrix &) const;
    QImage convertToImage() const;

    // operators ---------------------------------------------------------------

    QPixmap &operator=(const QPixmap &);

// protected -------------------------------------------------------------------
// private ---------------------------------------------------------------------
#ifdef _KWQ_
#if (defined(__APPLE__) && defined(__OBJC__) && defined(__cplusplus))
    NSImage *nsimage;
#else
    void *nsimage;
#endif
    QWMatrix xmatrix;
#endif
}; // class QPixmap ============================================================

#endif
