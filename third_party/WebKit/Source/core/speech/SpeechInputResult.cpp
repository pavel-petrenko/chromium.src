/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "core/speech/SpeechInputResult.h"

#if ENABLE(INPUT_SPEECH)

namespace WebCore {

PassRefPtrWillBeRawPtr<SpeechInputResult> SpeechInputResult::create(const String& utterance, double confidence)
{
    return adoptRefWillBeNoop(new SpeechInputResult(utterance, confidence));
}

PassRefPtrWillBeRawPtr<SpeechInputResult> SpeechInputResult::create(const SpeechInputResult& source)
{
    return adoptRefWillBeNoop(new SpeechInputResult(source.m_utterance, source.m_confidence));
}

SpeechInputResult::SpeechInputResult(const String& utterance, double confidence)
    : m_utterance(utterance)
    , m_confidence(confidence)
{
    ScriptWrappable::init(this);
}

double SpeechInputResult::confidence() const
{
    return m_confidence;
}

const String& SpeechInputResult::utterance() const
{
    return m_utterance;
}

} // namespace WebCore

#endif // ENABLE(INPUT_SPEECH)
