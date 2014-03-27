/*
 * Copyright (C) 2011, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(WEB_AUDIO)

#include "modules/webaudio/OfflineAudioCompletionEvent.h"

namespace WebCore {

PassRefPtrWillBeRawPtr<OfflineAudioCompletionEvent> OfflineAudioCompletionEvent::create()
{
    return adoptRefWillBeRefCountedGarbageCollected(new OfflineAudioCompletionEvent);
}

PassRefPtrWillBeRawPtr<OfflineAudioCompletionEvent> OfflineAudioCompletionEvent::create(PassRefPtr<AudioBuffer> renderedBuffer)
{
    return adoptRefWillBeRefCountedGarbageCollected(new OfflineAudioCompletionEvent(renderedBuffer));
}

OfflineAudioCompletionEvent::OfflineAudioCompletionEvent()
{
    ScriptWrappable::init(this);
}

OfflineAudioCompletionEvent::OfflineAudioCompletionEvent(PassRefPtr<AudioBuffer> renderedBuffer)
    : Event(EventTypeNames::complete, true, false)
    , m_renderedBuffer(renderedBuffer)
{
    ScriptWrappable::init(this);
}

OfflineAudioCompletionEvent::~OfflineAudioCompletionEvent()
{
}

const AtomicString& OfflineAudioCompletionEvent::interfaceName() const
{
    return EventNames::OfflineAudioCompletionEvent;
}

void OfflineAudioCompletionEvent::trace(Visitor* visitor)
{
    Event::trace(visitor);
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
