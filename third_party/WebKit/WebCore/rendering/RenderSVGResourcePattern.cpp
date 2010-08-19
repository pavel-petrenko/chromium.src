/*
 * Copyright (C) 2006 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"

#if ENABLE(SVG)
#include "RenderSVGResourcePattern.h"

#include "FrameView.h"
#include "GraphicsContext.h"
#include "PatternAttributes.h"
#include "RenderSVGRoot.h"
#include "SVGImageBufferTools.h"
#include "SVGRenderSupport.h"

namespace WebCore {

RenderSVGResourceType RenderSVGResourcePattern::s_resourceType = PatternResourceType;

RenderSVGResourcePattern::RenderSVGResourcePattern(SVGPatternElement* node)
    : RenderSVGResourceContainer(node)
{
}

RenderSVGResourcePattern::~RenderSVGResourcePattern()
{
    if (m_pattern.isEmpty())
        return;

    deleteAllValues(m_pattern);
    m_pattern.clear();
}

void RenderSVGResourcePattern::removeAllClientsFromCache(bool markForInvalidation)
{
    if (!m_pattern.isEmpty()) {
        deleteAllValues(m_pattern);
        m_pattern.clear();
    }

    markAllClientsForInvalidation(markForInvalidation ? RepaintInvalidation : ParentOnlyInvalidation);
}

void RenderSVGResourcePattern::removeClientFromCache(RenderObject* client, bool markForInvalidation)
{
    ASSERT(client);

    if (m_pattern.contains(client))
        delete m_pattern.take(client);

    markClientForInvalidation(client, markForInvalidation ? RepaintInvalidation : ParentOnlyInvalidation);
}

bool RenderSVGResourcePattern::applyResource(RenderObject* object, RenderStyle* style, GraphicsContext*& context, unsigned short resourceMode)
{
    ASSERT(object);
    ASSERT(style);
    ASSERT(context);
    ASSERT(resourceMode != ApplyToDefaultMode);

    // Be sure to synchronize all SVG properties on the patternElement _before_ processing any further.
    // Otherwhise the call to collectPatternAttributes() below, may cause the SVG DOM property
    // synchronization to kick in, which causes removeAllClientsFromCache() to be called, which in turn deletes our
    // PatternData object! Leaving out the line below will cause svg/dynamic-updates/SVGPatternElement-svgdom* to crash.
    SVGPatternElement* patternElement = static_cast<SVGPatternElement*>(node());
    if (!patternElement)
        return false;

    patternElement->updateAnimatedSVGAttribute(anyQName());

    if (!m_pattern.contains(object))
        m_pattern.set(object, new PatternData);

    PatternData* patternData = m_pattern.get(object);
    if (!patternData->pattern) {
        PatternAttributes attributes = patternElement->collectPatternProperties();

        // If we couldn't determine the pattern content element root, stop here.
        if (!attributes.patternContentElement())
            return false;

        // Compute all necessary transformations to build the tile image & the pattern.
        FloatRect tileBoundaries;
        AffineTransform tileImageTransform = buildTileImageTransform(object, attributes, patternElement, tileBoundaries);

        AffineTransform absoluteTransform(SVGImageBufferTools::transformationToOutermostSVGCoordinateSystem(object));
        FloatRect absoluteTileBoundaries = absoluteTransform.mapRect(tileBoundaries);

        // Build tile image.
        OwnPtr<ImageBuffer> tileImage = createTileImage(object, attributes, tileBoundaries, absoluteTileBoundaries, tileImageTransform);
        if (!tileImage)
            return false;

        RefPtr<Image> copiedImage = tileImage->copyImage();
        if (!copiedImage)
            return false;

        // Build pattern.
        patternData->pattern = Pattern::create(copiedImage, true, true);
        if (!patternData->pattern)
            return false;

        // Compute pattern space transformation.
        patternData->transform.translate(tileBoundaries.x(), tileBoundaries.y());
        patternData->transform.scale(tileBoundaries.width() / absoluteTileBoundaries.width(), tileBoundaries.height() / absoluteTileBoundaries.height());

        AffineTransform patternTransform = attributes.patternTransform();
        if (!patternTransform.isIdentity())
            patternData->transform.multiply(patternTransform);

        patternData->pattern->setPatternSpaceTransform(patternData->transform);
    }

    // Draw pattern
    context->save();

    const SVGRenderStyle* svgStyle = style->svgStyle();
    ASSERT(svgStyle);

    if (resourceMode & ApplyToFillMode) {
        context->setAlpha(svgStyle->fillOpacity());
        context->setFillPattern(patternData->pattern);
        context->setFillRule(svgStyle->fillRule());
    } else if (resourceMode & ApplyToStrokeMode) {
        if (svgStyle->vectorEffect() == VE_NON_SCALING_STROKE)
            patternData->pattern->setPatternSpaceTransform(transformOnNonScalingStroke(object, patternData->transform));
        context->setAlpha(svgStyle->strokeOpacity());
        context->setStrokePattern(patternData->pattern);
        SVGRenderSupport::applyStrokeStyleToContext(context, style, object);
    }

    if (resourceMode & ApplyToTextMode) {
        if (resourceMode & ApplyToFillMode) {
            context->setTextDrawingMode(cTextFill);

#if PLATFORM(CG)
            context->applyFillPattern();
#endif
        } else if (resourceMode & ApplyToStrokeMode) {
            context->setTextDrawingMode(cTextStroke);

#if PLATFORM(CG)
            context->applyStrokePattern();
#endif
        }
    }

    return true;
}

void RenderSVGResourcePattern::postApplyResource(RenderObject*, GraphicsContext*& context, unsigned short resourceMode)
{
    ASSERT(context);
    ASSERT(resourceMode != ApplyToDefaultMode);

    if (!(resourceMode & ApplyToTextMode)) {
        if (resourceMode & ApplyToFillMode)
            context->fillPath();
        else if (resourceMode & ApplyToStrokeMode)
            context->strokePath();
    }

    context->restore();
}

static inline FloatRect calculatePatternBoundaries(const PatternAttributes& attributes,
                                                   const FloatRect& objectBoundingBox,
                                                   const SVGPatternElement* patternElement)
{
    ASSERT(patternElement);

    if (attributes.boundingBoxMode())
        return FloatRect(attributes.x().valueAsPercentage() * objectBoundingBox.width() + objectBoundingBox.x(),
                         attributes.y().valueAsPercentage() * objectBoundingBox.height() + objectBoundingBox.y(),
                         attributes.width().valueAsPercentage() * objectBoundingBox.width(),
                         attributes.height().valueAsPercentage() * objectBoundingBox.height());

    return FloatRect(attributes.x().value(patternElement),
                     attributes.y().value(patternElement),
                     attributes.width().value(patternElement),
                     attributes.height().value(patternElement));
}

AffineTransform RenderSVGResourcePattern::buildTileImageTransform(RenderObject* renderer,
                                                                  const PatternAttributes& attributes,
                                                                  const SVGPatternElement* patternElement,
                                                                  FloatRect& patternBoundaries) const
{
    ASSERT(renderer);
    ASSERT(patternElement);

    FloatRect objectBoundingBox = renderer->objectBoundingBox();    
    patternBoundaries = calculatePatternBoundaries(attributes, objectBoundingBox, patternElement); 

    AffineTransform viewBoxCTM = patternElement->viewBoxToViewTransform(patternElement->viewBox(), patternElement->preserveAspectRatio(), patternBoundaries.width(), patternBoundaries.height());
    AffineTransform tileImageTransform;

    // Apply viewBox/objectBoundingBox transformations.
    if (!viewBoxCTM.isIdentity())
        tileImageTransform = viewBoxCTM;
    else if (attributes.boundingBoxModeContent()) {
        tileImageTransform.translate(objectBoundingBox.x(), objectBoundingBox.y());
        tileImageTransform.scale(objectBoundingBox.width(), objectBoundingBox.height());
    }

    return tileImageTransform;
}

PassOwnPtr<ImageBuffer> RenderSVGResourcePattern::createTileImage(RenderObject* object,
                                                                  const PatternAttributes& attributes,
                                                                  const FloatRect& tileBoundaries,
                                                                  const FloatRect& absoluteTileBoundaries,
                                                                  const AffineTransform& tileImageTransform) const
{
    ASSERT(object);

    // Clamp tile image size against SVG viewport size, as last resort, to avoid allocating huge image buffers.
    FloatRect contentBoxRect = SVGRenderSupport::findTreeRootObject(object)->contentBoxRect();

    FloatRect clampedAbsoluteTileBoundaries = absoluteTileBoundaries;
    if (clampedAbsoluteTileBoundaries.width() > contentBoxRect.width())
        clampedAbsoluteTileBoundaries.setWidth(contentBoxRect.width());

    if (clampedAbsoluteTileBoundaries.height() > contentBoxRect.height())
        clampedAbsoluteTileBoundaries.setHeight(contentBoxRect.height());

    OwnPtr<ImageBuffer> tileImage;

    if (!SVGImageBufferTools::createImageBuffer(clampedAbsoluteTileBoundaries, tileImage, DeviceRGB))
        return PassOwnPtr<ImageBuffer>();

    GraphicsContext* tileImageContext = tileImage->context();
    ASSERT(tileImageContext);

    // Compensate rounding effects, as the absolute target rect is using floating-point numbers and the image buffer size is integer.
    IntSize unclampedImageSize(SVGImageBufferTools::roundedImageBufferSize(absoluteTileBoundaries.size()));
    tileImageContext->scale(FloatSize(unclampedImageSize.width() / absoluteTileBoundaries.width(),
                                      unclampedImageSize.height() / absoluteTileBoundaries.height()));

    // The image buffer represents the final rendered size, so the content has to be scaled (to avoid pixelation).
    tileImageContext->scale(FloatSize(absoluteTileBoundaries.width() / tileBoundaries.width(),
                                      absoluteTileBoundaries.height() / tileBoundaries.height()));

    // Apply tile image transformations.
    if (!tileImageTransform.isIdentity())
        tileImageContext->concatCTM(tileImageTransform);

    // Draw the content into the ImageBuffer.
    for (Node* node = attributes.patternContentElement()->firstChild(); node; node = node->nextSibling()) {
        if (!node->isSVGElement() || !static_cast<SVGElement*>(node)->isStyled() || !node->renderer())
            continue;
        SVGRenderSupport::renderSubtreeToImage(tileImage.get(), node->renderer());
    }

    return tileImage.release();
}

}

#endif
