/*
 * Copyright (C) 2001, 2002 Apple Computer, Inc.  All rights reserved.
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

#import <qobject.h>

#import <qvariant.h>
#import <qguardedptr.h>
#import <KWQAssertions.h>
#import <KWQSignal.h>
#import <KWQSlot.h>

const QObject *QObject::m_sender;

static CFMutableDictionaryRef timerDictionaries;

@interface KWQObjectTimerTarget : NSObject
{
    QObject *target;
    int timerId;
}

- initWithQObject:(QObject *)object timerId:(int)timerId;
- (void)timerFired;

@end

KWQSignal *QObject::findSignal(const char *signalName) const
{
    for (KWQSignal *signal = m_signalListHead; signal; signal = signal->m_next) {
        if (KWQNamesMatch(signalName, signal->m_name)) {
            return signal;
        }
    }
    return 0;
}

void QObject::connect(const QObject *sender, const char *signalName, const QObject *receiver, const char *member)
{
    // FIXME: Assert that sender is not NULL rather than doing the if statement.
    if (!sender) {
        return;
    }
    
    KWQSignal *signal = sender->findSignal(signalName);
    if (!signal) {
#if !ERROR_DISABLED
        if (1
            && !KWQNamesMatch(member, SIGNAL(setStatusBarText(const QString &)))
            && !KWQNamesMatch(member, SLOT(parentDestroyed()))
            && !KWQNamesMatch(member, SLOT(slotData(KIO::Job *, const QByteArray &)))
            && !KWQNamesMatch(member, SLOT(slotFinished(KIO::Job *)))
            && !KWQNamesMatch(member, SLOT(slotHistoryChanged()))
            && !KWQNamesMatch(member, SLOT(slotJobPercent(KIO::Job *, unsigned long)))
            && !KWQNamesMatch(member, SLOT(slotJobSpeed(KIO::Job *, unsigned long)))
            && !KWQNamesMatch(member, SLOT(slotLoaderRequestDone(khtml::DocLoader *, khtml::CachedObject *)))
            && !KWQNamesMatch(member, SLOT(slotLoaderRequestStarted(khtml::DocLoader *, khtml::CachedObject *)))
            && !KWQNamesMatch(member, SLOT(slotRedirection(KIO::Job *, const KURL &)))
            && !KWQNamesMatch(member, SLOT(slotScrollBarMoved()))
            && !KWQNamesMatch(member, SLOT(slotViewCleared()))
            && !KWQNamesMatch(member, SLOT(slotWidgetDestructed()))
            )
	ERROR("connecting member %s to signal %s, but that signal was not found", member, signalName);
#endif
        return;
    }
    signal->connect(KWQSlot(const_cast<QObject *>(receiver), member));
}

void QObject::disconnect(const QObject *sender, const char *signalName, const QObject *receiver, const char *member)
{
    // FIXME: Assert that sender is not NULL rather than doing the if statement.
    if (!sender)
        return;
    
    KWQSignal *signal = sender->findSignal(signalName);
    if (!signal) {
        // FIXME: ERROR
        return;
    }
    signal->disconnect(KWQSlot(const_cast<QObject *>(receiver), member));
}

KWQObjectSenderScope::KWQObjectSenderScope(const QObject *o)
    : m_savedSender(QObject::m_sender)
{
    QObject::m_sender = o;
}

KWQObjectSenderScope::~KWQObjectSenderScope()
{
    QObject::m_sender = m_savedSender;
}

QObject::QObject(QObject *parent, const char *name)
    : m_signalListHead(0), m_signalsBlocked(false), m_eventFilterObject(0)
{
    guardedPtrDummyList.append(this);
}

QObject::~QObject()
{
    ASSERT(m_signalListHead == 0);
    killTimers();
}

void QObject::timerEvent(QTimerEvent *te)
{
}

bool QObject::event(QEvent *)
{
    return false;
}

int QObject::startTimer(int milliseconds)
{
    static int timerCount = 1;

    if (timerDictionaries == NULL) {
        // The global timers dictionary itself leaks, but the contents are removed
        // when each timer fires or is killed.
        timerDictionaries = CFDictionaryCreateMutable(NULL, 0, NULL, &kCFTypeDictionaryValueCallBacks);
    }
    
    NSMutableDictionary *timers = (NSMutableDictionary *)CFDictionaryGetValue(timerDictionaries, this);
    if (timers == nil) {
        timers = [[NSMutableDictionary alloc] init];
        CFDictionarySetValue(timerDictionaries, this, timers);
        [timers release];
    }

    NSNumber *timerId = [NSNumber numberWithInt:timerCount];
    KWQObjectTimerTarget *target = [[KWQObjectTimerTarget alloc] initWithQObject:this timerId:timerCount];
    NSTimer *timer = [NSTimer scheduledTimerWithTimeInterval:milliseconds / 1000.0
          target:target
        selector:@selector(timerFired)
        userInfo:target
         repeats:NO];
    [target release];
    [timers setObject:timer forKey:timerId];
    
    return timerCount++;    
}

void QObject::killTimer(int _timerId)
{
    if (_timerId == 0) {
        return;
    }
    if (timerDictionaries == NULL) {
        return;
    }
    NSMutableDictionary *timers = (NSMutableDictionary *)CFDictionaryGetValue(timerDictionaries, this);
    NSNumber *timerId = [NSNumber numberWithInt:_timerId];
    NSTimer *timer = (NSTimer *)[timers objectForKey:timerId];
    [timer invalidate];
    [timers removeObjectForKey:timerId];
}

void QObject::killTimers()
{
    if (timerDictionaries == NULL) {
        return;
    }
    NSMutableDictionary *timers = (NSMutableDictionary *)CFDictionaryGetValue(timerDictionaries, this);
    if (timers == nil) {
        return;
    }
    NSArray *keys = [timers allKeys];
    int count = [keys count];
    for (int i = 0; i < count; i++) {
        NSNumber *key = (NSNumber *)[keys objectAtIndex:i];
        NSTimer *timer = (NSTimer *)[timers objectForKey:key];
        [timer invalidate];
    }
    CFDictionaryRemoveValue(timerDictionaries, this);
}

@implementation KWQObjectTimerTarget

- initWithQObject:(QObject *)qo timerId:(int)t
{
    [super init];
    target = qo;
    timerId = t;
    return self;
}

- (void)timerFired
{
    QTimerEvent event(timerId);
    target->timerEvent(&event);
}

@end

// special includes only for inherits

#import <khtml_part.h>
#import <khtmlview.h>

bool QObject::inherits(const char *className) const
{
    if (strcmp(className, "KHTMLPart") == 0) {
        return dynamic_cast<const KHTMLPart *>(this);
    }
    if (strcmp(className, "KHTMLView") == 0) {
        return dynamic_cast<const KHTMLView *>(this);
    }
    if (strcmp(className, "KParts::Factory") == 0) {
        return false;
    }
    if (strcmp(className, "KParts::ReadOnlyPart") == 0) {
        return dynamic_cast<const KParts::ReadOnlyPart *>(this);
    }
    if (strcmp(className, "QFrame") == 0) {
        return dynamic_cast<const QFrame *>(this);
    }
    if (strcmp(className, "QScrollView") == 0) {
        return dynamic_cast<const QScrollView *>(this);
    }
    ERROR("class name %s not recognized", className);
    return false;
}
