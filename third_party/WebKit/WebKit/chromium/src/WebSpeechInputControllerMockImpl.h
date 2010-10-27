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

#ifndef WebSpeechInputControllerMockImpl_h
#define WebSpeechInputControllerMockImpl_h

#include "SpeechInputListener.h"
#include "WebSpeechInputControllerMock.h"
#include "WebSpeechInputListener.h"
#include "WebString.h"
#include <wtf/OwnPtr.h>

namespace WebCore {
class SpeechInputClientMock;
}

namespace WebKit {

struct WebRect;

class WebSpeechInputControllerMockImpl : public WebCore::SpeechInputListener
                                       , public WebSpeechInputControllerMock {
public:
    WebSpeechInputControllerMockImpl(WebSpeechInputListener*);
    virtual ~WebSpeechInputControllerMockImpl();

    // WebCore::SpeechInputListener methods.
    void didCompleteRecording(int requestId);
    void didCompleteRecognition(int requestId);
    void setRecognitionResult(int requestId, const WebCore::SpeechInputResultArray& result);

    // WebSpeechInputController methods.
    bool startRecognition(int requestId, const WebString& language, const WebRect& elementRect, const WebString& grammar);
    void cancelRecognition(int requestId);
    void stopRecording(int requestId);

    // WebSpeechInputControllerMock methods.
    void setMockRecognitionResult(const WebString& result, const WebString& language);

    // FIXME: this is a fix for a two-sided patch. Delete as soon as the chromium side is patched.
    // Chromium patch not uploaded yet, but will depend on http://codereview.chromium.org/3615005/show patch.
    void setMockRecognitionResult(const WebString& result);

private:
    OwnPtr<WebCore::SpeechInputClientMock> m_webcoreMock;
    WebSpeechInputListener* m_listener;
};

} // namespace WebKit

#endif // WebSpeechInputControllerMockImpl_h
