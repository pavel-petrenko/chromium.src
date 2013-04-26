/*
 * Copyright (C) 2010 Google Inc. All Rights Reserved.
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

#include "config.h"
#include "core/dom/DocumentEventQueue.h"

#include "core/dom/Document.h"
#include "core/dom/Event.h"
#include "core/dom/EventNames.h"
#include "core/dom/ScriptExecutionContext.h"
#include "core/dom/WebCoreMemoryInstrumentation.h"
#include "core/page/DOMWindow.h"
#include "core/page/SuspendableTimer.h"
#include <wtf/MemoryInstrumentationHashSet.h>
#include <wtf/MemoryInstrumentationListHashSet.h>

namespace WebCore {
    
class DocumentEventQueueTimer : public SuspendableTimer {
    WTF_MAKE_NONCOPYABLE(DocumentEventQueueTimer);
public:
    DocumentEventQueueTimer(DocumentEventQueue* eventQueue, ScriptExecutionContext* context)
        : SuspendableTimer(context)
        , m_eventQueue(eventQueue) { }

    virtual void reportMemoryUsage(MemoryObjectInfo* memoryObjectInfo) const OVERRIDE
    {
        MemoryClassInfo info(memoryObjectInfo, this, WebCoreMemoryTypes::DOM);
        SuspendableTimer::reportMemoryUsage(memoryObjectInfo);
        info.addWeakPointer(m_eventQueue);
    }

private:
    virtual void fired() { m_eventQueue->pendingEventTimerFired(); }
    DocumentEventQueue* m_eventQueue;
};

PassRefPtr<DocumentEventQueue> DocumentEventQueue::create(ScriptExecutionContext* context)
{
    return adoptRef(new DocumentEventQueue(context));
}

DocumentEventQueue::DocumentEventQueue(ScriptExecutionContext* context)
    : m_pendingEventTimer(adoptPtr(new DocumentEventQueueTimer(this, context)))
    , m_isClosed(false)
{
    m_pendingEventTimer->suspendIfNeeded();
}

DocumentEventQueue::~DocumentEventQueue()
{
}

bool DocumentEventQueue::enqueueEvent(PassRefPtr<Event> event)
{
    if (m_isClosed)
        return false;

    ASSERT(event->target());
    bool wasAdded = m_queuedEvents.add(event).isNewEntry;
    ASSERT_UNUSED(wasAdded, wasAdded); // It should not have already been in the list.
    
    if (!m_pendingEventTimer->isActive())
        m_pendingEventTimer->startOneShot(0);

    return true;
}

void DocumentEventQueue::enqueueOrDispatchScrollEvent(PassRefPtr<Node> target, ScrollEventTargetType targetType)
{
    if (!target->document()->hasListenerType(Document::SCROLL_LISTENER))
        return;

    // Per the W3C CSSOM View Module, scroll events fired at the document should bubble, others should not.
    bool canBubble = targetType == ScrollEventDocumentTarget;
    RefPtr<Event> scrollEvent = Event::create(eventNames().scrollEvent, canBubble, false /* non cancelleable */);
     
    if (!m_nodesWithQueuedScrollEvents.add(target.get()).isNewEntry)
        return;

    scrollEvent->setTarget(target);
    enqueueEvent(scrollEvent.release());
}

void DocumentEventQueue::reportMemoryUsage(MemoryObjectInfo* memoryObjectInfo) const
{
    MemoryClassInfo info(memoryObjectInfo, this, WebCoreMemoryTypes::DOM);
    info.addMember(m_pendingEventTimer, "pendingEventTimer");
    info.addMember(m_queuedEvents, "queuedEvents");
    info.addMember(m_nodesWithQueuedScrollEvents, "nodesWithQueuedScrollEvents");
}

bool DocumentEventQueue::cancelEvent(Event* event)
{
    bool found = m_queuedEvents.contains(event);
    m_queuedEvents.remove(event);
    if (m_queuedEvents.isEmpty())
        m_pendingEventTimer->stop();
    return found;
}

void DocumentEventQueue::close()
{
    m_isClosed = true;
    m_pendingEventTimer->stop();
    m_queuedEvents.clear();
}

void DocumentEventQueue::pendingEventTimerFired()
{
    ASSERT(!m_pendingEventTimer->isActive());
    ASSERT(!m_queuedEvents.isEmpty());

    m_nodesWithQueuedScrollEvents.clear();

    // Insert a marker for where we should stop.
    ASSERT(!m_queuedEvents.contains(0));
    bool wasAdded = m_queuedEvents.add(0).isNewEntry;
    ASSERT_UNUSED(wasAdded, wasAdded); // It should not have already been in the list.

    RefPtr<DocumentEventQueue> protector(this);

    while (!m_queuedEvents.isEmpty()) {
        ListHashSet<RefPtr<Event>, 16>::iterator iter = m_queuedEvents.begin();
        RefPtr<Event> event = *iter;
        m_queuedEvents.remove(iter);
        if (!event)
            break;
        dispatchEvent(event.get());
    }
}

void DocumentEventQueue::dispatchEvent(PassRefPtr<Event> event)
{
    EventTarget* eventTarget = event->target();
    if (eventTarget->toDOMWindow())
        eventTarget->toDOMWindow()->dispatchEvent(event, 0);
    else
        eventTarget->dispatchEvent(event);
}

}
