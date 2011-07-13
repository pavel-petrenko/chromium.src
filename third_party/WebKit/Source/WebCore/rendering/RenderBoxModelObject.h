/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2006, 2007, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
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

#ifndef RenderBoxModelObject_h
#define RenderBoxModelObject_h

#include "RenderObject.h"
#include "ShadowData.h"

namespace WebCore {

// Modes for some of the line-related functions.
enum LinePositionMode { PositionOnContainingLine, PositionOfInteriorLineBoxes };
enum LineDirectionMode { HorizontalLine, VerticalLine };
typedef unsigned BorderEdgeFlags;

enum BackgroundBleedAvoidance {
    BackgroundBleedNone,
    BackgroundBleedShrinkBackground,
    BackgroundBleedUseTransparencyLayer
};

// This class is the base for all objects that adhere to the CSS box model as described
// at http://www.w3.org/TR/CSS21/box.html

class RenderBoxModelObject : public RenderObject {
public:
    RenderBoxModelObject(Node*);
    virtual ~RenderBoxModelObject();
    
    LayoutUnit relativePositionOffsetX() const;
    LayoutUnit relativePositionOffsetY() const;
    LayoutSize relativePositionOffset() const { return LayoutSize(relativePositionOffsetX(), relativePositionOffsetY()); }
    LayoutSize relativePositionLogicalOffset() const { return style()->isHorizontalWritingMode() ? relativePositionOffset() : relativePositionOffset().transposedSize(); }

    // IE extensions. Used to calculate offsetWidth/Height.  Overridden by inlines (RenderFlow)
    // to return the remaining width on a given line (and the height of a single line).
    virtual LayoutUnit offsetLeft() const;
    virtual LayoutUnit offsetTop() const;
    virtual LayoutUnit offsetWidth() const = 0;
    virtual LayoutUnit offsetHeight() const = 0;

    virtual void styleWillChange(StyleDifference, const RenderStyle* newStyle);
    virtual void styleDidChange(StyleDifference, const RenderStyle* oldStyle);
    virtual void updateBoxModelInfoFromStyle();

    bool hasSelfPaintingLayer() const;
    RenderLayer* layer() const { return m_layer; }
    virtual bool requiresLayer() const { return isRoot() || isPositioned() || isRelPositioned() || isTransparent() || hasOverflowClip() || hasTransform() || hasMask() || hasReflection() || style()->specifiesColumns(); }

    // This will work on inlines to return the bounding box of all of the lines' border boxes.
    virtual LayoutRect borderBoundingBox() const = 0;

    // Virtual since table cells override
    virtual int paddingTop(bool includeIntrinsicPadding = true) const;
    virtual int paddingBottom(bool includeIntrinsicPadding = true) const;
    virtual int paddingLeft(bool includeIntrinsicPadding = true) const;
    virtual int paddingRight(bool includeIntrinsicPadding = true) const;
    virtual int paddingBefore(bool includeIntrinsicPadding = true) const;
    virtual int paddingAfter(bool includeIntrinsicPadding = true) const;
    virtual int paddingStart(bool includeIntrinsicPadding = true) const;
    virtual int paddingEnd(bool includeIntrinsicPadding = true) const;

    virtual int borderTop() const { return style()->borderTopWidth(); }
    virtual int borderBottom() const { return style()->borderBottomWidth(); }
    virtual int borderLeft() const { return style()->borderLeftWidth(); }
    virtual int borderRight() const { return style()->borderRightWidth(); }
    virtual int borderBefore() const { return style()->borderBeforeWidth(); }
    virtual int borderAfter() const { return style()->borderAfterWidth(); }
    virtual int borderStart() const { return style()->borderStartWidth(); }
    virtual int borderEnd() const { return style()->borderEndWidth(); }

    int borderAndPaddingHeight() const { return borderTop() + borderBottom() + paddingTop() + paddingBottom(); }
    int borderAndPaddingWidth() const { return borderLeft() + borderRight() + paddingLeft() + paddingRight(); }
    int borderAndPaddingLogicalHeight() const { return borderBefore() + borderAfter() + paddingBefore() + paddingAfter(); }
    int borderAndPaddingLogicalWidth() const { return borderStart() + borderEnd() + paddingStart() + paddingEnd(); }
    int borderAndPaddingLogicalLeft() const { return style()->isHorizontalWritingMode() ? borderLeft() + paddingLeft() : borderTop() + paddingTop(); }

    int borderAndPaddingStart() const { return borderStart() + paddingStart(); }
    int borderLogicalLeft() const { return style()->isHorizontalWritingMode() ? borderLeft() : borderTop(); }
    int borderLogicalRight() const { return style()->isHorizontalWritingMode() ? borderRight() : borderBottom(); }

    virtual LayoutUnit marginTop() const = 0;
    virtual LayoutUnit marginBottom() const = 0;
    virtual LayoutUnit marginLeft() const = 0;
    virtual LayoutUnit marginRight() const = 0;
    virtual LayoutUnit marginBefore() const = 0;
    virtual LayoutUnit marginAfter() const = 0;
    virtual LayoutUnit marginStart() const = 0;
    virtual LayoutUnit marginEnd() const = 0;

    bool hasInlineDirectionBordersPaddingOrMargin() const { return hasInlineDirectionBordersOrPadding() || marginStart()|| marginEnd(); }
    bool hasInlineDirectionBordersOrPadding() const { return borderStart() || borderEnd() || paddingStart()|| paddingEnd(); }

    virtual int containingBlockLogicalWidthForContent() const;

    virtual void childBecameNonInline(RenderObject* /*child*/) { }

    void paintBorder(GraphicsContext*, const IntRect&, const RenderStyle*, BackgroundBleedAvoidance = BackgroundBleedNone, bool includeLogicalLeftEdge = true, bool includeLogicalRightEdge = true);
    bool paintNinePieceImage(GraphicsContext*, const IntRect&, const RenderStyle*, const NinePieceImage&, CompositeOperator = CompositeSourceOver);
    void paintBoxShadow(GraphicsContext*, const LayoutRect&, const RenderStyle*, ShadowStyle, bool includeLogicalLeftEdge = true, bool includeLogicalRightEdge = true);
    void paintFillLayerExtended(const PaintInfo&, const Color&, const FillLayer*, const LayoutRect&, BackgroundBleedAvoidance, InlineFlowBox* = 0, const LayoutSize& = LayoutSize(), CompositeOperator = CompositeSourceOver, RenderObject* backgroundObject = 0);
    
    // Overridden by subclasses to determine line height and baseline position.
    virtual int lineHeight(bool firstLine, LineDirectionMode, LinePositionMode = PositionOnContainingLine) const = 0;
    virtual int baselinePosition(FontBaseline, bool firstLine, LineDirectionMode, LinePositionMode = PositionOnContainingLine) const = 0;

    // Called by RenderObject::willBeDestroyed() and is the only way layers should ever be destroyed
    void destroyLayer();

    void highQualityRepaintTimerFired(Timer<RenderBoxModelObject>*);

    virtual void setSelectionState(SelectionState s);

protected:
    virtual void willBeDestroyed();

    class BackgroundImageGeometry {
    public:
        LayoutPoint destOrigin() const { return m_destOrigin; }
        void setDestOrigin(const LayoutPoint& destOrigin)
        {
            m_destOrigin = destOrigin;
        }
        
        LayoutRect destRect() const { return m_destRect; }
        void setDestRect(const LayoutRect& destRect)
        {
            m_destRect = destRect;
        }

        // Returns the phase relative to the destination rectangle.
        LayoutPoint relativePhase() const;
        
        LayoutPoint phase() const { return m_phase; }   
        void setPhase(const LayoutPoint& phase)
        {
            m_phase = phase;
        }

        LayoutSize tileSize() const { return m_tileSize; }    
        void setTileSize(const LayoutSize& tileSize)
        {
            m_tileSize = tileSize;
        }
        
        void setPhaseX(int x) { m_phase.setX(x); }
        void setPhaseY(int y) { m_phase.setY(y); }
        
        void setNoRepeatX(int xOffset);
        void setNoRepeatY(int yOffset);
        
        void useFixedAttachment(const LayoutPoint& attachmentPoint);
        
        void clip(const LayoutRect&);
    private:
        LayoutRect m_destRect;
        LayoutPoint m_destOrigin;
        LayoutPoint m_phase;
        LayoutSize m_tileSize;
    };

    void calculateBackgroundImageGeometry(const FillLayer*, const IntRect& paintRect, BackgroundImageGeometry&);
    void getBorderEdgeInfo(class BorderEdge[], bool includeLogicalLeftEdge = true, bool includeLogicalRightEdge = true) const;
    bool borderObscuresBackgroundEdge(const FloatSize& contextScale) const;

    bool shouldPaintAtLowQuality(GraphicsContext*, Image*, const void*, const IntSize&);

    RenderBoxModelObject* continuation() const;
    void setContinuation(RenderBoxModelObject*);

private:
    virtual bool isBoxModelObject() const { return true; }

    IntSize calculateFillTileSize(const FillLayer*, IntSize scaledSize) const;

    RoundedRect getBackgroundRoundedRect(const LayoutRect&, InlineFlowBox*, LayoutUnit inlineBoxWidth, LayoutUnit inlineBoxHeight,
        bool includeLogicalLeftEdge, bool includeLogicalRightEdge);

    void clipBorderSidePolygon(GraphicsContext*, const RoundedRect& outerBorder, const RoundedRect& innerBorder,
                               BoxSide, bool firstEdgeMatches, bool secondEdgeMatches);
    void paintOneBorderSide(GraphicsContext*, const RenderStyle*, const RoundedRect& outerBorder, const RoundedRect& innerBorder,
                                const IntRect& sideRect, BoxSide, BoxSide adjacentSide1, BoxSide adjacentSide2, const class BorderEdge[],
                                const Path*, BackgroundBleedAvoidance, bool includeLogicalLeftEdge, bool includeLogicalRightEdge, bool antialias, const Color* overrideColor = 0);
    void paintTranslucentBorderSides(GraphicsContext*, const RenderStyle*, const RoundedRect& outerBorder, const RoundedRect& innerBorder,
                          const class BorderEdge[], BackgroundBleedAvoidance, bool includeLogicalLeftEdge, bool includeLogicalRightEdge, bool antialias = false);
    void paintBorderSides(GraphicsContext*, const RenderStyle*, const RoundedRect& outerBorder, const RoundedRect& innerBorder,
                          const class BorderEdge[], BorderEdgeFlags, BackgroundBleedAvoidance,
                          bool includeLogicalLeftEdge, bool includeLogicalRightEdge, bool antialias = false, const Color* overrideColor = 0);
    void drawBoxSideFromPath(GraphicsContext*, const IntRect&, const Path&, const class BorderEdge[],
                            float thickness, float drawThickness, BoxSide, const RenderStyle*, 
                            Color, EBorderStyle, BackgroundBleedAvoidance, bool includeLogicalLeftEdge, bool includeLogicalRightEdge);

    friend class RenderView;

    RenderLayer* m_layer;
    
    // Used to store state between styleWillChange and styleDidChange
    static bool s_wasFloating;
    static bool s_hadLayer;
    static bool s_layerWasSelfPainting;
};

inline RenderBoxModelObject* toRenderBoxModelObject(RenderObject* object)
{ 
    ASSERT(!object || object->isBoxModelObject());
    return static_cast<RenderBoxModelObject*>(object);
}

inline const RenderBoxModelObject* toRenderBoxModelObject(const RenderObject* object)
{ 
    ASSERT(!object || object->isBoxModelObject());
    return static_cast<const RenderBoxModelObject*>(object);
}

// This will catch anyone doing an unnecessary cast.
void toRenderBoxModelObject(const RenderBoxModelObject*);

} // namespace WebCore

#endif // RenderBoxModelObject_h
