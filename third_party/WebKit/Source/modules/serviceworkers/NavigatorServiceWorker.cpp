/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
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
#include "modules/serviceworkers/NavigatorServiceWorker.h"

#include "RuntimeEnabledFeatures.h"
#include "V8ServiceWorker.h"
#include "bindings/v8/CallbackPromiseAdapter.h"
#include "bindings/v8/ScriptPromiseResolver.h"
#include "core/dom/Document.h"
#include "core/dom/ExceptionCode.h"
#include "core/dom/ExecutionContext.h"
#include "core/frame/Frame.h"
#include "core/frame/Navigator.h"
#include "core/loader/DocumentLoader.h"
#include "core/loader/FrameLoaderClient.h"
#include "core/workers/SharedWorker.h"
#include "modules/serviceworkers/ServiceWorker.h"
#include "public/platform/WebServiceWorkerProvider.h"
#include "public/platform/WebServiceWorkerProviderClient.h"
#include "public/platform/WebString.h"
#include "public/platform/WebURL.h"

using WebKit::WebServiceWorkerProvider;
using WebKit::WebString;

namespace WebCore {

NavigatorServiceWorker::NavigatorServiceWorker(Navigator* navigator)
    : DOMWindowProperty(navigator->frame())
    , m_navigator(navigator)
{
}

NavigatorServiceWorker::~NavigatorServiceWorker()
{
}

const char* NavigatorServiceWorker::supplementName()
{
    return "NavigatorServiceWorker";
}

WebServiceWorkerProvider* NavigatorServiceWorker::ensureProvider()
{
    ASSERT(m_navigator->frame());
    if (!m_provider) {
        Frame* frame = m_navigator->frame();

        FrameLoaderClient* client = frame->loader()->client();
        // FIXME: This is temporarily hooked up here until we hook up to the loading process.
        m_provider = client->createServiceWorkerProvider(nullptr);
    }
    return m_provider.get();
}

NavigatorServiceWorker* NavigatorServiceWorker::from(Navigator* navigator)
{
    NavigatorServiceWorker* supplement = toNavigatorServiceWorker(navigator);
    if (!supplement) {
        supplement = new NavigatorServiceWorker(navigator);
        provideTo(navigator, supplementName(), adoptPtr(supplement));
    }
    return supplement;
}

ScriptPromise NavigatorServiceWorker::registerServiceWorker(ExecutionContext* context, Navigator* navigator, const String& pattern, const String& url, ExceptionState& es)
{
    return from(navigator)->registerServiceWorker(context, pattern, url, es);
}

ScriptPromise NavigatorServiceWorker::registerServiceWorker(ExecutionContext* executionContext, const String& pattern, const String& scriptSrc, ExceptionState& es)
{
    ASSERT(RuntimeEnabledFeatures::serviceWorkerEnabled());
    Frame* frame = m_navigator->frame();

    if (!frame) {
        es.throwDOMException(InvalidStateError, "No document available.");
        return ScriptPromise();
    }

    RefPtr<SecurityOrigin> documentOrigin = frame->document()->securityOrigin();

    KURL patternURL = executionContext->completeURL(pattern);
    if (documentOrigin->canRequest(patternURL)) {
        es.throwSecurityError("Can only register for patterns in the document's origin.");
        return ScriptPromise();
    }

    KURL scriptURL = executionContext->completeURL(scriptSrc);
    if (documentOrigin->canRequest(scriptURL)) {
        es.throwSecurityError("Script must be in document's origin.");
        return ScriptPromise();
    }

    ScriptPromise promise = ScriptPromise::createPending(executionContext);
    RefPtr<ScriptPromiseResolver> resolver = ScriptPromiseResolver::create(promise, executionContext);
    ensureProvider()->registerServiceWorker(patternURL, scriptURL, new CallbackPromiseAdapter<ServiceWorker, ServiceWorker>(resolver, executionContext));
    return promise;
}

ScriptPromise NavigatorServiceWorker::unregisterServiceWorker(ExecutionContext* context, Navigator* navigator, const String& pattern, ExceptionState& es)
{
    return from(navigator)->unregisterServiceWorker(context, pattern, es);
}

ScriptPromise NavigatorServiceWorker::unregisterServiceWorker(ExecutionContext* executionContext, const String& pattern, ExceptionState& es)
{
    ASSERT(RuntimeEnabledFeatures::serviceWorkerEnabled());
    Frame* frame = m_navigator->frame();
    if (!frame) {
        es.throwDOMException(InvalidStateError, "No document available.");
        return ScriptPromise();
    }

    RefPtr<SecurityOrigin> documentOrigin = frame->document()->securityOrigin();

    KURL patternURL = executionContext->completeURL(pattern);
    if (documentOrigin->canRequest(patternURL)) {
        es.throwSecurityError("Can only unregister for patterns in the document's origin.");
        return ScriptPromise();
    }

    ScriptPromise promise = ScriptPromise::createPending(executionContext);
    RefPtr<ScriptPromiseResolver> resolver = ScriptPromiseResolver::create(promise, executionContext);
    ensureProvider()->unregisterServiceWorker(patternURL, new CallbackPromiseAdapter<ServiceWorker, ServiceWorker>(resolver, executionContext));
    return promise;
}

void NavigatorServiceWorker::willDetachGlobalObjectFromFrame()
{
    m_provider = nullptr;
    DOMWindowProperty::willDetachGlobalObjectFromFrame();
}


} // namespace WebCore
