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

#ifndef QTEXTCODEC_H_
#define QTEXTCODEC_H_

#include "qstring.h"
#include "qcstring.h"

// class QTextDecoder ==========================================================

class QTextDecoder {
public:

    // typedefs ----------------------------------------------------------------
    // enums -------------------------------------------------------------------
    // constants ---------------------------------------------------------------
    // static member functions -------------------------------------------------
    
    // constructors, copy constructors, and destructors ------------------------
    
    QTextDecoder();
    
    virtual ~QTextDecoder();
    
    // member functions --------------------------------------------------------

    virtual QString toUnicode(const char *, int)=0;

    // operators ---------------------------------------------------------------

// protected -------------------------------------------------------------------
// private ---------------------------------------------------------------------

private:
    // no copying or assignment
    QTextDecoder(const QTextDecoder &);
    QTextDecoder &operator=(const QTextDecoder &);

}; // class QTextDecoder =======================================================


// class QTextCodec ============================================================

class QTextCodec {
public:

    // typedefs ----------------------------------------------------------------
    // enums -------------------------------------------------------------------
    // constants ---------------------------------------------------------------

    // static member functions -------------------------------------------------

    static QTextCodec *codecForMib(int);
    static QTextCodec *codecForName(const char *, int accuracy=0);
    static QTextCodec *codecForLocale();

    // constructors, copy constructors, and destructors ------------------------

    QTextCodec();
    
    virtual ~QTextCodec();

    // member functions --------------------------------------------------------

    virtual const char* name() const=0;
    virtual int mibEnum() const=0;
    virtual QTextDecoder *makeDecoder() const;
    QCString fromUnicode(const QString &) const;

    virtual QString toUnicode(const char *, int) const;
    QString toUnicode(const QByteArray &, int) const;
    QString toUnicode(const char *) const;

    // operators ---------------------------------------------------------------

// protected -------------------------------------------------------------------
// private ---------------------------------------------------------------------

private:
    // no copying or assignment
    QTextCodec(const QTextCodec &);
    QTextCodec &operator=(const QTextCodec &);

}; // class QTextCodec =========================================================

#endif
