/*	
    WebBridge.mm
	Copyright (c) 2002, Apple, Inc. All rights reserved.
*/

#import <WebKit/WebBridge.h>

#import <WebKit/WebHTMLRepresentationPrivate.h>
#import <WebKit/WebHTMLViewPrivate.h>
#import <WebKit/WebSubresourceClient.h>
#import <WebKit/WebControllerPrivate.h>
#import <WebKit/WebFrameBridge.h>
#import <WebKit/WebDataSourcePrivate.h>
#import <WebKit/WebFramePrivate.h>
#import <WebKit/WebViewPrivate.h>
#import <WebKit/WebLoadProgress.h>
#import <WebKit/WebKitStatisticsPrivate.h>
#import <WebKit/WebKitDebug.h>

#import <WebFoundation/WebResourceHandle.h>

@interface NSApplication (DeclarationStolenFromAppKit)
- (void)_cycleWindowsReversed:(BOOL)reversed;
@end

@implementation WebBridge

- init
{
    ++WebBridgeCount;

    return [super init];
}

- (void)dealloc
{
    --WebBridgeCount;
    
    [super dealloc];
}

- (WebCoreFrameBridge *)frame
{
    WEBKIT_ASSERT(frame != nil);
    return [frame _frameBridge];
}

- (WebCoreBridge *)parent
{
    return [[[self dataSource] parent] _bridge];
}

- (NSArray *)childFrames
{
    NSArray *frames = [[self dataSource] children];
    NSEnumerator *e = [frames objectEnumerator];
    NSMutableArray *frameBridges = [NSMutableArray arrayWithCapacity:[frames count]];
    WebFrame *childFrame;
    while ((childFrame = [e nextObject])) {
        id frameBridge = [childFrame _frameBridge];
        if (frameBridge)
            [frameBridges addObject:frameBridge];
    }
    return frameBridges;
}

- (WebCoreFrameBridge *)descendantFrameNamed:(NSString *)name
{
    WEBKIT_ASSERT(frame != nil);
    return [[frame frameNamed:name] _frameBridge];
}

- (BOOL)createChildFrameNamed:(NSString *)frameName
    withURL:(NSURL *)URL renderPart:(KHTMLRenderPart *)renderPart
    allowsScrolling:(BOOL)allowsScrolling marginWidth:(int)width marginHeight:(int)height
{
    WEBKIT_ASSERT(frame != nil);
    WebFrame *newFrame = [[frame controller] createFrameNamed:frameName for:nil inParent:[self dataSource] allowsScrolling:allowsScrolling];
    if (newFrame == nil) {
        return NO;
    }
    
    [[newFrame _frameBridge] setRenderPart:renderPart];
    
    [[newFrame webView] _setMarginWidth:width];
    [[newFrame webView] _setMarginHeight:height];

    [[newFrame _frameBridge] loadURL:URL attributes:nil flags:0 withParent:[self dataSource]];
    
    return YES;
}

- (WebCoreBridge *)openNewWindowWithURL:(NSURL *)url
{
    WEBKIT_ASSERT(frame != nil);

    WebController *newController = [[[frame controller] windowContext] openNewWindowWithURL:url];
    WebFrame *newFrame = [newController mainFrame];

    return [newFrame _bridge];
}

- (BOOL)areToolbarsVisible
{
    WEBKIT_ASSERT(frame != nil);
    return [[[frame controller] windowContext] areToolbarsVisible];
}

- (void)setToolbarsVisible:(BOOL)visible
{
    WEBKIT_ASSERT(frame != nil);
    [[[frame controller] windowContext] setToolbarsVisible:visible];
}

- (BOOL)areScrollbarsVisible
{
    WEBKIT_ASSERT(frame != nil);
    return [[frame webView] allowsScrolling];
}

- (void)setScrollbarsVisible:(BOOL)visible
{
    WEBKIT_ASSERT(frame != nil);
    return [[frame webView] setAllowsScrolling:visible];
}

- (BOOL)isStatusBarVisible
{
    WEBKIT_ASSERT(frame != nil);
    return [[[frame controller] windowContext] isStatusBarVisible];
}

- (void)setStatusBarVisible:(BOOL)visible
{
    WEBKIT_ASSERT(frame != nil);
    [[[frame controller] windowContext] setStatusBarVisible:visible];
}

- (void)setWindowFrame:(NSRect)frameRect
{
    WEBKIT_ASSERT(frame != nil);
    [[[frame controller] windowContext] setFrame:frameRect];
}

- (NSWindow *)window
{
    WEBKIT_ASSERT(frame != nil);
    return [[[frame controller] windowContext] window];
}

- (void)setTitle:(NSString *)title
{
    [[self dataSource] _setTitle:title];
}

- (void)setStatusText:(NSString *)status
{
    WEBKIT_ASSERT(frame != nil);
    [[[frame controller] windowContext] setStatusText:status];
}

- (WebCoreFrameBridge *)mainFrame
{
    WEBKIT_ASSERT(frame != nil);
    return [[[frame controller] mainFrame] _frameBridge];
}

- (WebCoreFrameBridge *)frameNamed:(NSString *)name
{
    WEBKIT_ASSERT(frame != nil);
    return [[[frame controller] frameNamed:name] _frameBridge];
}

- (void)receivedData:(NSData *)data withDataSource:(WebDataSource *)withDataSource
{
    WEBKIT_ASSERT([self dataSource] == withDataSource);

    [self addData:data withEncoding:[withDataSource encoding]];
}

- (WebResourceHandle *)startLoadingResource:(id <WebCoreResourceLoader>)resourceLoader withURL:(NSURL *)URL
{
    return [WebSubresourceClient startLoadingResource:resourceLoader withURL:URL dataSource:[self dataSource]];
}

- (void)objectLoadedFromCache:(NSURL *)URL size:(unsigned)bytes
{
    WEBKIT_ASSERT(frame != nil);

    WebResourceHandle *handle = [[WebResourceHandle alloc] initWithURL:URL];

    WebLoadProgress *loadProgress = [[WebLoadProgress alloc] initWithBytesSoFar:bytes totalToLoad:bytes];
    [[frame controller] _receivedProgress:loadProgress forResourceHandle:handle fromDataSource: [self dataSource] complete:YES];
    [loadProgress release];
    [handle release];
}

- (void)setFrame: (WebFrame *)webFrame
{
    // FIXME: needed temporarily while we still use the dummy data
    // source hack
    if (webFrame == nil) {
	return;
    }

    WEBKIT_ASSERT(webFrame != nil);

    if (frame == nil) {
	// FIXME: non-retained because data source owns representation owns bridge
	frame = webFrame;
    } else {
	WEBKIT_ASSERT(frame == webFrame);
    }
}

- (void)dataSourceChanged
{
    // FIXME: needed temporarily while we still use the dummy data
    // source hack
    if ([frame dataSource] == nil) {
	[self openURL:nil];
    } else {
	[self openURL:[[self dataSource] inputURL]];
	if ([[self dataSource] redirectedURL]) {
	    [self setURL:[[self dataSource] redirectedURL]];
	}
    }
}

- (WebDataSource *)dataSource
{
    WEBKIT_ASSERT(frame != nil);
    WebDataSource *dataSource = [frame dataSource];

    WEBKIT_ASSERT(dataSource != nil);
    WEBKIT_ASSERT([dataSource _isCommitted]);

    return dataSource;
}


- (BOOL)openedByScript
{
    WEBKIT_ASSERT(frame != nil);
    return [[frame controller] _openedByScript];
}

- (void)setOpenedByScript:(BOOL)openedByScript
{
    WEBKIT_ASSERT(frame != nil);
    [[frame controller] _setOpenedByScript:openedByScript];
}

- (void)unfocusWindow
{
    if ([[self window] isKeyWindow] || [[[self window] attachedSheet] isKeyWindow]) {
	[NSApp _cycleWindowsReversed:FALSE];
    }
}

- (BOOL)modifierTrackingEnabled
{
    return [WebHTMLView _modifierTrackingEnabled];
}

- (void)setIconURL:(NSURL *)url
{
    [[self dataSource] _setIconURL:url];
}

- (void)setIconURL:(NSURL *)url withType:(NSString *)type
{
    [[self dataSource] _setIconURL:url withType:type];
}

@end
