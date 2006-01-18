/*
 * Copyright (C) 2005, 2006 Apple Computer, Inc.  All rights reserved.
 *               2006 Alexander Kellett <lypanov@kde.org>
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
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
#import "KRenderingPaintServerQuartz.h"
#import "QuartzSupport.h"
#import "KCanvasResourcesQuartz.h"
#import "KRenderingDeviceQuartz.h"

#import "KCanvasRenderingStyle.h"
#import "KRenderingPaintServer.h"
#import "KRenderingFillPainter.h"
#import "KRenderingStrokePainter.h"
#import "KCanvasMatrix.h"
#import "KRenderingDevice.h"

#import "KWQLogging.h"

void KRenderingPaintServerSolidQuartz::draw(KRenderingDeviceContext *renderingContext, const RenderPath *renderPath, KCPaintTargetType type) const
{
    KRenderingDeviceContextQuartz *quartzContext = static_cast<KRenderingDeviceContextQuartz *>(renderingContext);
    CGContextRef context = quartzContext->cgContext();
    khtml::RenderStyle *renderStyle = renderPath->style();

    CGContextSetAlpha(context, renderStyle->opacity());
        
    if ((type & APPLY_TO_FILL) && KSVG::KSVGPainterFactory::isFilled(renderStyle)) {
        CGColorRef colorCG = cgColor(color());
        CGColorRef withAlpha = CGColorCreateCopyWithAlpha(colorCG, KSVG::KSVGPainterFactory::fillPainter(renderStyle, renderPath).opacity());
        CGContextSetFillColorWithColor(context, withAlpha);
        CGColorRelease(colorCG);
        CGColorRelease(withAlpha);
        if (KSVG::KSVGPainterFactory::fillPainter(renderStyle, renderPath).fillRule() == RULE_EVENODD)
            CGContextEOFillPath(context);
        else
            CGContextFillPath(context);
    }

    if ((type & APPLY_TO_STROKE) && KSVG::KSVGPainterFactory::isStroked(renderStyle)) {
        CGColorRef colorCG = cgColor(color());
        CGColorRef withAlpha = CGColorCreateCopyWithAlpha(colorCG, KSVG::KSVGPainterFactory::strokePainter(renderStyle, renderPath).opacity());         
        CGContextSetStrokeColorWithColor(context, withAlpha);
        CGColorRelease(colorCG);
        CGColorRelease(withAlpha);
        
        applyStrokeStyleToContext(context, renderStyle, renderPath);
        
        CGContextStrokePath(context);
    }
}


void patternCallback(void *info, CGContextRef context)
{
    KCanvasImageQuartz *image = (KCanvasImageQuartz *)info;
    CGLayerRef layer = image->cgLayer();
    CGContextDrawLayerAtPoint(context, CGPointZero, layer);
}

void KRenderingPaintServerPatternQuartz::draw(KRenderingDeviceContext *renderingContext, const RenderPath *renderPath, KCPaintTargetType type) const
{
    if(listener()) // this seems like bad design to me, should be in a common baseclass. -- ecs 8/6/05
        listener()->resourceNotification();

    khtml::RenderStyle *renderStyle = renderPath->style();

    KRenderingDeviceContextQuartz *quartzContext = static_cast<KRenderingDeviceContextQuartz *>(renderingContext);
    CGContextRef context = quartzContext->cgContext();

    KCanvasImage *cell = tile();
    if (!cell)
        return;

    CGContextSaveGState(context);

    CGSize cellSize = CGSize(cell->size());

    float alpha = 1; // canvasStyle->opacity(); //which?
            
    // Patterns don't seem to resepect the CTM unless we make them...
    CGAffineTransform ctm = CGContextGetCTM(context);
    CGAffineTransform transform = CGAffineTransform(patternTransform().qmatrix());
    transform = CGAffineTransformConcat(transform, ctm);

    CGSize phase = CGSizeMake(bbox().x(), bbox().y());
    CGContextSetPatternPhase(context, phase);

    CGPatternCallbacks callbacks = {0, patternCallback, NULL};
    CGPatternRef pattern = CGPatternCreate (
        tile(),
        CGRectMake(0,0,cellSize.width,cellSize.height),
        transform,
        bbox().width(), //cellSize.width,
        bbox().height(), //cellSize.height,
        kCGPatternTilingConstantSpacing,  // FIXME: should ask CG guys.
        true, // has color
        &callbacks );

    CGContextSetAlpha(context, renderStyle->opacity()); // or do I set the alpha above?

    CGColorSpaceRef patternSpace = CGColorSpaceCreatePattern(NULL);

    if ((type & APPLY_TO_FILL) && KSVG::KSVGPainterFactory::isFilled(renderStyle)) {
        CGContextSetFillColorSpace(context, patternSpace);
        CGContextSetFillPattern(context, pattern, &alpha);
        if (KSVG::KSVGPainterFactory::fillPainter(renderStyle, renderPath).fillRule() == RULE_EVENODD)
            CGContextEOFillPath(context);
        else
            CGContextFillPath(context);
    }
    
    if ((type & APPLY_TO_STROKE) && KSVG::KSVGPainterFactory::isStroked(renderStyle)) {
        CGContextSetStrokeColorSpace(context, patternSpace);
        CGContextSetStrokePattern(context, pattern, &alpha);
        applyStrokeStyleToContext(context, renderStyle, renderPath);
        CGContextStrokePath(context);
    }

    CGPatternRelease(pattern);
    CGColorSpaceRelease (patternSpace);

    CGContextRestoreGState(context);
}