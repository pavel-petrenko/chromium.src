/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WKBundleFramePrivate_h
#define WKBundleFramePrivate_h

#include <JavaScriptCore/JavaScript.h>
#include <WebKit2/WKBase.h>

#ifndef __cplusplus
#include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

WK_EXPORT WKStringRef WKBundleFrameCopyInnerText(WKBundleFrameRef frame);
WK_EXPORT WKStringRef WKBundleFrameCopyCounterValue(WKBundleFrameRef frame, JSObjectRef element);
WK_EXPORT WKStringRef WKBundleFrameCopyMarkerText(WKBundleFrameRef frame, JSObjectRef element);
WK_EXPORT JSValueRef WKBundleFrameGetComputedStyleIncludingVisitedInfo(WKBundleFrameRef frame, JSObjectRef element);
WK_EXPORT unsigned WKBundleFrameGetNumberOfActiveAnimations(WKBundleFrameRef frame);
WK_EXPORT bool WKBundleFramePauseAnimationOnElementWithId(WKBundleFrameRef frame, WKStringRef animationName, WKStringRef elementID, double time);
WK_EXPORT bool WKBundleFramePauseTransitionOnElementWithId(WKBundleFrameRef frame, WKStringRef propertyName, WKStringRef elementID, double time);
WK_EXPORT void WKBundleFrameSuspendAnimations(WKBundleFrameRef frame);
WK_EXPORT void WKBundleFrameResumeAnimations(WKBundleFrameRef frame);
WK_EXPORT unsigned WKBundleFrameGetPendingUnloadCount(WKBundleFrameRef frame);
WK_EXPORT WKStringRef WKBundleFrameCopyLayerTreeAsText(WKBundleFrameRef frame);
WK_EXPORT void WKBundleFrameClearOpener(WKBundleFrameRef frame);

WK_EXPORT bool WKBundleFrameContainsAnyFormElements(WKBundleFrameRef frame);

#ifdef __cplusplus
}
#endif

#endif /* WKBundleFramePrivate_h */
