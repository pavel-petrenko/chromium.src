/*	
    WebHTMLViewPrivate.m
    Copyright 2002, Apple, Inc. All rights reserved.
*/

#import <WebKit/WebHTMLViewPrivate.h>

#import <AppKit/NSGraphicsContextPrivate.h> // for PSbuttondown
#import <AppKit/NSResponder_Private.h>

#import <WebFoundation/WebAssertions.h>
#import <WebFoundation/WebNSURLExtras.h>

#import <WebKit/WebBridge.h>
#import <WebKit/WebContextMenuDelegate.h>
#import <WebKit/WebController.h>
#import <WebKit/WebControllerPolicyDelegate.h>
#import <WebKit/WebControllerPrivate.h>
#import <WebKit/WebDataSourcePrivate.h>
#import <WebKit/WebFramePrivate.h>
#import <WebKit/WebHTMLView.h>
#import <WebKit/WebImageRenderer.h>
#import <WebKit/WebNSPasteboardExtras.h>
#import <WebKit/WebNSViewExtras.h>
#import <WebKit/WebNetscapePluginEmbeddedView.h>
#import <WebKit/WebPreferences.h>
#import <WebKit/WebStringTruncator.h>
#import <WebKit/WebViewPrivate.h>
#import <WebKit/WebWindowOperationsDelegate.h>


// These are a little larger than typical because dragging links is a fairly
// advanced feature that can confuse non-power-users
#define DragStartXHysteresis  		10.0
#define DragStartYHysteresis  		10.0

#define DRAG_LABEL_BORDER_X		4.0
#define DRAG_LABEL_BORDER_Y		2.0
#define DRAG_LABEL_RADIUS	5

#define MIN_DRAG_LABEL_WIDTH_BEFORE_CLIP	120.0

#import <CoreGraphics/CGStyle.h>
#import <CoreGraphics/CGSTypes.h>
#import <CoreGraphics/CGContextGState.h>


static BOOL forceRealHitTest = NO;

@interface NSView (AppKitSecretsIKnowAbout)
- (void)_recursiveDisplayRectIfNeededIgnoringOpacity:(NSRect)rect isVisibleRect:(BOOL)isVisibleRect rectIsVisibleRectForView:(NSView *)visibleView topView:(BOOL)topView;
- (void)_recursiveDisplayAllDirtyWithLockFocus:(BOOL)needsLockFocus visRect:(NSRect)visRect;
- (NSRect)_dirtyRect;
- (void)_drawRect:(NSRect)rect clip:(BOOL)clip;
@end

@interface NSView (WebNSViewDisplayExtras)
- (void)_web_stopIfPluginView;
- (void)_web_propagateDirtyRectToAncestor;
- (void)_web_dumpDirtyRects;
@end

@interface WebNSTextView : NSTextView
{
}
@end

@interface WebNSView : NSView
{
}
@end

@interface WebNSWindow : NSWindow
{
}
@end

@interface NSMutableDictionary (WebHTMLViewExtras)
- (void)_web_setObjectIfNotNil:(id)object forKey:(id)key;
@end

@implementation WebHTMLViewPrivate

- (void)dealloc
{
    [mouseDownEvent release];
    [dragElement release];
    [draggingImageURL release];
    [super dealloc];
}

@end


@implementation WebHTMLView (WebPrivate)

// Danger Will Robinson. We have to poseAsClass: as early as possible
// so that any NSViews and NSWindows will be created with the
// appropriate poser.
+ (void)load
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    [[WebNSView class] poseAsClass:[NSView class]];
    [[WebNSTextView class] poseAsClass:[NSTextView class]];
    [[WebNSWindow class] poseAsClass:[NSWindow class]];
    [pool release];
}


- (void)_adjustFrames
{
    // Ick!  khtml set the frame size during layout and
    // the frame origins during drawing!  So we have to 
    // layout and do a draw with rendering disabled to
    // correclty adjust the frames.
    [[self _bridge] adjustFrames:[self frame]];
}

- (void)_reset
{
    NSArray *subviews = [[self subviews] copy];
    [subviews makeObjectsPerformSelector:@selector(_web_stopIfPluginView)];
    [subviews release];

    [WebImageRenderer stopAnimationsInView:self];
}

- (WebController *)_controller
{
    return [[self _web_parentWebView] controller];
}

- (WebFrame *)_frame
{
    WebView *webView = [self _web_parentWebView];
    return [[webView controller] frameForView:webView];
}

// Required so view can access the part's selection.
- (WebBridge *)_bridge
{
    return [[self _frame] _bridge];
}

+ (void)_postFlagsChangedEvent:(NSEvent *)flagsChangedEvent
{
    NSEvent *fakeEvent = [NSEvent mouseEventWithType:NSMouseMoved
        location:[[flagsChangedEvent window] convertScreenToBase:[NSEvent mouseLocation]]
        modifierFlags:[flagsChangedEvent modifierFlags]
        timestamp:[flagsChangedEvent timestamp]
        windowNumber:[flagsChangedEvent windowNumber]
        context:[flagsChangedEvent context]
        eventNumber:0 clickCount:0 pressure:0];

    // Pretend it's a mouse move.
    [[NSNotificationCenter defaultCenter]
        postNotificationName:NSMouseMovedNotification object:self
        userInfo:[NSDictionary dictionaryWithObject:fakeEvent forKey:@"NSEvent"]];
}

- (void)_updateMouseoverWithFakeEvent
{
    NSEvent *fakeEvent = [NSEvent mouseEventWithType:NSMouseMoved
        location:[[self window] convertScreenToBase:[NSEvent mouseLocation]]
        modifierFlags:[[NSApp currentEvent] modifierFlags]
        timestamp:[NSDate timeIntervalSinceReferenceDate]
        windowNumber:[[self window] windowNumber]
        context:[[NSApp currentEvent] context]
        eventNumber:0 clickCount:0 pressure:0];
    
    [self _updateMouseoverWithEvent:fakeEvent];
}

- (void)_frameOrBoundsChanged
{
    if (!NSEqualSizes(_private->lastLayoutSize, [(NSClipView *)[self superview] documentVisibleRect].size)) {
        [self setNeedsLayout:YES];
        [self setNeedsDisplay:YES];
    }
    
    // Don't update mouseover while mouse is down.
    int mouseIsDown;
    PSbuttondown(&mouseIsDown);
    if (!mouseIsDown) {
        SEL selector = @selector(_updateMouseoverWithFakeEvent);
        [NSObject cancelPreviousPerformRequestsWithTarget:self selector:selector object:nil];
        [self performSelector:selector withObject:nil afterDelay:0];
    }
}

- (NSDictionary *)_elementAtPoint:(NSPoint)point
{
    NSDictionary *elementInfoWC = [[self _bridge] elementAtPoint:point];
    NSMutableDictionary *elementInfo = [elementInfoWC mutableCopy];

    // Convert URL strings to NSURLs
    [elementInfo _web_setObjectIfNotNil:[NSURL _web_URLWithString:[elementInfoWC objectForKey:WebElementLinkURLKey]] forKey:WebElementLinkURLKey];
    [elementInfo _web_setObjectIfNotNil:[NSURL _web_URLWithString:[elementInfoWC objectForKey:WebElementImageURLKey]] forKey:WebElementImageURLKey];
    
    WebView *webView = [self _web_parentWebView];
    ASSERT(webView);
    WebFrame *webFrame = [[webView controller] frameForView:webView];
    
    if (webFrame) {
        NSString *frameName = [elementInfoWC objectForKey:WebElementLinkTargetFrameKey];
        if ([frameName length] == 0) {
            [elementInfo setObject:webFrame forKey:WebElementLinkTargetFrameKey];
        } else {
            WebFrame *wf = [webFrame findFrameNamed:frameName];
            if (wf != nil)
                [elementInfo setObject:wf forKey:WebElementLinkTargetFrameKey];
            else
                [elementInfo removeObjectForKey:WebElementLinkTargetFrameKey];
        }
    
        [elementInfo setObject:webFrame forKey:WebElementFrameKey];
    }
    
    return [elementInfo autorelease];
}

- (void)_setAsideSubviews
{
    ASSERT(!_private->subviewsSetAside);
    ASSERT(_private->savedSubviews == nil);
    _private->savedSubviews = _subviews;
    _subviews = nil;
    _private->subviewsSetAside = YES;
 }
 
 - (void)_restoreSubviews
 {
    ASSERT(_private->subviewsSetAside);
    ASSERT(_subviews == nil);
    _subviews = _private->savedSubviews;
    _private->savedSubviews = nil;
    _private->subviewsSetAside = NO;
}

// Don't let AppKit even draw subviews. We take care of that.
- (void)_recursiveDisplayRectIfNeededIgnoringOpacity:(NSRect)rect isVisibleRect:(BOOL)isVisibleRect rectIsVisibleRectForView:(NSView *)visibleView topView:(BOOL)topView
{
    [_subviews makeObjectsPerformSelector:@selector(_web_propagateDirtyRectToAncestor)];
    [self _setAsideSubviews];
    [super _recursiveDisplayRectIfNeededIgnoringOpacity:rect isVisibleRect:isVisibleRect
        rectIsVisibleRectForView:visibleView topView:topView];
    [self _restoreSubviews];
}

// Don't let AppKit even draw subviews. We take care of that.
- (void)_recursiveDisplayAllDirtyWithLockFocus:(BOOL)needsLockFocus visRect:(NSRect)visRect
{
    BOOL needToSetAsideSubviews = !_private->subviewsSetAside;
    
    if (needToSetAsideSubviews) {
        [_subviews makeObjectsPerformSelector:@selector(_web_propagateDirtyRectToAncestor)];
        [self _setAsideSubviews];
    }
    
    [super _recursiveDisplayAllDirtyWithLockFocus:needsLockFocus visRect:visRect];
    
    if (needToSetAsideSubviews) {
        [self _restoreSubviews];
    }
}

- (BOOL)_insideAnotherHTMLView
{
    NSView *view = self;
    while ((view = [view superview])) {
        if ([view isKindOfClass:[WebHTMLView class]]) {
            return YES;
        }
    }
    return NO;
}

- (void)scrollPoint:(NSPoint)point
{
    // Since we can't subclass NSTextView to do what we want, we have to second guess it here.
    // If we get called during the handling of a key down event, we assume the call came from
    // NSTextView, and ignore it and use our own code to decide how to page up and page down
    // We are smarter about how far to scroll, and we have "superview scrolling" logic.
    NSEvent *event = [[self window] currentEvent];
    if ([event type] == NSKeyDown) {
        const unichar pageUp = NSPageUpFunctionKey;
        if ([[event characters] rangeOfString:[NSString stringWithCharacters:&pageUp length:1]].length == 1) {
            [self tryToPerform:@selector(scrollPageUp:) with:nil];
            return;
        }
        const unichar pageDown = NSPageDownFunctionKey;
        if ([[event characters] rangeOfString:[NSString stringWithCharacters:&pageDown length:1]].length == 1) {
            [self tryToPerform:@selector(scrollPageDown:) with:nil];
            return;
        }
    }
    
    [super scrollPoint:point];
}

- (NSView *)hitTest:(NSPoint)point
{
    // WebHTMLView objects handle all left mouse clicks for objects inside them.
    // That does not include left mouse clicks with the control key held down.
    BOOL captureHitsOnSubviews;
    if (forceRealHitTest) {
        captureHitsOnSubviews = NO;
    } else {
        NSEvent *event = [[self window] currentEvent];
        captureHitsOnSubviews = [event type] == NSLeftMouseDown && ([event modifierFlags] & NSControlKeyMask) == 0;
    }
    if (!captureHitsOnSubviews) {
        return [super hitTest:point];
    }
    if ([[self superview] mouse:point inRect:[self frame]]) {
        return self;
    }
    return nil;
}

- (void)_updateMouseoverWithEvent:(NSEvent *)event
{
    WebHTMLView *view = nil;
    if ([event window] == [self window]) {
        forceRealHitTest = YES;
        NSView *hitView = [[[self window] contentView] hitTest:[event locationInWindow]];
        forceRealHitTest = NO;
        while (hitView) {
            if ([hitView isKindOfClass:[WebHTMLView class]]) {
                view = (WebHTMLView *)hitView;
                break;
            }
            hitView = [hitView superview];
        }
    }
    
    if (view == nil) {
        [[self _controller] _mouseDidMoveOverElement:nil modifierFlags:0];
    } else {
        [[view _bridge] mouseMoved:event];
        NSPoint point = [view convertPoint:[event locationInWindow] fromView:nil];
        [[self _controller] _mouseDidMoveOverElement:[view _elementAtPoint:point] modifierFlags:[event modifierFlags]];
    }
}

- (BOOL)_interceptKeyEvent:(NSEvent *)event toView:(NSView *)view
{
    return [[self _bridge] interceptKeyEvent:event toView:view];
}


+ (NSArray *)_pasteboardTypes
{
    return [NSArray arrayWithObjects:NSStringPboardType,
#if SUPPORT_HTML_PBOARD
        NSHTMLPboardType,
#endif
        NSRTFPboardType, nil];
}

- (void)_writeSelectionToPasteboard:(NSPasteboard *)pasteboard
{
    [pasteboard declareTypes:[[self class] _pasteboardTypes] owner:nil];
    [pasteboard setString:[self selectedString] forType:NSStringPboardType];

    // Put attributed string on the pasteboard.
    NSAttributedString *attributedString = [self selectedAttributedString];
    NSData *attributedData = [attributedString RTFFromRange:NSMakeRange(0, [attributedString length]) documentAttributes:nil];
    [pasteboard setData:attributedData forType:NSRTFPboardType];

#if SUPPORT_HTML_PBOARD
    // Put HTML on the pasteboard.
#endif
}


-(NSImage *)_dragImageForElement:(NSDictionary *)element
{
    NSURL *linkURL = [element objectForKey: WebElementLinkURLKey];

    BOOL drawURLString = YES;
    BOOL clipURLString = NO;
    
    NSString *label = [element objectForKey: WebElementLinkLabelKey];
    NSString *urlString = [linkURL absoluteString];
    
    if (!label) {
	drawURLString = NO;
	label = urlString;
    }
    
    // FIXME: This mega-block of code needs to be cleaned-up or put into another method.
    NSFont *labelFont = [NSFont systemFontOfSize: 12.0];
    NSFont *urlFont = [NSFont systemFontOfSize: 8.0];
    NSDictionary *labelAttributes = [NSDictionary dictionaryWithObjectsAndKeys: labelFont, NSFontAttributeName, [NSColor whiteColor], NSForegroundColorAttributeName, nil];
    NSDictionary *urlAttributes = [NSDictionary dictionaryWithObjectsAndKeys: urlFont, NSFontAttributeName, [NSColor whiteColor], NSForegroundColorAttributeName, nil];
    NSSize labelSize = [label sizeWithAttributes: labelAttributes];
    NSSize imageSize, urlStringSize;
    imageSize.width += labelSize.width + DRAG_LABEL_BORDER_X * 2;
    imageSize.height += labelSize.height + DRAG_LABEL_BORDER_Y *2;
    if (drawURLString) {
	urlStringSize = [urlString sizeWithAttributes: urlAttributes];
	imageSize.height += urlStringSize.height;
	// Clip the url string to 2.5 times the width of the label.
	if (urlStringSize.width > MAX(2.5 * labelSize.width, MIN_DRAG_LABEL_WIDTH_BEFORE_CLIP)) {
	    imageSize.width = MAX((labelSize.width * 2.5) + DRAG_LABEL_BORDER_X * 2, MIN_DRAG_LABEL_WIDTH_BEFORE_CLIP);
	    clipURLString = YES;
	} else {
	    imageSize.width = MAX(labelSize.width + DRAG_LABEL_BORDER_X * 2, urlStringSize.width + DRAG_LABEL_BORDER_X * 2);
	}
    }
    NSImage *dragImage = [[[NSImage alloc] initWithSize: imageSize] autorelease];
    [dragImage lockFocus];
    
    [[NSColor colorWithCalibratedRed: 0.5 green: 0.5 blue: 0.5 alpha: 0.8] set];
    
    // Drag a rectangle with rounded corners/
    NSBezierPath *path = [NSBezierPath bezierPath];
    [path appendBezierPathWithOvalInRect: NSMakeRect(0,0, DRAG_LABEL_RADIUS * 2, DRAG_LABEL_RADIUS * 2)];
    [path appendBezierPathWithOvalInRect: NSMakeRect(0,imageSize.height - DRAG_LABEL_RADIUS * 2, DRAG_LABEL_RADIUS * 2, DRAG_LABEL_RADIUS * 2)];
    [path appendBezierPathWithOvalInRect: NSMakeRect(imageSize.width - DRAG_LABEL_RADIUS * 2, imageSize.height - DRAG_LABEL_RADIUS * 2, DRAG_LABEL_RADIUS * 2, DRAG_LABEL_RADIUS * 2)];
    [path appendBezierPathWithOvalInRect: NSMakeRect(imageSize.width - DRAG_LABEL_RADIUS * 2,0, DRAG_LABEL_RADIUS * 2, DRAG_LABEL_RADIUS * 2)];
    
    [path appendBezierPathWithRect: NSMakeRect(DRAG_LABEL_RADIUS, 0, imageSize.width - DRAG_LABEL_RADIUS * 2, imageSize.height)];
    [path appendBezierPathWithRect: NSMakeRect(0, DRAG_LABEL_RADIUS, DRAG_LABEL_RADIUS + 10, imageSize.height - 2 * DRAG_LABEL_RADIUS)];
    [path appendBezierPathWithRect: NSMakeRect(imageSize.width - DRAG_LABEL_RADIUS - 20,DRAG_LABEL_RADIUS, DRAG_LABEL_RADIUS + 20, imageSize.height - 2 * DRAG_LABEL_RADIUS)];
    [path fill];
    
    // Draw the label with a slight shadow.
    CGShadowStyle shadow;
    CGSGenericObj style;
    
    shadow.version    = 0;
    shadow.elevation  = kCGShadowElevationDefault;
    shadow.azimuth    = 136.869995;
    shadow.ambient    = 0.317708;
    shadow.height     = 2.187500;
    shadow.radius     = 1.875000;
    shadow.saturation = kCGShadowSaturationDefault;
    style = CGStyleCreateShadow(&shadow);
    [NSGraphicsContext saveGraphicsState];
    CGContextSetStyle([[NSGraphicsContext currentContext] graphicsPort], style);
    
    if (drawURLString) {
	if (clipURLString) {
	    urlString = [WebStringTruncator rightTruncateString: urlString toWidth:imageSize.width - (DRAG_LABEL_BORDER_X * 2) withFont:urlFont];
	}
	[urlString drawAtPoint: NSMakePoint(DRAG_LABEL_BORDER_X, DRAG_LABEL_BORDER_Y) withAttributes: urlAttributes];
    }
    [label drawAtPoint: NSMakePoint (DRAG_LABEL_BORDER_X, DRAG_LABEL_BORDER_Y + urlStringSize.height) withAttributes: labelAttributes];
    
    [NSGraphicsContext restoreGraphicsState];
    CGStyleRelease(style);
    
    [dragImage unlockFocus];
    
    return dragImage;
}

- (void)_handleMouseDragged:(NSEvent *)event
{
    // If the frame has a provisional data source, this view may be released.
    // Don't allow drag because drag callbacks will reference this released view.
    if ([[self _frame] provisionalDataSource]) {
	return;
    }
    
    NSPoint mouseDownPoint = [_private->mouseDownEvent locationInWindow];
    float deltaX = ABS([event locationInWindow].x - mouseDownPoint.x);
    float deltaY = ABS([event locationInWindow].y - mouseDownPoint.y);

    NSDictionary *element = _private->dragElement;

    NSURL *linkURL = [element objectForKey: WebElementLinkURLKey];
    NSURL *imageURL = [element objectForKey: WebElementImageURLKey];

    [_private->draggingImageURL release];
    _private->draggingImageURL = nil;

    // We must have started over something draggable:
    ASSERT((imageURL && [[WebPreferences standardPreferences] willLoadImagesAutomatically]) || (!imageURL && linkURL)); 

    // drag hysteresis hasn't ben met yet but we don't want to do
    // other drag actions like selection.
    if (deltaX < DragStartXHysteresis && deltaY < DragStartYHysteresis) {
	return;
    }

    if (imageURL) {
	_private->draggingImageURL = [imageURL retain];
	
	[self _web_dragPromisedImage:[element objectForKey:WebElementImageKey]
	      origin:[[element objectForKey:WebElementImageLocationKey] pointValue]
	      URL:linkURL ? linkURL : imageURL
	      fileType:[[imageURL path] pathExtension]
	      title:[element objectForKey:WebElementImageAltStringKey]
	      event:_private->mouseDownEvent];
    } else if (linkURL) {
	NSImage *dragImage = [self _dragImageForElement:element];
	NSString *label = [element objectForKey: WebElementLinkLabelKey];
	
	NSPasteboard *pasteboard = [NSPasteboard pasteboardWithName:NSDragPboard];
	[pasteboard _web_writeURL:linkURL andTitle:label withOwner:self];
	
	NSSize imageSize = [dragImage size];
	NSPoint mousePoint = [self convertPoint:[event locationInWindow] fromView:nil];
	NSSize centerOffset = NSMakeSize(imageSize.width / 2, -DRAG_LABEL_BORDER_Y);
	NSPoint imagePoint = NSMakePoint(mousePoint.x - centerOffset.width, mousePoint.y - centerOffset.height);
	
	[self dragImage:dragImage
	      at:imagePoint
	      offset:centerOffset
	      event:event
	      pasteboard:pasteboard
	      source:self
	      slideBack:NO];
    }
}

- (void)_handleAutoscrollForMouseDragged:(NSEvent *)event
{
    // FIXME: this really needs to be based on a timer
    [self autoscroll:event];
}

- (BOOL)_mayStartDragWithMouseDown:(NSEvent *)event
{
    NSPoint mouseDownPoint = [event locationInWindow];

    NSPoint point = [self convertPoint:mouseDownPoint fromView:nil];

    NSDictionary *element = [[self _elementAtPoint: point] retain];
    [_private->dragElement release];
    _private->dragElement = element;

    NSURL *linkURL = [element objectForKey: WebElementLinkURLKey];
    NSURL *imageURL = [element objectForKey: WebElementImageURLKey];

    // are we over something draggable?
    return (imageURL && [[WebPreferences standardPreferences] willLoadImagesAutomatically]) || (!imageURL && linkURL);
}

@end

@implementation NSView (WebHTMLViewPrivate)

- (void)_web_stopIfPluginView
{
    if ([self isKindOfClass:[WebNetscapePluginEmbeddedView class]]) {
	WebNetscapePluginEmbeddedView *pluginView = (WebNetscapePluginEmbeddedView *)self;
        [pluginView stop];
    }
}

- (void)_web_propagateDirtyRectToAncestor
{
    [_subviews makeObjectsPerformSelector:@selector(_web_propagateDirtyRectToAncestor)];
    if ([self needsDisplay]) {
        [[self superview] setNeedsDisplayInRect:[self convertRect:[self _dirtyRect] toView:[self superview]]];
    }
}

@end

@implementation WebNSTextView

static BOOL inNSTextViewDrawRect;

// This code is here to make insertion point drawing work in a way that respects the
// HTML view layering. If we can find a way to make it work without poseAsClass, we
// should do that.

- (BOOL)_web_inHTMLView
{
    NSView *view = self;
    for (;;) {
        NSView *superview = [view superview];
        if (!superview) {
            return NO;
        }
        view = superview;
        if ([view isKindOfClass:[WebHTMLView class]]) {
            return YES;
        }
    }
}

- (BOOL)isOpaque
{
    if (![self _web_inHTMLView]) {
        return [super isOpaque];
    }

    // Text views in the HTML view all say they are not opaque.
    // This prevents the insertion point rect cache from being used,
    // and all the side effects are good since we want the view to act
    // opaque anyway. This could go in NSView instead of NSTextView,
    // but we need to pose as NSTextView anyway for the other override.
    // If we did this in NSView, we wouldn't need to call _web_propagateDirtyRectToAncestor.
    return NO;
}

- (void)drawInsertionPointInRect:(NSRect)rect color:(NSColor *)color turnedOn:(BOOL)turnedOn
{
    if (![self _web_inHTMLView]) {
        [super drawInsertionPointInRect:rect color:color turnedOn:turnedOn];
        return;
    }
    
    // Use the display mechanism to do all insertion point drawing in the web view.
    if (inNSTextViewDrawRect) {
        [super drawInsertionPointInRect:rect color:color turnedOn:turnedOn];
        return;
    }
    [super drawInsertionPointInRect:rect color:color turnedOn:NO];
    if (turnedOn) {
        rect.size.width = 1;
        [self setNeedsDisplayInRect:rect];
    }
}

- (void)_drawRect:(NSRect)rect clip:(BOOL)clip
{
    ASSERT(!inNSTextViewDrawRect);
    inNSTextViewDrawRect = YES;
    [super _drawRect:rect clip:clip];
    inNSTextViewDrawRect = NO;
}

@end

@implementation WebNSView

- (NSView *)opaqueAncestor
{
    if (![self isOpaque]) {
        return [super opaqueAncestor];
    }
    NSView *opaqueAncestor = self;
    NSView *superview = self;
    while ((superview = [superview superview])) {
        if ([superview isKindOfClass:[WebHTMLView class]]) {
            opaqueAncestor = superview;
        }
    }
    return opaqueAncestor;
}

@end

@implementation WebNSWindow

- (void)sendEvent:(NSEvent *)event
{
    if ([event type] == NSKeyDown || [event type] == NSKeyUp) {
	NSResponder *responder = [self firstResponder];

	NSView *view;
	while (responder != nil && ![responder isKindOfClass:[WebNSView class]]) {
	    responder = [responder nextResponder];
	}

	view = (NSView *)responder;

	NSView *ancestorHTMLView = view;
	while (ancestorHTMLView != nil && ![ancestorHTMLView isKindOfClass:[WebHTMLView class]]) {
	    ancestorHTMLView = [ancestorHTMLView superview];
	}
	    
	if (ancestorHTMLView != nil) {
	    if ([(WebHTMLView *)ancestorHTMLView _interceptKeyEvent:event toView:view]) {

	    }
	}
    }

    [super sendEvent:event];
}

@end

@implementation NSMutableDictionary (WebHTMLViewExtras)

- (void)_web_setObjectIfNotNil:(id)object forKey:(id)key
{
    if (object == nil) {
        [self removeObjectForKey:key];
    } else {
        [self setObject:object forKey:key];
    }
}

@end
