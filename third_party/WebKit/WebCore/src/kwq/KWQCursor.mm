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

#include <kwqdebug.h>
#include <qcursor.h>

const QCursor & Qt::sizeAllCursor = QCursor();
const QCursor & Qt::splitHCursor = QCursor();
const QCursor & Qt::splitVCursor = QCursor();
const QCursor & Qt::sizeHorCursor = QCursor();
const QCursor & Qt::sizeVerCursor = QCursor();

QCursor::QCursor()
{
    //_logNotYetImplemented();
}

QCursor::QCursor(const QPixmap &pixmap, int hotX, int hotY)
{
    _logNotYetImplemented();
}

QCursor::QCursor(const QCursor &)
{
    _logNotYetImplemented();
}

QCursor::~QCursor()
{
    //_logNotYetImplemented();
}
      
QPoint QCursor::pos()
{
    _logNotYetImplemented();
    return QPoint();
}

QCursor &QCursor::operator=(const QCursor &)
{
    _logNotYetImplemented();
    return *this;
}

int QCursor::handle()
{
    _logNotYetImplemented();
    return 0;
}
