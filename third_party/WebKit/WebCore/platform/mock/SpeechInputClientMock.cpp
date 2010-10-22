/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "SpeechInputClientMock.h"

#if ENABLE(INPUT_SPEECH)

#include "SpeechInputListener.h"

namespace {
    // HashMap doesn't support empty strings as keys, so this value (an invalid BCP47 tag) is used for those cases.
    const String emptyLanguage = "_";
}

namespace WebCore {

SpeechInputClientMock::SpeechInputClientMock()
    : m_recording(false)
    , m_timer(this, &SpeechInputClientMock::timerFired)
    , m_listener(0)
    , m_requestId(0)
{
}

void SpeechInputClientMock::setListener(SpeechInputListener* listener)
{
    m_listener = listener;
}

bool SpeechInputClientMock::startRecognition(int requestId, const String& language, const IntRect&)
{
    if (m_timer.isActive())
        return false;
    m_requestId = requestId;
    m_recording = true;
    m_language = language;
    m_timer.startOneShot(0);
    return true;
}

void SpeechInputClientMock::stopRecording(int requestId)
{
    ASSERT(requestId == m_requestId);
    if (m_timer.isActive() && m_recording) {
        m_timer.stop();
        timerFired(&m_timer);
    }
}

void SpeechInputClientMock::cancelRecognition(int requestId)
{
    ASSERT(requestId == m_requestId);
    if (m_timer.isActive()) {
        m_timer.stop();
        m_recording = false;
        m_listener->didCompleteRecognition(m_requestId);
        m_requestId = 0;
    }
}

void SpeechInputClientMock::setRecognitionResult(const String& result, const String& language)
{
    if (language.isEmpty())
        m_recognitionResult.set(emptyLanguage, result);
    else
        m_recognitionResult.set(language, result);
}

void SpeechInputClientMock::timerFired(WebCore::Timer<SpeechInputClientMock>*)
{
    if (m_recording) {
        m_recording = false;
        m_listener->didCompleteRecording(m_requestId);
        m_timer.startOneShot(0);
    } else {
        String language = m_language.isEmpty() ? String(emptyLanguage) : m_language;
        if (!m_recognitionResult.contains(language)) {
            // Can't avoid setting a result on empty or invalid language strings.
            // This would avoid generating the events used to check the results and the test would timeout.
            String error("error: no result found for language '");
            error.append(m_language);
            error.append("'");
            m_listener->setRecognitionResult(m_requestId, error);
        } else
            m_listener->setRecognitionResult(m_requestId, m_recognitionResult.get(language));

        m_listener->didCompleteRecognition(m_requestId);
        m_requestId = 0;
    }
}

} // namespace WebCore

#endif // ENABLE(INPUT_SPEECH)
