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
#include <qevent.h>

// class QEvent ================================================================


QEvent::~QEvent()
{
    //_logNotYetImplemented();
}


QEvent::Type QEvent::type() const
{
    return _type;
}


// class QMouseEvent ===========================================================

QMouseEvent::QMouseEvent( Type t, const QPoint &pos, int b, int s )
    : QEvent(t), _position(pos), _button(b), _state((ushort)s){
}

int QMouseEvent::x()
{
    return (int)_position.x();
}


int QMouseEvent::y()
{
    return (int)_position.y();
}


int QMouseEvent::globalX()
{
    _logNotYetImplemented();
    return 0;
}


int QMouseEvent::globalY()
{
    _logNotYetImplemented();
    return 0;
}


const QPoint &QMouseEvent::pos() const
{
    return _position;
}


Qt::ButtonState QMouseEvent::button()
{
    return Qt::ButtonState(_button);
}


Qt::ButtonState QMouseEvent::state()
{
    return Qt::ButtonState(_state);
}


Qt::ButtonState QMouseEvent::stateAfter()
{
    return Qt::ButtonState(_state);
}


// class QTimerEvent ===========================================================

QTimerEvent::QTimerEvent(int timerId)
{
    _logNotYetImplemented();
}


QTimerEvent::~QTimerEvent()
{
    _logNotYetImplemented();
}


int QTimerEvent::timerId() const
{
    _logNotYetImplemented();
    return 0;
}


// class QKeyEvent =============================================================

QKeyEvent::QKeyEvent(Type, Key, int, int)
{
    _logNotYetImplemented();
}


int QKeyEvent::key() const
{
    _logNotYetImplemented();
    return 0;
}


Qt::ButtonState QKeyEvent::state() const
{
    _logNotYetImplemented();
    return Qt::NoButton;
}


void QKeyEvent::accept()
{
    _logNotYetImplemented();
}


void QKeyEvent::ignore()
{
    _logNotYetImplemented();
}


// class QFocusEvent ===========================================================

QFocusEvent::QFocusEvent(Type)
{
    _logNotYetImplemented();
}


// class QHideEvent ============================================================

QHideEvent::QHideEvent(Type)
{
    _logNotYetImplemented();
}


// class QResizeEvent ==========================================================

QResizeEvent::QResizeEvent(Type)
{
    _logNotYetImplemented();
}


// class QShowEvent ============================================================

QShowEvent::QShowEvent(Type)
{
    _logNotYetImplemented();
}


// class QWheelEvent ===========================================================

QWheelEvent::QWheelEvent(Type)
{
    _logNotYetImplemented();
}


void QWheelEvent::accept()
{
    _logNotYetImplemented();
}


void QWheelEvent::ignore()
{
    _logNotYetImplemented();
}


// class QCustomEvent ===========================================================

QCustomEvent::QCustomEvent( int type )
    : QEvent( (QEvent::Type)type ), d( 0 )
{
}
