/*
 * CSS Media Query Evaluator
 *
 * Copyright (C) 2006 Kimmo Kinnunen <kimmo.t.kinnunen@nokia.com>.
 * Copyright (C) 2013 Apple Inc. All rights reserved.
 * Copyright (C) 2013 Intel Corporation. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "core/css/MediaQueryEvaluator.h"

#include "CSSValueKeywords.h"
#include "MediaFeatureNames.h"
#include "MediaFeatures.h"
#include "MediaTypeNames.h"
#include "core/css/CSSAspectRatioValue.h"
#include "core/css/CSSHelper.h"
#include "core/css/CSSPrimitiveValue.h"
#include "core/css/CSSToLengthConversionData.h"
#include "core/css/MediaList.h"
#include "core/css/MediaQuery.h"
#include "core/css/MediaValues.h"
#include "core/css/resolver/MediaQueryResult.h"
#include "core/dom/NodeRenderStyle.h"
#include "core/frame/FrameHost.h"
#include "core/frame/FrameView.h"
#include "core/frame/LocalFrame.h"
#include "core/frame/Settings.h"
#include "core/inspector/InspectorInstrumentation.h"
#include "core/rendering/RenderView.h"
#include "core/rendering/compositing/RenderLayerCompositor.h"
#include "core/rendering/style/RenderStyle.h"
#include "platform/PlatformScreen.h"
#include "platform/geometry/FloatRect.h"
#include "wtf/HashMap.h"

namespace WebCore {

using namespace MediaFeatureNames;

enum MediaFeaturePrefix { MinPrefix, MaxPrefix, NoPrefix };

typedef bool (*EvalFunc)(CSSValue*, MediaFeaturePrefix, const MediaValues&);
typedef HashMap<StringImpl*, EvalFunc> FunctionMap;
static FunctionMap* gFunctionMap;

MediaQueryEvaluator::MediaQueryEvaluator(bool mediaFeatureResult)
    : m_expectedResult(mediaFeatureResult)
{
}

MediaQueryEvaluator::MediaQueryEvaluator(const String& acceptedMediaType, bool mediaFeatureResult)
    : m_mediaType(acceptedMediaType)
    , m_expectedResult(mediaFeatureResult)
{
}

MediaQueryEvaluator::MediaQueryEvaluator(const char* acceptedMediaType, bool mediaFeatureResult)
    : m_mediaType(acceptedMediaType)
    , m_expectedResult(mediaFeatureResult)
{
}

MediaQueryEvaluator::MediaQueryEvaluator(const String& acceptedMediaType, LocalFrame* frame, RenderStyle* style)
    : m_mediaType(acceptedMediaType)
    , m_expectedResult(false) // Doesn't matter when we have m_frame and m_style.
    , m_mediaValues(MediaValues::create(frame, style, MediaValues::DynamicMode))
{
}

MediaQueryEvaluator::MediaQueryEvaluator(const String& acceptedMediaType, const MediaValues& mediaValues)
    : m_mediaType(acceptedMediaType)
    , m_expectedResult(false) // Doesn't matter when we have mediaValues.
    , m_mediaValues(mediaValues.copy())
{
}

MediaQueryEvaluator::~MediaQueryEvaluator()
{
}

bool MediaQueryEvaluator::mediaTypeMatch(const String& mediaTypeToMatch) const
{
    return mediaTypeToMatch.isEmpty()
        || equalIgnoringCase(mediaTypeToMatch, MediaTypeNames::all)
        || equalIgnoringCase(mediaTypeToMatch, m_mediaType);
}

bool MediaQueryEvaluator::mediaTypeMatchSpecific(const char* mediaTypeToMatch) const
{
    // Like mediaTypeMatch, but without the special cases for "" and "all".
    ASSERT(mediaTypeToMatch);
    ASSERT(mediaTypeToMatch[0] != '\0');
    ASSERT(!equalIgnoringCase(mediaTypeToMatch, MediaTypeNames::all));
    return equalIgnoringCase(mediaTypeToMatch, m_mediaType);
}

static bool applyRestrictor(MediaQuery::Restrictor r, bool value)
{
    return r == MediaQuery::Not ? !value : value;
}

bool MediaQueryEvaluator::eval(const MediaQuerySet* querySet, MediaQueryResultList* viewportDependentMediaQueryResults) const
{
    if (!querySet)
        return true;

    const WillBeHeapVector<OwnPtrWillBeMember<MediaQuery> >& queries = querySet->queryVector();
    if (!queries.size())
        return true; // Empty query list evaluates to true.

    // Iterate over queries, stop if any of them eval to true (OR semantics).
    bool result = false;
    for (size_t i = 0; i < queries.size() && !result; ++i) {
        MediaQuery* query = queries[i].get();

        if (mediaTypeMatch(query->mediaType())) {
            const ExpressionHeapVector& expressions = query->expressions();
            // Iterate through expressions, stop if any of them eval to false (AND semantics).
            size_t j = 0;
            for (; j < expressions.size(); ++j) {
                bool exprResult = eval(expressions.at(j).get());
                if (viewportDependentMediaQueryResults && expressions.at(j)->isViewportDependent())
                    viewportDependentMediaQueryResults->append(adoptRefWillBeNoop(new MediaQueryResult(*expressions.at(j), exprResult)));
                if (!exprResult)
                    break;
            }

            // Assume true if we are at the end of the list, otherwise assume false.
            result = applyRestrictor(query->restrictor(), expressions.size() == j);
        } else {
            result = applyRestrictor(query->restrictor(), false);
        }
    }

    return result;
}

template<typename T>
bool compareValue(T a, T b, MediaFeaturePrefix op)
{
    switch (op) {
    case MinPrefix:
        return a >= b;
    case MaxPrefix:
        return a <= b;
    case NoPrefix:
        return a == b;
    }
    return false;
}

static bool compareAspectRatioValue(CSSValue* value, int width, int height, MediaFeaturePrefix op)
{
    if (value->isAspectRatioValue()) {
        CSSAspectRatioValue* aspectRatio = toCSSAspectRatioValue(value);
        return compareValue(width * static_cast<int>(aspectRatio->denominatorValue()), height * static_cast<int>(aspectRatio->numeratorValue()), op);
    }

    return false;
}

static bool numberValue(CSSValue* value, float& result)
{
    if (value->isPrimitiveValue()
        && toCSSPrimitiveValue(value)->isNumber()) {
        result = toCSSPrimitiveValue(value)->getFloatValue(CSSPrimitiveValue::CSS_NUMBER);
        return true;
    }
    return false;
}

static bool colorMediaFeatureEval(CSSValue* value, MediaFeaturePrefix op, const MediaValues& mediaValues)
{
    float number;
    int bitsPerComponent = mediaValues.colorBitsPerComponent();
    if (value)
        return numberValue(value, number) && compareValue(bitsPerComponent, static_cast<int>(number), op);

    return bitsPerComponent != 0;
}

static bool colorIndexMediaFeatureEval(CSSValue* value, MediaFeaturePrefix op, const MediaValues&)
{
    // FIXME: We currently assume that we do not support indexed displays, as it is unknown
    // how to retrieve the information if the display mode is indexed. This matches Firefox.
    if (!value)
        return false;

    // Acording to spec, if the device does not use a color lookup table, the value is zero.
    float number;
    return numberValue(value, number) && compareValue(0, static_cast<int>(number), op);
}

static bool monochromeMediaFeatureEval(CSSValue* value, MediaFeaturePrefix op, const MediaValues& mediaValues)
{
    if (!mediaValues.monochromeBitsPerComponent()) {
        if (value) {
            float number;
            return numberValue(value, number) && compareValue(0, static_cast<int>(number), op);
        }
        return false;
    }

    return colorMediaFeatureEval(value, op, mediaValues);
}

static bool orientationMediaFeatureEval(CSSValue* value, MediaFeaturePrefix, const MediaValues& mediaValues)
{
    int width = mediaValues.viewportWidth();
    int height = mediaValues.viewportHeight();

    if (value && value->isPrimitiveValue()) {
        const CSSValueID id = toCSSPrimitiveValue(value)->getValueID();
        if (width > height) // Square viewport is portrait.
            return CSSValueLandscape == id;
        return CSSValuePortrait == id;
    }

    // Expression (orientation) evaluates to true if width and height >= 0.
    return height >= 0 && width >= 0;
}

static bool aspectRatioMediaFeatureEval(CSSValue* value, MediaFeaturePrefix op, const MediaValues& mediaValues)
{
    if (value)
        return compareAspectRatioValue(value, mediaValues.viewportWidth(), mediaValues.viewportHeight(), op);

    // ({,min-,max-}aspect-ratio)
    // assume if we have a device, its aspect ratio is non-zero.
    return true;
}

static bool deviceAspectRatioMediaFeatureEval(CSSValue* value, MediaFeaturePrefix op, const MediaValues& mediaValues)
{
    if (value)
        return compareAspectRatioValue(value, mediaValues.deviceWidth(), mediaValues.deviceHeight(), op);

    // ({,min-,max-}device-aspect-ratio)
    // assume if we have a device, its aspect ratio is non-zero.
    return true;
}

static bool evalResolution(CSSValue* value, MediaFeaturePrefix op, const MediaValues& mediaValues)
{
    // According to MQ4, only 'screen', 'print' and 'speech' may match.
    // FIXME: What should speech match? https://www.w3.org/Style/CSS/Tracker/issues/348
    float actualResolution = 0;

    // This checks the actual media type applied to the document, and we know
    // this method only got called if this media type matches the one defined
    // in the query. Thus, if if the document's media type is "print", the
    // media type of the query will either be "print" or "all".
    if (mediaValues.screenMediaType()) {
        actualResolution = clampTo<float>(mediaValues.devicePixelRatio());
    } else if (mediaValues.printMediaType()) {
        // The resolution of images while printing should not depend on the DPI
        // of the screen. Until we support proper ways of querying this info
        // we use 300px which is considered minimum for current printers.
        actualResolution = 300 / cssPixelsPerInch;
    }

    if (!value)
        return !!actualResolution;

    if (!value->isPrimitiveValue())
        return false;

    CSSPrimitiveValue* resolution = toCSSPrimitiveValue(value);

    if (resolution->isNumber())
        return compareValue(actualResolution, resolution->getFloatValue(), op);

    if (!resolution->isResolution())
        return false;

    if (resolution->isDotsPerCentimeter()) {
        // To match DPCM to DPPX values, we limit to 2 decimal points.
        // The http://dev.w3.org/csswg/css3-values/#absolute-lengths recommends
        // "that the pixel unit refer to the whole number of device pixels that best
        // approximates the reference pixel". With that in mind, allowing 2 decimal
        // point precision seems appropriate.
        return compareValue(
            floorf(0.5 + 100 * actualResolution) / 100,
            floorf(0.5 + 100 * resolution->getFloatValue(CSSPrimitiveValue::CSS_DPPX)) / 100, op);
    }

    return compareValue(actualResolution, resolution->getFloatValue(CSSPrimitiveValue::CSS_DPPX), op);
}

static bool devicePixelRatioMediaFeatureEval(CSSValue* value, MediaFeaturePrefix op, const MediaValues& mediaValues)
{
    UseCounter::count(mediaValues.document(), UseCounter::PrefixedDevicePixelRatioMediaFeature);

    return (!value || toCSSPrimitiveValue(value)->isNumber()) && evalResolution(value, op, mediaValues);
}

static bool resolutionMediaFeatureEval(CSSValue* value, MediaFeaturePrefix op, const MediaValues& MediaValues)
{
    return (!value || toCSSPrimitiveValue(value)->isResolution()) && evalResolution(value, op, MediaValues);
}

static bool gridMediaFeatureEval(CSSValue* value, MediaFeaturePrefix op, const MediaValues&)
{
    // if output device is bitmap, grid: 0 == true
    // assume we have bitmap device
    float number;
    if (value && numberValue(value, number))
        return compareValue(static_cast<int>(number), 0, op);
    return false;
}

static bool computeLengthWithoutStyle(CSSPrimitiveValue* primitiveValue, int defaultFontSize, int& result)
{
    // We're running in a background thread, so RenderStyle is not available.
    // Nevertheless, we can evaluate length MQs with em, rem or px units.
    // FIXME: Learn to support more units here, or teach CSSPrimitiveValue about MediaValues.
    unsigned short type = primitiveValue->primitiveType();
    int factor = 0;
    if (type == CSSPrimitiveValue::CSS_EMS || type == CSSPrimitiveValue::CSS_REMS) {
        if (defaultFontSize > 0)
            factor = defaultFontSize;
        else
            return false;
    } else if (type == CSSPrimitiveValue::CSS_PX) {
        factor = 1;
    } else {
        return false;
    }
    result = roundForImpreciseConversion<int>(primitiveValue->getDoubleValue()*factor);
    return true;
}

static bool computeLength(CSSValue* value, bool strict, RenderStyle* initialStyle, int defaultFontSize, int& result)
{
    if (!value->isPrimitiveValue())
        return false;

    CSSPrimitiveValue* primitiveValue = toCSSPrimitiveValue(value);

    if (primitiveValue->isNumber()) {
        result = primitiveValue->getIntValue();
        return !strict || !result;
    }

    if (primitiveValue->isLength()) {
        if (initialStyle) {
            // Relative (like EM) and root relative (like REM) units are always resolved against
            // the initial values for media queries, hence the two initialStyle parameters.
            // FIXME: We need to plumb viewport unit support down to here.
            result = primitiveValue->computeLength<int>(CSSToLengthConversionData(initialStyle, initialStyle, 0, 1.0 /* zoom */, true /* computingFontSize */));
        } else {
            return computeLengthWithoutStyle(primitiveValue, defaultFontSize, result);
        }
        return true;
    }

    return false;
}

static bool deviceHeightMediaFeatureEval(CSSValue* value, MediaFeaturePrefix op, const MediaValues& mediaValues)
{
    if (value) {
        int length;
        return computeLength(value, mediaValues.strictMode(), mediaValues.style(), mediaValues.defaultFontSize(), length)
            && compareValue(static_cast<int>(mediaValues.deviceHeight()), length, op);
    }
    // ({,min-,max-}device-height)
    // assume if we have a device, assume non-zero
    return true;
}

static bool deviceWidthMediaFeatureEval(CSSValue* value, MediaFeaturePrefix op, const MediaValues& mediaValues)
{
    if (value) {
        int length;
        return computeLength(value, mediaValues.strictMode(), mediaValues.style(), mediaValues.defaultFontSize(), length)
            && compareValue(static_cast<int>(mediaValues.deviceWidth()), length, op);
    }
    // ({,min-,max-}device-width)
    // assume if we have a device, assume non-zero
    return true;
}

static bool heightMediaFeatureEval(CSSValue* value, MediaFeaturePrefix op, const MediaValues& mediaValues)
{
    int height = mediaValues.viewportHeight();
    if (value) {
        int length;
        return computeLength(value, mediaValues.strictMode(), mediaValues.style(), mediaValues.defaultFontSize(), length)
            && compareValue(height, length, op);
    }

    return height;
}

static bool widthMediaFeatureEval(CSSValue* value, MediaFeaturePrefix op, const MediaValues& mediaValues)
{
    int width = mediaValues.viewportWidth();
    if (value) {
        int length;
        return computeLength(value, mediaValues.strictMode(), mediaValues.style(), mediaValues.defaultFontSize(), length)
            && compareValue(width, length, op);
    }

    return width;
}

// Rest of the functions are trampolines which set the prefix according to the media feature expression used.

static bool minColorMediaFeatureEval(CSSValue* value, MediaFeaturePrefix, const MediaValues& mediaValues)
{
    return colorMediaFeatureEval(value, MinPrefix, mediaValues);
}

static bool maxColorMediaFeatureEval(CSSValue* value, MediaFeaturePrefix, const MediaValues& mediaValues)
{
    return colorMediaFeatureEval(value, MaxPrefix, mediaValues);
}

static bool minColorIndexMediaFeatureEval(CSSValue* value, MediaFeaturePrefix, const MediaValues& mediaValues)
{
    return colorIndexMediaFeatureEval(value, MinPrefix, mediaValues);
}

static bool maxColorIndexMediaFeatureEval(CSSValue* value, MediaFeaturePrefix, const MediaValues& mediaValues)
{
    return colorIndexMediaFeatureEval(value, MaxPrefix, mediaValues);
}

static bool minMonochromeMediaFeatureEval(CSSValue* value, MediaFeaturePrefix, const MediaValues& mediaValues)
{
    return monochromeMediaFeatureEval(value, MinPrefix, mediaValues);
}

static bool maxMonochromeMediaFeatureEval(CSSValue* value, MediaFeaturePrefix, const MediaValues& mediaValues)
{
    return monochromeMediaFeatureEval(value, MaxPrefix, mediaValues);
}

static bool minAspectRatioMediaFeatureEval(CSSValue* value, MediaFeaturePrefix, const MediaValues& mediaValues)
{
    return aspectRatioMediaFeatureEval(value, MinPrefix, mediaValues);
}

static bool maxAspectRatioMediaFeatureEval(CSSValue* value, MediaFeaturePrefix, const MediaValues& mediaValues)
{
    return aspectRatioMediaFeatureEval(value, MaxPrefix, mediaValues);
}

static bool minDeviceAspectRatioMediaFeatureEval(CSSValue* value, MediaFeaturePrefix, const MediaValues& mediaValues)
{
    return deviceAspectRatioMediaFeatureEval(value, MinPrefix, mediaValues);
}

static bool maxDeviceAspectRatioMediaFeatureEval(CSSValue* value, MediaFeaturePrefix, const MediaValues& mediaValues)
{
    return deviceAspectRatioMediaFeatureEval(value, MaxPrefix, mediaValues);
}

static bool minDevicePixelRatioMediaFeatureEval(CSSValue* value, MediaFeaturePrefix, const MediaValues& mediaValues)
{
    UseCounter::count(mediaValues.document(), UseCounter::PrefixedMinDevicePixelRatioMediaFeature);

    return devicePixelRatioMediaFeatureEval(value, MinPrefix, mediaValues);
}

static bool maxDevicePixelRatioMediaFeatureEval(CSSValue* value, MediaFeaturePrefix, const MediaValues& mediaValues)
{
    UseCounter::count(mediaValues.document(), UseCounter::PrefixedMaxDevicePixelRatioMediaFeature);

    return devicePixelRatioMediaFeatureEval(value, MaxPrefix, mediaValues);
}

static bool minHeightMediaFeatureEval(CSSValue* value, MediaFeaturePrefix, const MediaValues& mediaValues)
{
    return heightMediaFeatureEval(value, MinPrefix, mediaValues);
}

static bool maxHeightMediaFeatureEval(CSSValue* value, MediaFeaturePrefix, const MediaValues& mediaValues)
{
    return heightMediaFeatureEval(value, MaxPrefix, mediaValues);
}

static bool minWidthMediaFeatureEval(CSSValue* value, MediaFeaturePrefix, const MediaValues& mediaValues)
{
    return widthMediaFeatureEval(value, MinPrefix, mediaValues);
}

static bool maxWidthMediaFeatureEval(CSSValue* value, MediaFeaturePrefix, const MediaValues& mediaValues)
{
    return widthMediaFeatureEval(value, MaxPrefix, mediaValues);
}

static bool minDeviceHeightMediaFeatureEval(CSSValue* value, MediaFeaturePrefix, const MediaValues& mediaValues)
{
    return deviceHeightMediaFeatureEval(value, MinPrefix, mediaValues);
}

static bool maxDeviceHeightMediaFeatureEval(CSSValue* value, MediaFeaturePrefix, const MediaValues& mediaValues)
{
    return deviceHeightMediaFeatureEval(value, MaxPrefix, mediaValues);
}

static bool minDeviceWidthMediaFeatureEval(CSSValue* value, MediaFeaturePrefix, const MediaValues& mediaValues)
{
    return deviceWidthMediaFeatureEval(value, MinPrefix, mediaValues);
}

static bool maxDeviceWidthMediaFeatureEval(CSSValue* value, MediaFeaturePrefix, const MediaValues& mediaValues)
{
    return deviceWidthMediaFeatureEval(value, MaxPrefix, mediaValues);
}

static bool minResolutionMediaFeatureEval(CSSValue* value, MediaFeaturePrefix, const MediaValues& mediaValues)
{
    return resolutionMediaFeatureEval(value, MinPrefix, mediaValues);
}

static bool maxResolutionMediaFeatureEval(CSSValue* value, MediaFeaturePrefix, const MediaValues& mediaValues)
{
    return resolutionMediaFeatureEval(value, MaxPrefix, mediaValues);
}

static bool animationMediaFeatureEval(CSSValue* value, MediaFeaturePrefix op, const MediaValues& mediaValues)
{
    UseCounter::count(mediaValues.document(), UseCounter::PrefixedAnimationMediaFeature);

    if (value) {
        float number;
        return numberValue(value, number) && compareValue(1, static_cast<int>(number), op);
    }
    return true;
}

static bool transform2dMediaFeatureEval(CSSValue* value, MediaFeaturePrefix op, const MediaValues& mediaValues)
{
    UseCounter::count(mediaValues.document(), UseCounter::PrefixedTransform2dMediaFeature);

    if (value) {
        float number;
        return numberValue(value, number) && compareValue(1, static_cast<int>(number), op);
    }
    return true;
}

static bool transform3dMediaFeatureEval(CSSValue* value, MediaFeaturePrefix op, const MediaValues& mediaValues)
{
    UseCounter::count(mediaValues.document(), UseCounter::PrefixedTransform3dMediaFeature);

    bool returnValueIfNoParameter;
    int have3dRendering;

    bool threeDEnabled = mediaValues.threeDEnabled();

    returnValueIfNoParameter = threeDEnabled;
    have3dRendering = threeDEnabled ? 1 : 0;

    if (value) {
        float number;
        return numberValue(value, number) && compareValue(have3dRendering, static_cast<int>(number), op);
    }
    return returnValueIfNoParameter;
}

static bool viewModeMediaFeatureEval(CSSValue* value, MediaFeaturePrefix, const MediaValues& mediaValues)
{
    UseCounter::count(mediaValues.document(), UseCounter::PrefixedViewModeMediaFeature);

    if (!value)
        return true;

    return toCSSPrimitiveValue(value)->getValueID() == CSSValueWindowed;
}

static bool hoverMediaFeatureEval(CSSValue* value, MediaFeaturePrefix, const MediaValues& mediaValues)
{
    MediaValues::PointerDeviceType pointer = mediaValues.pointer();

    // If we're on a port that hasn't explicitly opted into providing pointer device information
    // (or otherwise can't be confident in the pointer hardware available), then behave exactly
    // as if this feature feature isn't supported.
    if (pointer == MediaValues::UnknownPointer)
        return false;

    float number = 1;
    if (value) {
        if (!numberValue(value, number))
            return false;
    }

    return (pointer == MediaValues::NoPointer && !number)
        || (pointer == MediaValues::TouchPointer && !number)
        || (pointer == MediaValues::MousePointer && number == 1);
}

static bool pointerMediaFeatureEval(CSSValue* value, MediaFeaturePrefix, const MediaValues& mediaValues)
{
    MediaValues::PointerDeviceType pointer = mediaValues.pointer();

    // If we're on a port that hasn't explicitly opted into providing pointer device information
    // (or otherwise can't be confident in the pointer hardware available), then behave exactly
    // as if this feature feature isn't supported.
    if (pointer == MediaValues::UnknownPointer)
        return false;

    if (!value)
        return pointer != MediaValues::NoPointer;

    if (!value->isPrimitiveValue())
        return false;

    const CSSValueID id = toCSSPrimitiveValue(value)->getValueID();
    return (pointer == MediaValues::NoPointer && id == CSSValueNone)
        || (pointer == MediaValues::TouchPointer && id == CSSValueCoarse)
        || (pointer == MediaValues::MousePointer && id == CSSValueFine);
}

static bool scanMediaFeatureEval(CSSValue* value, MediaFeaturePrefix, const MediaValues& mediaValues)
{
    if (!mediaValues.scanMediaType())
        return false;

    if (!value)
        return true;

    if (!value->isPrimitiveValue())
        return false;

    // If a platform interface supplies progressive/interlace info for TVs in the
    // future, it needs to be handled here. For now, assume a modern TV with
    // progressive display.
    return toCSSPrimitiveValue(value)->getValueID() == CSSValueProgressive;
}

static void createFunctionMap()
{
    // Create the table.
    gFunctionMap = new FunctionMap;
#define ADD_TO_FUNCTIONMAP(name)  \
    gFunctionMap->set(name##MediaFeature.impl(), name##MediaFeatureEval);
    CSS_MEDIAQUERY_NAMES_FOR_EACH_MEDIAFEATURE(ADD_TO_FUNCTIONMAP);
#undef ADD_TO_FUNCTIONMAP
}

bool MediaQueryEvaluator::eval(const MediaQueryExp* expr) const
{
    if (!m_mediaValues)
        return m_expectedResult;

    if (!gFunctionMap)
        createFunctionMap();

    // Call the media feature evaluation function. Assume no prefix and let
    // trampoline functions override the prefix if prefix is used.
    EvalFunc func = gFunctionMap->get(expr->mediaFeature().impl());
    if (func)
        return func(expr->value(), NoPrefix, *m_mediaValues);

    return false;
}

} // namespace
