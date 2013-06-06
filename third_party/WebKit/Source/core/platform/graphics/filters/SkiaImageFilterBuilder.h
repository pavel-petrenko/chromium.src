/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SkiaImageFilterBuilder_h
#define SkiaImageFilterBuilder_h

#include "core/platform/graphics/ColorSpace.h"
#include <wtf/HashMap.h>

class SkImageFilter;

namespace WebCore {
class FilterEffect;
class FilterOperations;

class SkiaImageFilterBuilder {
public:
    SkiaImageFilterBuilder();
    ~SkiaImageFilterBuilder();

    SkImageFilter* build(FilterEffect*, ColorSpace);
    SkImageFilter* build(const FilterOperations&);

    SkImageFilter* transformColorSpace(
        SkImageFilter* input, ColorSpace srcColorSpace, ColorSpace dstColorSpace);
private:
    typedef std::pair<FilterEffect*, ColorSpace> FilterColorSpacePair;
    typedef HashMap<FilterColorSpacePair, SkImageFilter*> FilterBuilderHashMap;
    FilterBuilderHashMap m_map;
};

} // namespace WebCore

namespace WTF {

template<> struct DefaultHash<WebCore::FilterEffect*> {
    typedef PtrHash<WebCore::FilterEffect*> Hash;
};
template<> struct DefaultHash<WebCore::ColorSpace> {
    typedef IntHash<unsigned> Hash;
};

} // namespace WTF

#endif
