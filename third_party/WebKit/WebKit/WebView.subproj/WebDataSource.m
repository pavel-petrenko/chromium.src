/*	
        WebDataSource.m
	Copyright 2001, 2002, Apple, Inc. All rights reserved.
*/

#import <WebKit/WebDocument.h>
#import <WebKit/WebDownloadHandler.h>
#import <WebKit/WebException.h>
#import <WebKit/WebHTMLRepresentation.h>
#import <WebKit/WebMainResourceClient.h>
#import <WebKit/WebBridge.h>
#import <WebKit/WebDataSourcePrivate.h>
#import <WebKit/WebController.h>
#import <WebKit/WebFramePrivate.h>
#import <WebKit/WebView.h>
#import <WebFoundation/WebAssertions.h>
#import <WebKit/WebKitStatisticsPrivate.h>

#import <WebFoundation/WebFileTypeMappings.h>
#import <WebFoundation/WebResourceHandle.h>
#import <WebFoundation/WebResourceRequest.h>
#import <WebFoundation/WebResourceResponse.h>
#import <WebFoundation/WebNSDictionaryExtras.h>

@implementation WebDataSource

-(id)initWithURL:(NSURL *)URL
{
    id result = nil;

    WebResourceRequest *request = [[WebResourceRequest alloc] initWithURL:URL];
    if (request) {
        result = [self initWithRequest:request];
        [request release];
    }
    else {
        [self release];
    }
    
    return result;
}

-(id)initWithRequest:(WebResourceRequest *)request
{
    self = [super init];
    if (!self) {
        return nil;
    }
    
    _private = [[WebDataSourcePrivate alloc] init];
    _private->originalRequest = [request retain];
    _private->request = [request retain];

    ++WebDataSourceCount;
    
    return self;
}

- (void)dealloc
{
    --WebDataSourceCount;
    
    [_private release];
    
    [super dealloc];
}

- (NSData *)data
{
    if(!_private->resourceData){
        return [_private->mainClient resourceData];
    }else{
        return _private->resourceData;
    }
}

- (id <WebDocumentRepresentation>) representation
{
    return _private->representation;
}

- (WebFrame *)webFrame
{
    return [_private->controller frameForDataSource: self];
}

// Returns the name of the frame containing this data source, or nil
// if the data source is not in a frame set.
- (NSString *)frameName 
{
    return [[self webFrame] name];    
}

- (WebController *)controller
{
    // All data sources used in a document share the same controller.
    // A single document may have many data sources corresponding to
    // frames or iframes.
    return _private->controller;
}

-(WebResourceRequest *)request
{
    return _private->request;
}

- (WebResourceResponse *)response
{
    return _private->response;
}

// May return nil if not initialized with a URL.
- (NSURL *)URL
{
    return [[self request] URL];
}


- (void)startLoading
{
    [self _startLoading];
}


// Cancels any pending loads.  A data source is conceptually only ever loading
// one document at a time, although one document may have many related
// resources.  stopLoading will stop all loads related to the data source.  This
// method will also stop loads that may be loading in child frames.
- (void)stopLoading
{
    [self _recursiveStopLoading];
}


// Returns YES if there are any pending loads.
- (BOOL)isLoading
{
    // FIXME: This comment says that the state check is just an optimization, but that's
    // not true. There's a window where the state is complete, but primaryLoadComplete
    // is still NO and loading is still YES, because _setPrimaryLoadComplete has not yet
    // been called. We should fix that and simplify this code here.
    
    // As an optimization, check to see if the frame is in the complete state.
    // If it is, we aren't loading, so we don't have to check all the child frames.    
    if ([[self webFrame] _state] == WebFrameStateComplete) {
        return NO;
    }
    
    if (!_private->primaryLoadComplete && _private->loading) {
        return YES;
    }
    if ([_private->subresourceClients count]) {
	return YES;
    }
    
    // Put in the auto-release pool because it's common to call this from a run loop source,
    // and then the entire list of frames lasts until the next autorelease.
    NSAutoreleasePool *pool = [NSAutoreleasePool new];
    NSEnumerator *e = [[[self webFrame] children] objectEnumerator];
    WebFrame *childFrame;
    while ((childFrame = [e nextObject])) {
        if ([[childFrame dataSource] isLoading] || [[childFrame provisionalDataSource] isLoading]) {
            break;
        }
    }
    [pool release];
    
    return childFrame != nil;
}


- (BOOL)isDocumentHTML
{
    return [[self representation] isKindOfClass: [WebHTMLRepresentation class]];
}

// Returns nil or the page title.
- (NSString *)pageTitle
{
    return _private->pageTitle;
}

- (NSString *)fileType
{
    return [[WebFileTypeMappings sharedMappings] preferredExtensionForMIMEType:[[self response] contentType]];
}

- (WebError *)mainDocumentError
{
    return _private->mainDocumentError;
}

- (NSString *)stringWithData:(NSData *)data
{
    NSString *textEncodingName = [self _overrideEncoding];

    if(!textEncodingName){
        textEncodingName = [[self response] textEncodingName];
    }

    if(textEncodingName){
        return [WebBridge stringWithData:data textEncodingName:textEncodingName];
    }else{
        return [WebBridge stringWithData:data textEncoding:kCFStringEncodingISOLatin1];
    }
}

+ (void)registerRepresentationClass:(Class)repClass forMIMEType:(NSString *)MIMEType
{
    // FIXME: OK to allow developers to override built-in reps?
    [[self _repTypes] setObject:repClass forKey:MIMEType];
}

- (BOOL)isDownloading
{
    return _private->isDownloading;
}

- (NSString *)downloadPath
{
    return [[_private->downloadPath retain] autorelease];
}

@end
