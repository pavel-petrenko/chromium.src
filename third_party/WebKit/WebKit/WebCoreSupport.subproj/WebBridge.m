/*	
    IFWebCoreBridge.mm
	Copyright (c) 2002, Apple, Inc. All rights reserved.
*/

#import <WebKit/IFWebCoreBridge.h>

#import <WebKit/IFResourceURLHandleClient.h>
#import <WebKit/IFWebControllerPrivate.h>
#import <WebKit/IFWebCoreFrame.h>
#import <WebKit/IFWebDataSourcePrivate.h>
#import <WebKit/IFWebFramePrivate.h>
#import <WebKit/IFWebViewPrivate.h>

#import <WebKit/WebKitDebug.h>

@implementation IFWebDataSource (IFWebCoreBridge)

- (IFWebCoreBridge *)_bridge
{
    id representation = [self representation];
    return [representation respondsToSelector:@selector(_bridge)] ? [representation _bridge] : nil;
}

@end

@implementation IFWebCoreBridge

- (void)dealloc
{
    [dataSource release];
    
    [super dealloc];
}

- (id <WebCoreFrame>)frame
{
    return [[dataSource webFrame] _bridgeFrame];
}

- (WebCoreBridge *)parent
{
    WEBKIT_ASSERT(dataSource);
    return [[dataSource parent] _bridge];
}

- (NSArray *)childFrames
{
    WEBKIT_ASSERT(dataSource);
    NSArray *frames = [dataSource children];
    NSEnumerator *e = [frames objectEnumerator];
    NSMutableArray *bridgeFrames = [NSMutableArray arrayWithCapacity:[frames count]];
    IFWebFrame *frame;
    while ((frame = [e nextObject])) {
        id bridgeFrame = [frame _bridgeFrame];
        if (bridgeFrame)
            [bridgeFrames addObject:bridgeFrame];
    }
    return bridgeFrames;
}

- (id <WebCoreFrame>)childFrameNamed:(NSString *)name
{
    return [[dataSource frameNamed:name] _bridgeFrame];
}

- (id <WebCoreFrame>)descendantFrameNamed:(NSString *)name
{
    return [[[dataSource webFrame] frameNamed:name] _bridgeFrame];
}

- (BOOL)createChildFrameNamed:(NSString *)frameName
    withURL:(NSURL *)URL renderPart:(khtml::RenderPart *)renderPart
    allowsScrolling:(BOOL)allowsScrolling marginWidth:(int)width marginHeight:(int)height
{
    WEBKIT_ASSERT(dataSource);

    IFWebFrame *frame = [[dataSource controller] createFrameNamed:frameName for:nil inParent:dataSource allowsScrolling:allowsScrolling];
    if (frame == nil) {
        return NO;
    }
    
    [frame _setRenderFramePart:renderPart];
    
    [[frame webView] _setMarginWidth:width];
    [[frame webView] _setMarginHeight:height];

    [[frame _bridgeFrame] loadURL:URL attributes:nil flags:0 withParent:dataSource];
    
    return YES;
}

- (void)openNewWindowWithURL:(NSURL *)url
{
    [[dataSource controller] openNewWindowWithURL:url];
}

- (void)setTitle:(NSString *)title
{
    WEBKIT_ASSERT(dataSource);
    [dataSource _setTitle:title];
}

- (id <WebCoreFrame>)mainFrame
{
    return [[[dataSource controller] mainFrame] _bridgeFrame];
}

- (id <WebCoreFrame>)frameNamed:(NSString *)name
{
    return [[[dataSource controller] frameNamed:name] _bridgeFrame];
}

- (void)receivedData:(NSData *)data withDataSource:(IFWebDataSource *)withDataSource
{
    if (dataSource == nil) {
        dataSource = [withDataSource retain];
        [self openURL:[dataSource inputURL]];
    } else {
        WEBKIT_ASSERT(dataSource == withDataSource);
    }
    
    [self addData:data withEncoding:[dataSource encoding]];
}

- (IFURLHandle *)startLoadingResource:(id <WebCoreResourceLoader>)resourceLoader withURL:(NSURL *)URL
{
    return [IFResourceURLHandleClient startLoadingResource:resourceLoader withURL:URL dataSource:dataSource];
}

@end
