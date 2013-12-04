/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
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

#ifndef NotificationPresenterImpl_h
#define NotificationPresenterImpl_h

#include "core/html/VoidCallback.h"
#include "modules/notifications/NotificationClient.h"

#include "wtf/HashMap.h"
#include "wtf/PassRefPtr.h"

namespace blink {

class WebNotificationPresenter;

class NotificationPresenterImpl : public WebCore::NotificationClient {
public:
    NotificationPresenterImpl() : m_presenter(0) { }

    void initialize(WebNotificationPresenter* presenter);
    bool isInitialized();

    // WebCore::NotificationPresenter implementation.
    virtual bool show(WebCore::NotificationBase*);
    virtual void cancel(WebCore::NotificationBase*);
    virtual void notificationObjectDestroyed(WebCore::NotificationBase*);
    virtual void notificationControllerDestroyed();
    virtual WebCore::NotificationClient::Permission checkPermission(WebCore::ExecutionContext*);
#if ENABLE(LEGACY_NOTIFICATIONS)
    virtual void requestPermission(WebCore::ExecutionContext*, WTF::PassOwnPtr<WebCore::VoidCallback>);
#endif
    virtual void requestPermission(WebCore::ExecutionContext*, WTF::PassOwnPtr<WebCore::NotificationPermissionCallback>);
    virtual void cancelRequestsForPermission(WebCore::ExecutionContext*) { }

private:
    // WebNotificationPresenter that this object delegates to.
    WebNotificationPresenter* m_presenter;
};

} // namespace blink

#endif // NotificationPresenterImpl_h
