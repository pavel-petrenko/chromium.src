/*	
    WebBaseResourceHandleDelegate.m
    Copyright (c) 2002, Apple Computer, Inc. All rights reserved.
*/

#import <WebKit/WebBaseResourceHandleDelegate.h>

#import <WebFoundation/WebAssertions.h>
#import <WebFoundation/WebError.h>
#import <WebFoundation/WebHTTPResourceRequest.h>
#import <WebFoundation/WebResourceHandlePrivate.h>
#import <WebFoundation/WebResourceRequest.h>
#import <WebFoundation/WebResourceResponse.h>

#import <WebKit/WebController.h>
#import <WebKit/WebDataSourcePrivate.h>
#import <WebKit/WebKitErrors.h>
#import <WebKit/WebResourceLoadDelegate.h>
#import <WebKit/WebStandardPanelsPrivate.h>

@implementation WebBaseResourceHandleDelegate

- (void)_releaseResources
{
    // It's possible that when we release the handle, it will be
    // deallocated and release the last reference to this object.
    // We need to retain to avoid accessing the object after it
    // has been deallocated and also to avoid reentering this method.
    
    [self retain];
    
    [identifier release];
    identifier = nil;

    [handle release];
    handle = nil;

    [controller release];
    controller = nil;
    
    [dataSource release];
    dataSource = nil;
    
    [resourceLoadDelegate release];
    resourceLoadDelegate = nil;

    [downloadDelegate release];
    downloadDelegate = nil;
    
    reachedTerminalState = YES;
    
    [self release];
}

- (void)dealloc
{
    [self _releaseResources];
    [request release];
    [response release];
    [currentURL release];
    [super dealloc];
}

- (void)startLoading:(WebResourceRequest *)r
{
    [handle loadWithDelegate:self];
}

- (BOOL)loadWithRequest:(WebResourceRequest *)r
{
    ASSERT(handle == nil);
    
    handle = [[WebResourceHandle alloc] initWithRequest:r];
    if (!handle) {
        return NO;
    }
    if (defersCallbacks) {
        [handle _setDefersCallbacks:YES];
    }

    [self startLoading:r];

    return YES;
}

- (void)setDefersCallbacks:(BOOL)defers
{
    defersCallbacks = defers;
    [handle _setDefersCallbacks:defers];
}

- (BOOL)defersCallbacks
{
    return defersCallbacks;
}

- (void)setDataSource:(WebDataSource *)d
{
    ASSERT(d);
    ASSERT([d controller]);
    
    [d retain];
    [dataSource release];
    dataSource = d;

    [controller release];
    controller = [[dataSource controller] retain];
    
    [resourceLoadDelegate release];
    resourceLoadDelegate = [[controller resourceLoadDelegate] retain];

    [downloadDelegate release];
    downloadDelegate = [[controller downloadDelegate] retain];
}

- (WebDataSource *)dataSource
{
    return dataSource;
}

- (id <WebResourceLoadDelegate>)resourceLoadDelegate
{
    return resourceLoadDelegate;
}

- (id <WebResourceLoadDelegate>)downloadDelegate
{
    return downloadDelegate;
}

- (BOOL)isDownload
{
    return NO;
}

-(WebResourceRequest *)handle:(WebResourceHandle *)h willSendRequest:(WebResourceRequest *)newRequest
{
    ASSERT(handle == h);
    ASSERT(!reachedTerminalState);
    
    [newRequest setUserAgent:[controller userAgentForURL:[newRequest URL]]];

    if (identifier == nil) {
        // The identifier is released after the last callback, rather than in dealloc
        // to avoid potential cycles.
        identifier = [[resourceLoadDelegate identifierForInitialRequest:newRequest fromDataSource:dataSource] retain];
    }

    if (resourceLoadDelegate) {
        newRequest = [resourceLoadDelegate resource:identifier willSendRequest:newRequest fromDataSource:dataSource];
    }

    // Store a copy of the request.
    [request autorelease];

    if (currentURL) {
        [[WebStandardPanels sharedStandardPanels] _didStopLoadingURL:currentURL inController:controller];
        [currentURL release];
        currentURL = nil;
    }
    
    // Client may return a nil request, indicating that the request should be aborted.
    if (newRequest){
        request = [newRequest copy];
        currentURL = [[request URL] retain];
        if (currentURL)
            [[WebStandardPanels sharedStandardPanels] _didStartLoadingURL:currentURL inController:controller];
    }
    else {
        request = nil;
    }
    
    return request;
}

-(void)handle:(WebResourceHandle *)h didReceiveResponse:(WebResourceResponse *)r
{
    ASSERT(handle == h);
    ASSERT(!reachedTerminalState);

    [r retain];
    [response release];
    response = r;

    if ([self isDownload])
        [downloadDelegate resource:identifier didReceiveResponse:r fromDataSource:dataSource];
    else {
        [dataSource _addResponse: r];
        [resourceLoadDelegate resource:identifier didReceiveResponse:r fromDataSource:dataSource];
    }
}

- (void)handle:(WebResourceHandle *)h didReceiveData:(NSData *)data
{
    ASSERT(handle == h);
    ASSERT(!reachedTerminalState);

    if ([self isDownload])
        [downloadDelegate resource:identifier didReceiveContentLength:[data length] fromDataSource:dataSource];
    else
        [resourceLoadDelegate resource:identifier didReceiveContentLength:[data length] fromDataSource:dataSource];
}

- (void)handleDidFinishLoading:(WebResourceHandle *)h
{
    ASSERT(handle == h);
    ASSERT(!reachedTerminalState);

    if ([self isDownload])
        [downloadDelegate resource:identifier didFinishLoadingFromDataSource:dataSource];
    else
        [resourceLoadDelegate resource:identifier didFinishLoadingFromDataSource:dataSource];

    ASSERT(currentURL);
    [[WebStandardPanels sharedStandardPanels] _didStopLoadingURL:currentURL inController:controller];

    [self _releaseResources];
}

- (void)handle:(WebResourceHandle *)h didFailLoadingWithError:(WebError *)result
{
    ASSERT(handle == h);
    ASSERT(!reachedTerminalState);
    
    if ([self isDownload])
        [downloadDelegate resource:identifier didFailLoadingWithError:result fromDataSource:dataSource];
    else
        [resourceLoadDelegate resource:identifier didFailLoadingWithError:result fromDataSource:dataSource];

    // currentURL may be nil if the request was aborted
    if (currentURL)
        [[WebStandardPanels sharedStandardPanels] _didStopLoadingURL:currentURL inController:controller];

    [self _releaseResources];
}

- (void)cancelWithError:(WebError *)error
{
    ASSERT(!reachedTerminalState);

    [handle cancel];
    
    // currentURL may be nil if the request was aborted
    if (currentURL)
        [[WebStandardPanels sharedStandardPanels] _didStopLoadingURL:currentURL inController:controller];

    if (error) {
        if ([self isDownload]) {
            [downloadDelegate resource:identifier didFailLoadingWithError:error fromDataSource:dataSource];
        } else {
            [resourceLoadDelegate resource:identifier didFailLoadingWithError:error fromDataSource:dataSource];
        }
    }

    [self _releaseResources];
}

- (void)cancel
{
    [self cancelWithError:[self cancelledError]];
}

- (void)cancelQuietly
{
    [self cancelWithError:nil];
}

- (WebError *)cancelledError
{
    return [WebError errorWithCode:WebErrorCodeCancelled
                          inDomain:WebErrorDomainWebFoundation
                        failingURL:[[request URL] absoluteString]];
}

- (void)notifyDelegatesOfInterruptionByPolicyChange
{
    WebError *error = [WebError errorWithCode:WebErrorResourceLoadInterruptedByPolicyChange
                                     inDomain:WebErrorDomainWebKit
                                   failingURL:nil];
    
    [[self resourceLoadDelegate] resource:identifier
                  didFailLoadingWithError:error
                           fromDataSource:dataSource];
}

- (void)setIdentifier: ident
{
    if (identifier != ident){
        [identifier release];
        identifier = [ident retain];
    }
}

@end
