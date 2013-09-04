/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 *           (C) 2006 Allan Sandfeld Jensen (kde@carewolf.com)
 *           (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
 * Copyright (C) 2003, 2004, 2005, 2006, 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
 * Copyright (C) Research In Motion Limited 2011-2012. All rights reserved.
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
#include "core/rendering/RenderImage.h"

#include "HTMLNames.h"
#include "core/editing/FrameSelection.h"
#include "core/fetch/ImageResource.h"
#include "core/html/HTMLAreaElement.h"
#include "core/html/HTMLImageElement.h"
#include "core/html/HTMLInputElement.h"
#include "core/html/HTMLMapElement.h"
#include "core/inspector/InspectorInstrumentation.h"
#include "core/page/Frame.h"
#include "core/page/Page.h"
#include "core/platform/graphics/Font.h"
#include "core/platform/graphics/FontCache.h"
#include "core/platform/graphics/GraphicsContext.h"
#include "core/platform/graphics/GraphicsContextStateSaver.h"
#include "core/rendering/HitTestResult.h"
#include "core/rendering/PaintInfo.h"
#include "core/rendering/RenderView.h"
#include "core/svg/graphics/SVGImage.h"
#include "wtf/UnusedParam.h"

using namespace std;

namespace WebCore {

using namespace HTMLNames;

RenderImage::RenderImage(Element* element)
    : RenderReplaced(element, IntSize())
    , m_needsToSetSizeForAltText(false)
    , m_didIncrementVisuallyNonEmptyPixelCount(false)
    , m_isGeneratedContent(false)
{
    updateAltText();
}

RenderImage* RenderImage::createAnonymous(Document* document)
{
    RenderImage* image = new RenderImage(0);
    image->setDocumentForAnonymous(document);
    return image;
}

RenderImage::~RenderImage()
{
    ASSERT(m_imageResource);
    m_imageResource->shutdown();
}

void RenderImage::setImageResource(PassOwnPtr<RenderImageResource> imageResource)
{
    ASSERT(!m_imageResource);
    m_imageResource = imageResource;
    m_imageResource->initialize(this);
}

// If we'll be displaying either alt text or an image, add some padding.
static const unsigned short paddingWidth = 4;
static const unsigned short paddingHeight = 4;

// Alt text is restricted to this maximum size, in pixels.  These are
// signed integers because they are compared with other signed values.
static const float maxAltTextWidth = 1024;
static const int maxAltTextHeight = 256;

IntSize RenderImage::imageSizeForError(ImageResource* newImage) const
{
    ASSERT_ARG(newImage, newImage);
    ASSERT_ARG(newImage, newImage->imageForRenderer(this));

    IntSize imageSize;
    if (newImage->willPaintBrokenImage()) {
        float deviceScaleFactor = WebCore::deviceScaleFactor(frame());
        pair<Image*, float> brokenImageAndImageScaleFactor = ImageResource::brokenImage(deviceScaleFactor);
        imageSize = brokenImageAndImageScaleFactor.first->size();
        imageSize.scale(1 / brokenImageAndImageScaleFactor.second);
    } else
        imageSize = newImage->imageForRenderer(this)->size();

    // imageSize() returns 0 for the error image. We need the true size of the
    // error image, so we have to get it by grabbing image() directly.
    return IntSize(paddingWidth + imageSize.width() * style()->effectiveZoom(), paddingHeight + imageSize.height() * style()->effectiveZoom());
}

// Sets the image height and width to fit the alt text.  Returns true if the
// image size changed.
bool RenderImage::setImageSizeForAltText(ImageResource* newImage /* = 0 */)
{
    IntSize imageSize;
    if (newImage && newImage->imageForRenderer(this))
        imageSize = imageSizeForError(newImage);
    else if (!m_altText.isEmpty() || newImage) {
        // If we'll be displaying either text or an image, add a little padding.
        imageSize = IntSize(paddingWidth, paddingHeight);
    }

    // we have an alt and the user meant it (its not a text we invented)
    if (!m_altText.isEmpty()) {
        FontCachePurgePreventer fontCachePurgePreventer;

        const Font& font = style()->font();
        IntSize paddedTextSize(paddingWidth + min(ceilf(font.width(RenderBlock::constructTextRun(this, font, m_altText, style()))), maxAltTextWidth), paddingHeight + min(font.fontMetrics().height(), maxAltTextHeight));
        imageSize = imageSize.expandedTo(paddedTextSize);
    }

    if (imageSize == intrinsicSize())
        return false;

    setIntrinsicSize(imageSize);
    return true;
}

void RenderImage::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    RenderReplaced::styleDidChange(diff, oldStyle);
    if (m_needsToSetSizeForAltText) {
        if (!m_altText.isEmpty() && setImageSizeForAltText(m_imageResource->cachedImage()))
            imageDimensionsChanged(true /* imageSizeChanged */);
        m_needsToSetSizeForAltText = false;
    }
}

void RenderImage::imageChanged(WrappedImagePtr newImage, const IntRect* rect)
{
    if (documentBeingDestroyed())
        return;

    if (hasBoxDecorations() || hasMask())
        RenderReplaced::imageChanged(newImage, rect);

    if (!m_imageResource)
        return;

    if (newImage != m_imageResource->imagePtr())
        return;

    if (!m_didIncrementVisuallyNonEmptyPixelCount) {
        // At a zoom level of 1 the image is guaranteed to have an integer size.
        view()->frameView()->incrementVisuallyNonEmptyPixelCount(flooredIntSize(m_imageResource->imageSize(1.0f)));
        m_didIncrementVisuallyNonEmptyPixelCount = true;
    }

    bool imageSizeChanged = false;

    // Set image dimensions, taking into account the size of the alt text.
    if (m_imageResource->errorOccurred() || !newImage) {
        if (!m_altText.isEmpty() && document().hasPendingStyleRecalc()) {
            ASSERT(node());
            if (node()) {
                m_needsToSetSizeForAltText = true;
                node()->setNeedsStyleRecalc(LocalStyleChange, StyleChangeFromRenderer);
            }
            return;
        }
        imageSizeChanged = setImageSizeForAltText(m_imageResource->cachedImage());
    }

    imageDimensionsChanged(imageSizeChanged, rect);
}

bool RenderImage::updateIntrinsicSizeIfNeeded(const LayoutSize& newSize, bool imageSizeChanged)
{
    if (newSize == intrinsicSize() && !imageSizeChanged)
        return false;
    if (m_imageResource->errorOccurred() || !m_imageResource->hasImage())
        return imageSizeChanged;
    setIntrinsicSize(newSize);
    return true;
}

void RenderImage::updateInnerContentRect()
{
    // Propagate container size to image resource.
    LayoutRect paintRect = replacedContentRect();
    IntSize containerSize(paintRect.width(), paintRect.height());
    if (!containerSize.isEmpty())
        m_imageResource->setContainerSizeForRenderer(containerSize);
}

void RenderImage::imageDimensionsChanged(bool imageSizeChanged, const IntRect* rect)
{
    bool intrinsicSizeChanged = updateIntrinsicSizeIfNeeded(m_imageResource->intrinsicSize(style()->effectiveZoom()), imageSizeChanged);

    // In the case of generated image content using :before/:after/content, we might not be
    // in the render tree yet. In that case, we just need to update our intrinsic size.
    // layout() will be called after we are inserted in the tree which will take care of
    // what we are doing here.
    if (!containingBlock())
        return;

    bool shouldRepaint = true;
    if (intrinsicSizeChanged) {
        if (!preferredLogicalWidthsDirty())
            setPreferredLogicalWidthsDirty();

        bool hasOverrideSize = hasOverrideHeight() || hasOverrideWidth();
        if (!hasOverrideSize && !imageSizeChanged) {
            LogicalExtentComputedValues computedValues;
            computeLogicalWidthInRegion(computedValues);
            LayoutUnit newWidth = computedValues.m_extent;
            computeLogicalHeight(height(), 0, computedValues);
            LayoutUnit newHeight = computedValues.m_extent;

            imageSizeChanged = width() != newWidth || height() != newHeight;
        }

        // FIXME: We only need to recompute the containing block's preferred size
        // if the containing block's size depends on the image's size (i.e., the container uses shrink-to-fit sizing).
        // There's no easy way to detect that shrink-to-fit is needed, always force a layout.
        bool containingBlockNeedsToRecomputePreferredSize =
            style()->logicalWidth().isPercent()
            || style()->logicalMaxWidth().isPercent()
            || style()->logicalMinWidth().isPercent();

        if (imageSizeChanged || hasOverrideSize || containingBlockNeedsToRecomputePreferredSize) {
            shouldRepaint = false;
            if (!selfNeedsLayout())
                setNeedsLayout();
        }

        if (everHadLayout() && !selfNeedsLayout()) {
            // The inner content rectangle is calculated during layout, but may need an update now
            // (unless the box has already been scheduled for layout). In order to calculate it, we
            // may need values from the containing block, though, so make sure that we're not too
            // early. It may be that layout hasn't even taken place once yet.
            updateInnerContentRect();
        }
    }

    if (shouldRepaint) {
        LayoutRect repaintRect;
        if (rect) {
            // The image changed rect is in source image coordinates (pre-zooming),
            // so map from the bounds of the image to the contentsBox.
            repaintRect = enclosingIntRect(mapRect(*rect, FloatRect(FloatPoint(), m_imageResource->imageSize(1.0f)), contentBoxRect()));
            // Guard against too-large changed rects.
            repaintRect.intersect(contentBoxRect());
        } else
            repaintRect = contentBoxRect();

        repaintRectangle(repaintRect);

        // Tell any potential compositing layers that the image needs updating.
        contentChanged(ImageChanged);
    }
}

void RenderImage::notifyFinished(Resource* newImage)
{
    if (!m_imageResource)
        return;

    if (documentBeingDestroyed())
        return;

    invalidateBackgroundObscurationStatus();

    if (newImage == m_imageResource->cachedImage()) {
        // tell any potential compositing layers
        // that the image is done and they can reference it directly.
        contentChanged(ImageChanged);
    }
}

void RenderImage::paintReplaced(PaintInfo& paintInfo, const LayoutPoint& paintOffset)
{
    LayoutUnit cWidth = contentWidth();
    LayoutUnit cHeight = contentHeight();
    LayoutUnit leftBorder = borderLeft();
    LayoutUnit topBorder = borderTop();
    LayoutUnit leftPad = paddingLeft();
    LayoutUnit topPad = paddingTop();

    GraphicsContext* context = paintInfo.context;

    Page* page = 0;
    if (Frame* frame = this->frame())
        page = frame->page();

    if (!m_imageResource->hasImage() || m_imageResource->errorOccurred()) {
        if (paintInfo.phase == PaintPhaseSelection)
            return;

        if (page && paintInfo.phase == PaintPhaseForeground)
            page->addRelevantUnpaintedObject(this, visualOverflowRect());

        if (cWidth > 2 && cHeight > 2) {
            const int borderWidth = 1;

            // Draw an outline rect where the image should be.
            context->setStrokeStyle(SolidStroke);
            context->setStrokeColor(Color::lightGray);
            context->setFillColor(Color::transparent);
            context->drawRect(pixelSnappedIntRect(LayoutRect(paintOffset.x() + leftBorder + leftPad, paintOffset.y() + topBorder + topPad, cWidth, cHeight)));

            bool errorPictureDrawn = false;
            LayoutSize imageOffset;
            // When calculating the usable dimensions, exclude the pixels of
            // the ouline rect so the error image/alt text doesn't draw on it.
            LayoutUnit usableWidth = cWidth - 2 * borderWidth;
            LayoutUnit usableHeight = cHeight - 2 * borderWidth;

            RefPtr<Image> image = m_imageResource->image();

            if (m_imageResource->errorOccurred() && !image->isNull() && usableWidth >= image->width() && usableHeight >= image->height()) {
                float deviceScaleFactor = WebCore::deviceScaleFactor(frame());
                // Call brokenImage() explicitly to ensure we get the broken image icon at the appropriate resolution.
                pair<Image*, float> brokenImageAndImageScaleFactor = ImageResource::brokenImage(deviceScaleFactor);
                image = brokenImageAndImageScaleFactor.first;
                IntSize imageSize = image->size();
                imageSize.scale(1 / brokenImageAndImageScaleFactor.second);
                // Center the error image, accounting for border and padding.
                LayoutUnit centerX = (usableWidth - imageSize.width()) / 2;
                if (centerX < 0)
                    centerX = 0;
                LayoutUnit centerY = (usableHeight - imageSize.height()) / 2;
                if (centerY < 0)
                    centerY = 0;
                imageOffset = LayoutSize(leftBorder + leftPad + centerX + borderWidth, topBorder + topPad + centerY + borderWidth);
                context->drawImage(image.get(), pixelSnappedIntRect(LayoutRect(paintOffset + imageOffset, imageSize)), CompositeSourceOver, shouldRespectImageOrientation());
                errorPictureDrawn = true;
            }

            if (!m_altText.isEmpty()) {
                String text = document().displayStringModifiedByEncoding(m_altText);
                const Font& font = style()->font();
                const FontMetrics& fontMetrics = font.fontMetrics();
                LayoutUnit ascent = fontMetrics.ascent();
                LayoutPoint textRectOrigin = paintOffset;
                textRectOrigin.move(leftBorder + leftPad + (paddingWidth / 2) - borderWidth, topBorder + topPad + (paddingHeight / 2) - borderWidth);
                LayoutPoint textOrigin(textRectOrigin.x(), textRectOrigin.y() + ascent);

                // Only draw the alt text if it'll fit within the content box,
                // and only if it fits above the error image.
                TextRun textRun = RenderBlock::constructTextRun(this, font, text, style());
                LayoutUnit textWidth = font.width(textRun);
                TextRunPaintInfo textRunPaintInfo(textRun);
                textRunPaintInfo.bounds = FloatRect(textRectOrigin, FloatSize(textWidth, fontMetrics.height()));
                context->setFillColor(resolveColor(CSSPropertyColor));
                if (errorPictureDrawn) {
                    if (usableWidth >= textWidth && fontMetrics.height() <= imageOffset.height())
                        context->drawText(font, textRunPaintInfo, textOrigin);
                } else if (usableWidth >= textWidth && usableHeight >= fontMetrics.height())
                    context->drawText(font, textRunPaintInfo, textOrigin);
            }
        }
    } else if (m_imageResource->hasImage() && cWidth > 0 && cHeight > 0) {
        RefPtr<Image> img = m_imageResource->image(cWidth, cHeight);
        if (!img || img->isNull()) {
            if (page && paintInfo.phase == PaintPhaseForeground)
                page->addRelevantUnpaintedObject(this, visualOverflowRect());
            return;
        }

        LayoutRect contentRect = contentBoxRect();
        contentRect.moveBy(paintOffset);
        LayoutRect paintRect = replacedContentRect();
        paintRect.moveBy(paintOffset);
        bool clip = !contentRect.contains(paintRect);
        if (clip) {
            context->save();
            context->clip(contentRect);
        }

        paintIntoRect(context, paintRect);

        if (cachedImage() && page && paintInfo.phase == PaintPhaseForeground) {
            // For now, count images as unpainted if they are still progressively loading. We may want
            // to refine this in the future to account for the portion of the image that has painted.
            LayoutRect visibleRect = intersection(paintRect, contentRect);
            if (cachedImage()->isLoading())
                page->addRelevantUnpaintedObject(this, visibleRect);
            else
                page->addRelevantRepaintedObject(this, visibleRect);
        }

        if (clip)
            context->restore();
    }
}

void RenderImage::paint(PaintInfo& paintInfo, const LayoutPoint& paintOffset)
{
    RenderReplaced::paint(paintInfo, paintOffset);

    if (paintInfo.phase == PaintPhaseOutline)
        paintAreaElementFocusRing(paintInfo);
}

void RenderImage::paintAreaElementFocusRing(PaintInfo& paintInfo)
{
    Document& document = this->document();

    if (document.printing() || !document.frame()->selection().isFocusedAndActive())
        return;

    if (paintInfo.context->paintingDisabled() && !paintInfo.context->updatingControlTints())
        return;

    Element* focusedElement = document.focusedElement();
    if (!focusedElement || !isHTMLAreaElement(focusedElement))
        return;

    HTMLAreaElement* areaElement = toHTMLAreaElement(focusedElement);
    if (areaElement->imageElement() != node())
        return;

    // Even if the theme handles focus ring drawing for entire elements, it won't do it for
    // an area within an image, so we don't call RenderTheme::supportsFocusRing here.

    Path path = areaElement->computePath(this);
    if (path.isEmpty())
        return;

    RenderStyle* areaElementStyle = areaElement->computedStyle();
    unsigned short outlineWidth = areaElementStyle->outlineWidth();
    if (!outlineWidth)
        return;

    // FIXME: Clip path instead of context when Skia pathops is ready.
    // https://crbug.com/251206
    GraphicsContextStateSaver savedContext(*paintInfo.context);
    paintInfo.context->clip(absoluteContentBox());
    paintInfo.context->drawFocusRing(path, outlineWidth,
        areaElementStyle->outlineOffset(),
        resolveColor(areaElementStyle, CSSPropertyOutlineColor));
}

void RenderImage::areaElementFocusChanged(HTMLAreaElement* areaElement)
{
    ASSERT(areaElement->imageElement() == node());

    Path path = areaElement->computePath(this);
    if (path.isEmpty())
        return;

    RenderStyle* areaElementStyle = areaElement->computedStyle();
    unsigned short outlineWidth = areaElementStyle->outlineWidth();

    IntRect repaintRect = enclosingIntRect(path.boundingRect());
    repaintRect.moveBy(-absoluteContentBox().location());
    repaintRect.inflate(outlineWidth);

    repaintRectangle(repaintRect);
}

void RenderImage::paintIntoRect(GraphicsContext* context, const LayoutRect& rect)
{
    IntRect alignedRect = pixelSnappedIntRect(rect);
    if (!m_imageResource->hasImage() || m_imageResource->errorOccurred() || alignedRect.width() <= 0 || alignedRect.height() <= 0)
        return;

    RefPtr<Image> img = m_imageResource->image(alignedRect.width(), alignedRect.height());
    if (!img || img->isNull())
        return;

    HTMLImageElement* imageElt = (node() && node()->hasTagName(imgTag)) ? toHTMLImageElement(node()) : 0;
    CompositeOperator compositeOperator = imageElt ? imageElt->compositeOperator() : CompositeSourceOver;
    Image* image = m_imageResource->image().get();
    bool useLowQualityScaling = shouldPaintAtLowQuality(context, image, image, alignedRect.size());

    InspectorInstrumentation::willPaintImage(this);
    context->drawImage(m_imageResource->image(alignedRect.width(), alignedRect.height()).get(), alignedRect, compositeOperator, shouldRespectImageOrientation(), useLowQualityScaling);
    InspectorInstrumentation::didPaintImage(this);
}

bool RenderImage::boxShadowShouldBeAppliedToBackground(BackgroundBleedAvoidance bleedAvoidance, InlineFlowBox*) const
{
    if (!RenderBoxModelObject::boxShadowShouldBeAppliedToBackground(bleedAvoidance))
        return false;

    return !const_cast<RenderImage*>(this)->backgroundIsKnownToBeObscured();
}

bool RenderImage::foregroundIsKnownToBeOpaqueInRect(const LayoutRect& localRect, unsigned maxDepthToTest) const
{
    UNUSED_PARAM(maxDepthToTest);
    if (!m_imageResource->hasImage() || m_imageResource->errorOccurred())
        return false;
    if (m_imageResource->cachedImage() && !m_imageResource->cachedImage()->isLoaded())
        return false;
    if (!contentBoxRect().contains(localRect))
        return false;
    EFillBox backgroundClip = style()->backgroundClip();
    // Background paints under borders.
    if (backgroundClip == BorderFillBox && style()->hasBorder() && !borderObscuresBackground())
        return false;
    // Background shows in padding area.
    if ((backgroundClip == BorderFillBox || backgroundClip == PaddingFillBox) && style()->hasPadding())
        return false;
    // Object-fit may leave parts of the content box empty.
    ObjectFit objectFit = style()->objectFit();
    if (objectFit != ObjectFitFill && objectFit != ObjectFitCover)
        return false;
    // Check for image with alpha.
    return m_imageResource->cachedImage() && m_imageResource->cachedImage()->currentFrameKnownToBeOpaque(this);
}

bool RenderImage::computeBackgroundIsKnownToBeObscured()
{
    if (!hasBackground())
        return false;
    return foregroundIsKnownToBeOpaqueInRect(backgroundPaintedExtent(), 0);
}

LayoutUnit RenderImage::minimumReplacedHeight() const
{
    return m_imageResource->errorOccurred() ? intrinsicSize().height() : LayoutUnit();
}

HTMLMapElement* RenderImage::imageMap() const
{
    HTMLImageElement* i = node() && node()->hasTagName(imgTag) ? toHTMLImageElement(node()) : 0;
    return i ? i->treeScope().getImageMap(i->fastGetAttribute(usemapAttr)) : 0;
}

bool RenderImage::nodeAtPoint(const HitTestRequest& request, HitTestResult& result, const HitTestLocation& locationInContainer, const LayoutPoint& accumulatedOffset, HitTestAction hitTestAction)
{
    HitTestResult tempResult(result.hitTestLocation());
    bool inside = RenderReplaced::nodeAtPoint(request, tempResult, locationInContainer, accumulatedOffset, hitTestAction);

    if (tempResult.innerNode() && node()) {
        if (HTMLMapElement* map = imageMap()) {
            LayoutRect contentBox = contentBoxRect();
            float scaleFactor = 1 / style()->effectiveZoom();
            LayoutPoint mapLocation = locationInContainer.point() - toLayoutSize(accumulatedOffset) - locationOffset() - toLayoutSize(contentBox.location());
            mapLocation.scale(scaleFactor, scaleFactor);

            if (map->mapMouseEvent(mapLocation, contentBox.size(), tempResult))
                tempResult.setInnerNonSharedNode(node());
        }
    }

    if (!inside && result.isRectBasedTest())
        result.append(tempResult);
    if (inside)
        result = tempResult;
    return inside;
}

void RenderImage::updateAltText()
{
    if (!node())
        return;

    if (node()->hasTagName(inputTag))
        m_altText = toHTMLInputElement(node())->altText();
    else if (node()->hasTagName(imgTag))
        m_altText = toHTMLImageElement(node())->altText();
}

void RenderImage::layout()
{
    RenderReplaced::layout();
    updateInnerContentRect();
}

void RenderImage::computeIntrinsicRatioInformation(FloatSize& intrinsicSize, double& intrinsicRatio, bool& isPercentageIntrinsicSize) const
{
    RenderReplaced::computeIntrinsicRatioInformation(intrinsicSize, intrinsicRatio, isPercentageIntrinsicSize);

    // Our intrinsicSize is empty if we're rendering generated images with relative width/height. Figure out the right intrinsic size to use.
    if (intrinsicSize.isEmpty() && (m_imageResource->imageHasRelativeWidth() || m_imageResource->imageHasRelativeHeight())) {
        RenderObject* containingBlock = isOutOfFlowPositioned() ? container() : this->containingBlock();
        if (containingBlock->isBox()) {
            RenderBox* box = toRenderBox(containingBlock);
            intrinsicSize.setWidth(box->availableLogicalWidth());
            intrinsicSize.setHeight(box->availableLogicalHeight(IncludeMarginBorderPadding));
        }
    }
    // Don't compute an intrinsic ratio to preserve historical WebKit behavior if we're painting alt text and/or a broken image.
    if (m_imageResource && m_imageResource->errorOccurred()) {
        intrinsicRatio = 1;
        return;
    }
}

bool RenderImage::needsPreferredWidthsRecalculation() const
{
    if (RenderReplaced::needsPreferredWidthsRecalculation())
        return true;
    return embeddedContentBox();
}

RenderBox* RenderImage::embeddedContentBox() const
{
    if (!m_imageResource)
        return 0;

    ImageResource* cachedImage = m_imageResource->cachedImage();
    if (cachedImage && cachedImage->image() && cachedImage->image()->isSVGImage())
        return static_cast<SVGImage*>(cachedImage->image())->embeddedContentBox();

    return 0;
}

} // namespace WebCore
