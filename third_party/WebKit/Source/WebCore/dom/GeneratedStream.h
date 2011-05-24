/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
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

#ifndef GeneratedStream_h
#define GeneratedStream_h

#if ENABLE(MEDIA_STREAM)

#include "Stream.h"
#include <wtf/Forward.h>

namespace WebCore {

class GeneratedStream : public Stream {
public:
    // FIXME: add audio and video tracks when available.
    static PassRefPtr<GeneratedStream> create(MediaStreamFrameController*, const String& label);
    virtual ~GeneratedStream();

    void stop();

    // MediaStreamFrameController::StreamClient implementation.
    virtual void detachEmbedder();

    // EventTarget.
    virtual GeneratedStream* toGeneratedStream();

private:
    GeneratedStream(MediaStreamFrameController*, const String& label);
    class DispatchUpdateTask;
    friend class DispatchUpdateTask;

    void onStop();
};

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)

#endif // GeneratedStream_h
