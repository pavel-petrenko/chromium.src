// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "config.h"

#include "WebTransformAnimationCurveImpl.h"

#include "CCKeyframedAnimationCurve.h"
#include "CCTimingFunction.h"
#include "WebAnimationCurveCommon.h"
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>

namespace WebKit {

WebTransformAnimationCurve* WebTransformAnimationCurve::create()
{
    return new WebTransformAnimationCurveImpl(WebCore::CCKeyframedTransformAnimationCurve::create());
}

WebTransformAnimationCurveImpl::WebTransformAnimationCurveImpl(PassOwnPtr<WebCore::CCKeyframedTransformAnimationCurve> curve)
    : m_curve(curve)
{
}

WebTransformAnimationCurveImpl::~WebTransformAnimationCurveImpl()
{
}

WebAnimationCurve::AnimationCurveType WebTransformAnimationCurveImpl::type() const
{
    return WebAnimationCurve::AnimationCurveTypeTransform;
}

void WebTransformAnimationCurveImpl::add(const WebTransformKeyframe& keyframe)
{
    add(keyframe, TimingFunctionTypeEase);
}

void WebTransformAnimationCurveImpl::add(const WebTransformKeyframe& keyframe, TimingFunctionType type)
{
    m_curve->addKeyframe(WebCore::CCTransformKeyframe::create(keyframe.time, keyframe.value, createTimingFunction(type)));
}

void WebTransformAnimationCurveImpl::add(const WebTransformKeyframe& keyframe, double x1, double y1, double x2, double y2)
{
    m_curve->addKeyframe(WebCore::CCTransformKeyframe::create(keyframe.time, keyframe.value, WebCore::CCCubicBezierTimingFunction::create(x1, y1, x2, y2)));
}

WebTransformationMatrix WebTransformAnimationCurveImpl::getValue(double time) const
{
    return m_curve->getValue(time);
}

PassOwnPtr<WebCore::CCAnimationCurve> WebTransformAnimationCurveImpl::cloneToCCAnimationCurve() const
{
    return m_curve->clone();
}

} // namespace WebKit
