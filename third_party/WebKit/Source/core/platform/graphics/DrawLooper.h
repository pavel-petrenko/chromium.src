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

#ifndef DrawLooper_h
#define DrawLooper_h

#include "SkRefCnt.h"
#include "core/platform/graphics/Color.h"
#include "core/platform/graphics/FloatSize.h"
#include "wtf/Noncopyable.h"
#include "wtf/RefCounted.h"
#include "wtf/RefPtr.h"
#include "wtf/Vector.h"

class SkBitmap;
class SkDrawLooper;
class SkImageFilter;
class SkLayerDrawLooper;

namespace WebCore {

class DrawLooper : public RefCounted<DrawLooper> {
    // Implementing the copy constructor properly would require writing code to
    // copy the underlying SkDrawLooper.
    WTF_MAKE_NONCOPYABLE(DrawLooper);

public:
    enum ShadowTransformMode {
        ShadowRespectsTransforms,
        ShadowIgnoresTransforms
    };
    enum ShadowAlphaMode {
        ShadowRespectsAlpha,
        ShadowIgnoresAlpha
    };

    DrawLooper();
    ~DrawLooper();

    // Callees should not modify this looper other than to iterate over it.
    // A downcast to SkLayerDrawLooper* is tantamount to a const_cast.
    SkDrawLooper* skDrawLooper() const;
    SkImageFilter* imageFilter() const;

    void addUnmodifiedContent();
    void addShadow(const FloatSize& offset, float blur, const Color&,
        ShadowTransformMode = ShadowRespectsTransforms,
        ShadowAlphaMode = ShadowRespectsAlpha);

    bool shouldUseImageFilterToDrawBitmap(const SkBitmap&) const;

private:
    enum LayerType {
        ShadowLayer,
        UnmodifiedLayer
    };
    void clearCached();
    void buildCachedDrawLooper() const;
    void buildCachedImageFilter() const;

    struct DrawLooperLayerInfo {
        FloatSize m_offset;
        float m_blur;
        Color m_color;
        ShadowTransformMode m_shadowTransformMode;
        ShadowAlphaMode m_shadowAlphaMode;
        LayerType m_layerType;
    };

    typedef Vector<DrawLooperLayerInfo, 2> LayerVector;

    LayerVector m_layerInfo;

    mutable RefPtr<SkLayerDrawLooper> m_cachedDrawLooper;
    mutable RefPtr<SkImageFilter> m_cachedImageFilter;
};

} // namespace WebCore

#endif // DrawLooper_h
