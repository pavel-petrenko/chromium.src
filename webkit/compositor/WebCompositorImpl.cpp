// Copyright 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "config.h"

#include "WebCompositorImpl.h"

#include "CCLayerTreeHost.h"
#include "CCProxy.h"
#include "CCSettings.h"
#include "CCThreadImpl.h"
#include <public/Platform.h>
#include <wtf/ThreadingPrimitives.h>

using namespace WebCore;

namespace WebKit {

bool WebCompositorImpl::s_initialized = false;
CCThread* WebCompositorImpl::s_mainThread = 0;
CCThread* WebCompositorImpl::s_implThread = 0;

void WebCompositor::initialize(WebThread* implThread)
{
    WebCompositorImpl::initialize(implThread);
}

bool WebCompositor::threadingEnabled()
{
    return WebCompositorImpl::threadingEnabled();
}

void WebCompositor::shutdown()
{
    WebCompositorImpl::shutdown();
    CCSettings::reset();
}

void WebCompositor::setPerTilePaintingEnabled(bool enabled)
{
    ASSERT(!WebCompositorImpl::initialized());
    CCSettings::setPerTilePaintingEnabled(enabled);
}

void WebCompositor::setPartialSwapEnabled(bool enabled)
{
    ASSERT(!WebCompositorImpl::initialized());
    CCSettings::setPartialSwapEnabled(enabled);
}

void WebCompositor::setAcceleratedAnimationEnabled(bool enabled)
{
    ASSERT(!WebCompositorImpl::initialized());
    CCSettings::setAcceleratedAnimationEnabled(enabled);
}

void WebCompositorImpl::initialize(WebThread* implThread)
{
    ASSERT(!s_initialized);
    s_initialized = true;

    s_mainThread = CCThreadImpl::create(WebKit::Platform::current()->currentThread()).leakPtr();
    CCProxy::setMainThread(s_mainThread);
    if (implThread) {
        s_implThread = CCThreadImpl::create(implThread).leakPtr();
        CCProxy::setImplThread(s_implThread);
    } else
        CCProxy::setImplThread(0);
}

bool WebCompositorImpl::threadingEnabled()
{
    return s_implThread;
}

bool WebCompositorImpl::initialized()
{
    return s_initialized;
}

void WebCompositorImpl::shutdown()
{
    ASSERT(s_initialized);
    ASSERT(!CCLayerTreeHost::anyLayerTreeHostInstanceExists());

    if (s_implThread) {
        delete s_implThread;
        s_implThread = 0;
    }
    delete s_mainThread;
    s_mainThread = 0;
    CCProxy::setImplThread(0);
    CCProxy::setMainThread(0);
    s_initialized = false;
}

}
