/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 *
 */

#ifndef ActiveDOMObject_h
#define ActiveDOMObject_h

#include "core/dom/ContextLifecycleObserver.h"
#include "wtf/Assertions.h"

namespace WebCore {

class ActiveDOMObject : public ContextLifecycleObserver {
public:
    ActiveDOMObject(ExecutionContext*);

    // suspendIfNeeded() should be called exactly once after object construction to synchronize
    // the suspend state with that in ExecutionContext.
    void suspendIfNeeded();
#if !ASSERT_DISABLED
    bool suspendIfNeededCalled() const { return m_suspendIfNeededCalled; }
#endif

    // Should return true if there's any pending asynchronous activity, and so
    // this object must not be garbage collected.
    //
    // Default implementation is that it returns true iff
    // m_pendingActivityCount is non-zero.
    virtual bool hasPendingActivity() const;

    // These methods have an empty default implementation so that subclasses
    // which don't need special treatment can skip implementation.
    virtual void suspend();
    virtual void resume();
    virtual void stop();

    void didMoveToNewExecutionContext(ExecutionContext*);

protected:
    virtual ~ActiveDOMObject();

    template<class T> void setPendingActivity(T* thisObject)
    {
        ASSERT(thisObject == this);
        thisObject->ref();
        m_pendingActivityCount++;
    }

    template<class T> void unsetPendingActivity(T* thisObject)
    {
        ASSERT(m_pendingActivityCount > 0);
        --m_pendingActivityCount;
        thisObject->deref();
    }

private:
    unsigned m_pendingActivityCount;
#if !ASSERT_DISABLED
    bool m_suspendIfNeededCalled;
#endif
};

} // namespace WebCore

#endif // ActiveDOMObject_h
