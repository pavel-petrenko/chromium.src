/*
 * Copyright (C) 2005 Apple Computer, Inc.  All rights reserved.
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


#import "KRenderingPaintServerQuartz.h"
#import "QuartzSupport.h"
#import "KCanvasResourcesQuartz.h"
#import "KRenderingDeviceQuartz.h"

#import "KRenderingStyle.h"
#import "KRenderingPaintServer.h"
#import "KRenderingFillPainter.h"
#import "KRenderingStrokePainter.h"
#import "KCanvas.h"
#import "KCanvasMatrix.h"
#import "KRenderingDevice.h"

#import "KWQLogging.h"

void KRenderingPaintServerSolidQuartz::draw(KRenderingDeviceContext *renderingContext, const KCanvasCommonArgs &args, KCPaintTargetType type) const
{
	//NSLog(@"KRenderingPaintServerSolidQuartz::draw()");
	KRenderingDeviceContextQuartz *quartzContext = static_cast<KRenderingDeviceContextQuartz *>(renderingContext);
	CGContextRef context = quartzContext->cgContext();
	KRenderingStyle *style = args.style();
	
	applyStyleToContext(context, style);
		
	if ( (type & APPLY_TO_FILL) && style->isFilled() ) {
		//NSLog(@"Filling in %p bbox(%@) with color: %@", context, NSStringFromRect(*(NSRect *)&CGContextGetPathBoundingBox(context)), nsColor(color()));
		CGColorRef colorCG = cgColor(color());
		CGColorRef withAlpha = CGColorCreateCopyWithAlpha(colorCG, style->fillPainter()->opacity() * double(style->opacity() / 255.) * opacity());
		CGContextSetFillColorWithColor(context, withAlpha);
		CGColorRelease(colorCG);
		CGColorRelease(withAlpha);
		if (style->fillPainter()->fillRule() == RULE_EVENODD) {
			CGContextEOFillPath(context);
		} else {
			CGContextFillPath(context);
		}
	}
	
	if ( (type & APPLY_TO_STROKE) && style->isStroked() ) {
		//NSLog(@"Stroking in %p bbox(%@) with color: %@", context, NSStringFromRect(*(NSRect *)&CGContextGetPathBoundingBox(context)), nsColor(color()));
		CGColorRef colorCG = cgColor(color());
		CGColorRef withAlpha = CGColorCreateCopyWithAlpha(colorCG, style->strokePainter()->opacity() * double(style->opacity() / 255.) * opacity());		
		CGContextSetStrokeColorWithColor(context, withAlpha);
		CGColorRelease(colorCG);
		CGColorRelease(withAlpha);
		
		applyStrokeStyleToContext(context, style);
		
		CGContextStrokePath(context);
	}
}


void patternCallback(void *info, CGContextRef context)
{
	KCanvasImageQuartz *image = (KCanvasImageQuartz *)info;
	CGLayerRef layer = image->cgLayer();
	CGContextDrawLayerAtPoint(context, CGPointZero, layer);
}

void KRenderingPaintServerPatternQuartz::draw(KRenderingDeviceContext *renderingContext, const KCanvasCommonArgs &args, KCPaintTargetType type) const
{
	KRenderingDeviceContextQuartz *quartzContext = static_cast<KRenderingDeviceContextQuartz *>(renderingContext);
	CGContextRef context = quartzContext->cgContext();
	KRenderingStyle *style = args.style();

    KCanvasImage *cell = tile();
    if (!cell) {
        NSLog(@"No image associated with pattern: %p can't draw!", this);
        return;
    }
	
	CGContextSaveGState(context);

	CGSize cellSize = CGSize(cell->size());
	
	float alpha = 1; // style->opacity() / 255.0f; //which?
		
	// Patterns don't seem to resepect the CTM unless we make them...
	CGAffineTransform ctm = CGContextGetCTM(context);
	CGAffineTransform transform = CGAffineTransform(patternTransform().qmatrix());
	CGSize phase = CGSizeMake(x(), y());
	if (boundingBoxMode()) {
		// get the object bbox
		CGRect objectBBox = CGContextGetPathBoundingBox(context);
		phase.width += objectBBox.origin.x;
		phase.height += objectBBox.origin.y;
	}
	transform = CGAffineTransformTranslate(transform, phase.width, phase.height);
	transform = CGAffineTransformConcat(transform, ctm);
		
	CGPatternCallbacks callbacks = {0, patternCallback, NULL};
	CGPatternRef pattern = CGPatternCreate (
	   tile(),
	   CGRectMake(0,0,cellSize.width,cellSize.height),
	   transform,
	   width(), //cellSize.width,
	   height(), //cellSize.height,
	   kCGPatternTilingConstantSpacing,  // FIXME: should ask CG guys.
	   true, // has color
	   &callbacks );
	   
	applyStyleToContext(context, style); // or do I set the alpha above?
	
	CGColorSpaceRef patternSpace = CGColorSpaceCreatePattern(NULL);
	
	
//	CGSize phase = CGSizeMake(x(), y());
//	if (boundingBoxMode()) {
//		// get the object bbox
//		CGRect objectBBox = CGContextGetPathBoundingBox(context);
//		phase.width += objectBBox.origin.x;
//		phase.height += objectBBox.origin.y;
//	}
	//CGContextSetPatternPhase(context, phase);
	CGContextSetPatternPhase(context, CGSizeZero); // FIXME: this might be right... might not.
	
	//NSLog(@"pattern phase: %f, %f transform: %@", phase.width, phase.height, CFStringFromCGAffineTransform(transform));
	
	if ( (type & APPLY_TO_FILL) && style->isFilled() ) {
		CGContextSetFillColorSpace(context, patternSpace);
		CGContextSetFillPattern(context, pattern, &alpha);
		if (style->fillPainter()->fillRule() == RULE_EVENODD) {
			CGContextEOFillPath(context);
		} else {
			CGContextFillPath(context);
		}
	}
	
	if ( (type & APPLY_TO_STROKE) && style->isStroked() ) {
		CGContextSetStrokeColorSpace(context, patternSpace);
		CGContextSetStrokePattern(context, pattern, &alpha);		
		applyStrokeStyleToContext(context, style);
		CGContextStrokePath(context);
	}
	
	CGPatternRelease(pattern);
	CGColorSpaceRelease (patternSpace);
	
	CGContextRestoreGState(context);
}

void KRenderingPaintServerImageQuartz::draw(KRenderingDeviceContext *renderingContext, const KCanvasCommonArgs &args, KCPaintTargetType type) const
{
	NSLog(@"KRenderingPaintServerImageQuartz::draw() not implemented!");
}
