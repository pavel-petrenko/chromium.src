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
#include <kconfig.h>

//FIX ME:
static QString *tempQString = NULL;
static QColor *tempQColor = NULL;
static QStringList *tempQStringList = NULL;


KConfigBase::KConfigBase()
{
    _logNotYetImplemented();
}


KConfigBase::~KConfigBase()
{
    _logNotYetImplemented();
}


void KConfigBase::setGroup(const QString &pGroup)
{
    _logNotYetImplemented();
}


void KConfigBase::writeEntry(const QString &pKey, const QStringList &rValue, 
    char sep=',', bool bPersistent=true, bool bGlobal=false, 
    bool bNLS=false)
{
    _logNotYetImplemented();
}



QString KConfigBase::readEntry(const char *pKey, 
    const QString& aDefault=QString::null) const
{
    _logNotYetImplemented();
    if (tempQString == NULL) {
        tempQString = new QString();
    }
    return *tempQString;
}



int KConfigBase::readNumEntry(const char *pKey, int nDefault=0) const
{
    _logNotYetImplemented();
    return 0;
}



unsigned int KConfigBase::readUnsignedNumEntry(const char *pKey, 
    unsigned int nDefault=0) const
{
    _logNotYetImplemented();
    return 0;
}


bool KConfigBase::readBoolEntry(const char *pKey, bool nDefault=0) const
{
    _logNotYetImplemented();
    return FALSE;
}


QColor KConfigBase::readColorEntry(const char *pKey, const QColor *pDefault=0L) const
{
    _logNotYetImplemented();
    if (tempQColor == NULL) {
        tempQColor = new QColor(0,0,0);
    }
    return *tempQColor;
}


QStringList KConfigBase::readListEntry(const QString &pKey, char sep=',') const
{
    _logNotYetImplemented();
    if (tempQStringList == NULL) {
        tempQStringList = new QStringList();
    }
    return *tempQStringList;
}


// class KConfig ===============================================================

KConfig::KConfig(const QString &n, bool bReadOnly=false)
{
    _logNotYetImplemented();
}


KConfig::~KConfig()
{
    _logNotYetImplemented();
}
