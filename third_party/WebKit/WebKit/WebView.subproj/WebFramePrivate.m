/*	
    WebFramePrivate.mm
	    
    Copyright 2001, 2002, Apple Computer, Inc. All rights reserved.
*/

#import <WebKit/WebFramePrivate.h>

#import <WebKit/WebDocument.h>
#import <WebKit/WebDynamicScrollBarsView.h>
#import <WebKit/WebHTMLView.h>
#import <WebKit/WebHTMLViewPrivate.h>
#import <WebKit/WebLocationChangeHandler.h>
#import <WebKit/WebPreferencesPrivate.h>
#import <WebKit/WebController.h>
#import <WebKit/WebControllerPrivate.h>
#import <WebKit/WebBridge.h>
#import <WebKit/WebFrameBridge.h>
#import <WebKit/WebDataSource.h>
#import <WebKit/WebDataSourcePrivate.h>
#import <WebKit/WebKitErrors.h>
#import <WebKit/WebViewPrivate.h>
#import <WebKit/WebKitDebug.h>

#import <WebFoundation/WebFoundation.h>

static const char * const stateNames[6] = {
    "zero state",
    "WebFrameStateUninitialized",
    "WebFrameStateProvisional",
    "WebFrameStateCommittedPage",
    "WebFrameStateLayoutAcceptable",
    "WebFrameStateComplete"
};

@implementation WebFramePrivate

- (void)dealloc
{
    [webView _setController: nil];
    [dataSource _setController: nil];
    [dataSource _setLocationChangeHandler: nil];
    [provisionalDataSource _setController: nil];
    [provisionalDataSource _setLocationChangeHandler: nil];

    [name autorelease];
    [webView autorelease];
    [dataSource autorelease];
    [provisionalDataSource autorelease];
    [frameBridge release];
    
    [super dealloc];
}

- (NSString *)name { return name; }
- (void)setName: (NSString *)n 
{
    [name autorelease];
    name = [n retain];
}

- (WebView *)webView { return webView; }
- (void)setWebView: (WebView *)v 
{ 
    [webView autorelease];
    webView = [v retain];
}

- (WebDataSource *)dataSource { return dataSource; }
- (void)setDataSource: (WebDataSource *)d
{
    if (dataSource != d) {
        [dataSource _removeFromFrame];
        [dataSource autorelease];
        dataSource = [d retain];
    }
}

- (WebController *)controller { return controller; }
- (void)setController: (WebController *)c
{ 
    controller = c; // not retained (yet)
}

- (WebDataSource *)provisionalDataSource { return provisionalDataSource; }
- (void)setProvisionalDataSource: (WebDataSource *)d
{ 
    if (provisionalDataSource != d) {
        [provisionalDataSource autorelease];
        provisionalDataSource = [d retain];
    }
}

@end

@implementation WebFrame (WebPrivate)

- (void)_parentDataSourceWillBeDeallocated
{
    [_private setController:nil];
    [_private->dataSource _setParent:nil];
    [_private->provisionalDataSource _setParent:nil];
}

- (void)_setController: (WebController *)controller
{
    [_private setController: controller];
}

- (void)_setDataSource: (WebDataSource *)ds
{
    [_private setDataSource: ds];
    [ds _setController: [self controller]];
}

- (void)_scheduleLayout: (NSTimeInterval)inSeconds
{
    if (_private->scheduledLayoutPending == NO) {
        [NSTimer scheduledTimerWithTimeInterval:inSeconds target:self selector: @selector(_timedLayout:) userInfo: nil repeats:FALSE];
        _private->scheduledLayoutPending = YES;
    }
}

- (void)_timedLayout: (id)userInfo
{
    WEBKITDEBUGLEVEL (WEBKIT_LOG_TIMING, "%s:  state = %s\n", [[self name] cString], stateNames[_private->state]);
    
    _private->scheduledLayoutPending = NO;
    if (_private->state == WebFrameStateLayoutAcceptable) {
        NSView <WebDocumentView> *documentView = [[self webView] documentView];
        
        if ([self controller])
            WEBKITDEBUGLEVEL (WEBKIT_LOG_TIMING, "%s:  performing timed layout, %f seconds since start of document load\n", [[self name] cString], CFAbsoluteTimeGetCurrent() - [[[[self controller] mainFrame] dataSource] _loadingStartedTime]);
            
        if ([[self webView] isDocumentHTML]) {
            WebHTMLView *htmlView = (WebHTMLView *)documentView;
            
            [htmlView setNeedsLayout: YES];
            
            NSRect frame = [htmlView frame];
            
            if (frame.size.width == 0 || frame.size.height == 0){
                // We must do the layout now, rather than depend on
                // display to do a lazy layout because the view
                // may be recently initialized with a zero size
                // and the AppKit will optimize out any drawing.
                
                // Force a layout now.  At this point we could
                // check to see if any CSS is pending and delay
                // the layout further to avoid the flash of unstyled
                // content.                    
                [htmlView layout];
            }
        }
            
        [documentView setNeedsDisplay: YES];
    }
    else {
        if ([self controller])
            WEBKITDEBUGLEVEL (WEBKIT_LOG_TIMING, "%s:  NOT performing timed layout (not needed), %f seconds since start of document load\n", [[self name] cString], CFAbsoluteTimeGetCurrent() - [[[[self controller] mainFrame] dataSource] _loadingStartedTime]);
    }
}


- (void)_transitionProvisionalToLayoutAcceptable
{
    switch ([self _state]) {
    	case WebFrameStateCommittedPage:
        {
            [self _setState: WebFrameStateLayoutAcceptable];
                    
            // Start a timer to guarantee that we get an initial layout after
            // X interval, even if the document and resources are not completely
            // loaded.
            BOOL timedDelayEnabled = [[WebPreferences standardPreferences] _initialTimedLayoutEnabled];
            if (timedDelayEnabled) {
                NSTimeInterval defaultTimedDelay = [[WebPreferences standardPreferences] _initialTimedLayoutDelay];
                double timeSinceStart;

                // If the delay getting to the commited state exceeds the initial layout delay, go
                // ahead and schedule a layout.
                timeSinceStart = (CFAbsoluteTimeGetCurrent() - [[self dataSource] _loadingStartedTime]);
                if (timeSinceStart > (double)defaultTimedDelay) {
                    WEBKITDEBUGLEVEL (WEBKIT_LOG_TIMING, "performing early layout because commit time, %f, exceeded initial layout interval %f\n", timeSinceStart, defaultTimedDelay);
                    [self _timedLayout: nil];
                }
                else {
                    NSTimeInterval timedDelay = defaultTimedDelay - timeSinceStart;
                    
                    WEBKITDEBUGLEVEL (WEBKIT_LOG_TIMING, "registering delayed layout after %f seconds, time since start %f\n", timedDelay, timeSinceStart);
                    [self _scheduleLayout: timedDelay];
                }
            }
            break;
        }

        case WebFrameStateProvisional:
        case WebFrameStateComplete:
        case WebFrameStateLayoutAcceptable:
        {
            break;
        }
        
        case WebFrameStateUninitialized:
        default:
        {
            [[NSException exceptionWithName:NSGenericException reason: [NSString stringWithFormat: @"invalid state attempting to transition to WebFrameStateLayoutAcceptable from %s", stateNames[_private->state]] userInfo: nil] raise];
            return;
        }
    }
}


- (void)_transitionProvisionalToCommitted
{
    WEBKIT_ASSERT ([self controller] != nil);
    NSView <WebDocumentView> *documentView = [[self webView] documentView];
    
    switch ([self _state]) {
    	case WebFrameStateProvisional:
        {
            WEBKIT_ASSERT (documentView != nil);

            // Set the committed data source on the frame.
            [self _setDataSource: _private->provisionalDataSource];

            // provisionalDataSourceCommitted: will reset the view and begin trying to
            // display the new new datasource.
            [documentView provisionalDataSourceCommitted: _private->provisionalDataSource];

            // Now that the provisional data source is committed, release it.
            [_private setProvisionalDataSource: nil];
        
            [self _setState: WebFrameStateCommittedPage];
        
            [[[self dataSource] _locationChangeHandler] locationChangeCommittedForDataSource:[self dataSource]];
            
            // If we have a title let the controller know about it.
            if ([[self dataSource] pageTitle])
                [[[self dataSource] _locationChangeHandler] receivedPageTitle:[[self dataSource] pageTitle] forDataSource:[self dataSource]];

            break;
        }
        
        case WebFrameStateUninitialized:
        case WebFrameStateCommittedPage:
        case WebFrameStateLayoutAcceptable:
        case WebFrameStateComplete:
        default:
        {
            [[NSException exceptionWithName:NSGenericException reason:[NSString stringWithFormat: @"invalid state attempting to transition to WebFrameStateCommittedPage from %s", stateNames[_private->state]] userInfo: nil] raise];
            return;
        }
    }
}

- (WebFrameState)_state
{
    return _private->state;
}

- (void)_setState: (WebFrameState)newState
{
    WEBKITDEBUGLEVEL (WEBKIT_LOG_LOADING, "%s:  transition from %s to %s\n", [[self name] cString], stateNames[_private->state], stateNames[newState]);
    if ([self controller])
        WEBKITDEBUGLEVEL (WEBKIT_LOG_TIMING, "%s:  transition from %s to %s, %f seconds since start of document load\n", [[self name] cString], stateNames[_private->state], stateNames[newState], CFAbsoluteTimeGetCurrent() - [[[[self controller] mainFrame] dataSource] _loadingStartedTime]);
    
    if (newState == WebFrameStateComplete && self == [[self controller] mainFrame]){
        WEBKITDEBUGLEVEL (WEBKIT_LOG_DOCUMENTLOAD, "completed %s (%f seconds)", [[[[self dataSource] inputURL] absoluteString] cString], CFAbsoluteTimeGetCurrent() - [[self dataSource] _loadingStartedTime]);
    }
    
    NSDictionary *userInfo = [NSDictionary dictionaryWithObjectsAndKeys:
                    [NSNumber numberWithInt:_private->state], WebPreviousFrameState,
                    [NSNumber numberWithInt:newState], WebCurrentFrameState, nil];
                    
    [[NSNotificationCenter defaultCenter] postNotificationName:WebFrameStateChangedNotification object:self userInfo:userInfo];
    
    _private->state = newState;
    
    if (_private->state == WebFrameStateProvisional){
        [[[self webView] frameScrollView] setDrawsBackground: NO];
    }
    
    if (_private->state == WebFrameStateComplete){
        NSScrollView *sv = [[self webView] frameScrollView];
        [sv setDrawsBackground: YES];
        [[sv contentView] setCopiesOnScroll: YES];
    }
}

- (void)_isLoadComplete
{
    WEBKIT_ASSERT ([self controller] != nil);

    switch ([self _state]) {
        // Shouldn't ever be in this state.
        case WebFrameStateUninitialized:
        {
            [[NSException exceptionWithName:NSGenericException reason:@"invalid state WebFrameStateUninitialized during completion check with error" userInfo: nil] raise];
            return;
        }
        
        case WebFrameStateProvisional:
        {
            WebDataSource *pd = [self provisionalDataSource];
            
            WEBKITDEBUGLEVEL (WEBKIT_LOG_LOADING, "%s:  checking complete in WebFrameStateProvisional\n", [[self name] cString]);
            // If we've received any errors we may be stuck in the provisional state and actually
            // complete.
            if ([[pd errors] count] != 0 || [pd mainDocumentError]) {
                // Check all children first.
                WEBKITDEBUGLEVEL (WEBKIT_LOG_LOADING, "%s:  checking complete, current state WebFrameStateProvisional, %d errors\n", [[self name] cString], [[pd errors] count]);
                if (![pd isLoading]) {
                    WEBKITDEBUGLEVEL (WEBKIT_LOG_LOADING, "%s:  checking complete in WebFrameStateProvisional, load done\n", [[self name] cString]);

                    [[pd _locationChangeHandler] locationChangeDone: [pd mainDocumentError] forDataSource:pd];

                    // We now the provisional data source didn't cut the mustard, release it.
                    [_private setProvisionalDataSource: nil];
                    
                    [self _setState: WebFrameStateComplete];
                    return;
                }
            }
            return;
        }
        
        case WebFrameStateCommittedPage:
        case WebFrameStateLayoutAcceptable:
        {
            WebDataSource *ds = [self dataSource];
            
            //WEBKITDEBUGLEVEL (WEBKIT_LOG_LOADING, "%s:  checking complete, current state WEBFRAMESTATE_COMMITTED\n", [[self name] cString]);
            if (![ds isLoading]) {
                id thisView = [self webView];
                NSView <WebDocumentView> *thisDocumentView = [thisView documentView];

                [self _setState: WebFrameStateComplete];
                
                [[ds _bridge] end];

                // Unfortunately we have to get our parent to adjust the frames in this
                // frameset so this frame's geometry is set correctly.  This should
                // be a reasonably inexpensive operation.
                id parentDS = [[[ds parent] webFrame] dataSource];
                if ([[parentDS _bridge] isFrameSet]){
                    id parentWebView = [[[ds parent] webFrame] webView];
                    if ([parentWebView isDocumentHTML])
                        [[parentWebView documentView] _adjustFrames];
                }

                // Tell the just loaded document to layout.  This may be necessary
                // for non-html content that needs a layout message.
                if ([thisView isDocumentHTML]){
                    WebHTMLView *hview = (WebHTMLView *)thisDocumentView;
                    [hview setNeedsLayout: YES];
                }
                [thisDocumentView layout];

                // Unfortunately if this frame has children we have to lay them
                // out too.  This could be an expensive operation.
                // FIXME:  If we can figure out how to avoid the layout of children,
                // (just need for iframe placement/sizing) we could get a few percent
                // speed improvement.
                [ds _layoutChildren];

                [thisDocumentView setNeedsDisplay: YES];
                //[thisDocumentView display];

                // Jump to anchor point, if necessary.
                [[ds _bridge] scrollToBaseAnchor];

                [[ds _locationChangeHandler] locationChangeDone: [ds mainDocumentError] forDataSource:ds];
 
                //if ([ds isDocumentHTML])
                //    [[ds representation] part]->closeURL();        
               
                return;
            }
            // A resource was loaded, but the entire frame isn't complete.  Schedule a
            // layout.
            else {
                if ([self _state] == WebFrameStateLayoutAcceptable) {
                    BOOL resourceTimedDelayEnabled = [[WebPreferences standardPreferences] _resourceTimedLayoutEnabled];
                    if (resourceTimedDelayEnabled) {
                        NSTimeInterval timedDelay = [[WebPreferences standardPreferences] _resourceTimedLayoutDelay];
                        [self _scheduleLayout: timedDelay];
                    }
                }
            }
            return;
        }
        
        case WebFrameStateComplete:
        {
            WEBKITDEBUGLEVEL (WEBKIT_LOG_LOADING, "%s:  checking complete, current state WebFrameStateComplete\n", [[self name] cString]);
            return;
        }
        
        // Yikes!  Serious horkage.
        default:
        {
            [[NSException exceptionWithName:NSGenericException reason:@"invalid state during completion check with error" userInfo: nil] raise];
            break;
        }
    }
}

+ (void)_recursiveCheckCompleteFromFrame: (WebFrame *)fromFrame
{
    int i, count;
    NSArray *childFrames;
    
    childFrames = [[fromFrame dataSource] children];
    count = [childFrames count];
    for (i = 0; i < count; i++) {
        WebFrame *childFrame;
        
        childFrame = [childFrames objectAtIndex: i];
        [WebFrame _recursiveCheckCompleteFromFrame: childFrame];
        [childFrame _isLoadComplete];
    }
    [fromFrame _isLoadComplete];
}

// Called every time a resource is completely loaded, or an error is received.
- (void)_checkLoadComplete
{

    WEBKIT_ASSERT ([self controller] != nil);

    // Now walk the frame tree to see if any frame that may have initiated a load is done.
    [WebFrame _recursiveCheckCompleteFromFrame: [[self controller] mainFrame]];
}

- (WebFrameBridge *)_frameBridge
{
    return _private->frameBridge;
}

- (BOOL)_shouldShowDataSource:(WebDataSource *)dataSource
{
    id <WebControllerPolicyHandler> policyHandler = [[self controller] policyHandler];
    NSFileManager *fileManager = [NSFileManager defaultManager];
    NSWorkspace *workspace = [NSWorkspace sharedWorkspace];
    NSURL *url = [dataSource inputURL];
    WebFileURLPolicy fileURLPolicy;
    NSString *path = [url path];
    BOOL isDirectory, fileExists;
    WebError *error;
    
    WebURLPolicy urlPolicy = [policyHandler URLPolicyForURL:url];
    
    if(urlPolicy == WebURLPolicyUseContentPolicy){
                    
        if([url isFileURL]){
        
            fileExists = [fileManager fileExistsAtPath:path isDirectory:&isDirectory];
            
            NSString *type = [WebController _MIMETypeForFile: path];
                
            if(isDirectory){
                fileURLPolicy = [policyHandler fileURLPolicyForMIMEType: nil dataSource: dataSource isDirectory:YES];
            }else{
                fileURLPolicy = [policyHandler fileURLPolicyForMIMEType: type dataSource: dataSource isDirectory:NO];
            }
            
            if(fileURLPolicy == WebFileURLPolicyIgnore)
                return NO;
            
            if(!fileExists){
                error = [[WebError alloc] initWithErrorCode:WebErrorFileDoesNotExist 
                            inDomain:WebErrorDomainWebKit failingURL:url];
                [policyHandler unableToImplementFileURLPolicy: error forDataSource: dataSource];
                return NO;
            }
            
            if(![fileManager isReadableFileAtPath:path]){
                error = [[WebError alloc] initWithErrorCode:WebErrorFileNotReadable 
                            inDomain:WebErrorDomainWebKit failingURL:url];
                [policyHandler unableToImplementFileURLPolicy: error forDataSource: dataSource];
                return NO;
            }
            
            if(fileURLPolicy == WebFileURLPolicyUseContentPolicy){
                if(isDirectory){
                    error = [[WebError alloc] initWithErrorCode:WebErrorCannotShowDirectory 
                                inDomain:WebErrorDomainWebKit failingURL: url];
                    [policyHandler unableToImplementFileURLPolicy: error forDataSource: dataSource];
                    return NO;
                }
                else if(![WebController canShowMIMEType: type]){
                    error = [[WebError alloc] initWithErrorCode:WebErrorCannotShowMIMEType 
                                inDomain:WebErrorDomainWebKit failingURL: url];
                    [policyHandler unableToImplementFileURLPolicy: error forDataSource: dataSource];
                    return NO;
                }else{
                    // File exists, its readable, we can show it
                    return YES;
                }
            }else if(fileURLPolicy == WebFileURLPolicyOpenExternally){
                if(![workspace openFile:path]){
                    error = [[WebError alloc] initWithErrorCode:WebErrorCouldNotFindApplicationForFile 
                                inDomain:WebErrorDomainWebKit failingURL: url];
                    [policyHandler unableToImplementFileURLPolicy: error forDataSource: dataSource];
                }
                return NO;
            }else if(fileURLPolicy == WebFileURLPolicyReveal){
                if(![workspace selectFile:path inFileViewerRootedAtPath:@""]){
                        error = [[WebError alloc] initWithErrorCode:WebErrorFinderCouldNotOpenDirectory 
                                    inDomain:WebErrorDomainWebKit failingURL: url];
                        [policyHandler unableToImplementFileURLPolicy: error forDataSource: dataSource];
                    }
                return NO;
            }else{
                [NSException raise:NSInvalidArgumentException format:
                    @"fileURLPolicyForMIMEType:dataSource:isDirectory: returned an invalid WebFileURLPolicy"];
                return NO;
            }
        }else{
            if(![WebResourceHandle canInitWithURL:url]){
            	error = [[WebError alloc] initWithErrorCode:WebErrorCannotShowURL 
                        inDomain:WebErrorDomainWebKit failingURL: url];
                [policyHandler unableToImplementURLPolicyForURL: url error: error];
                return NO;
            }
            // we can handle this URL
            return YES;
        }
    }
    else if(urlPolicy == WebURLPolicyOpenExternally){
        if(![workspace openURL:url]){
            error = [[WebError alloc] initWithErrorCode:WebErrorCouldNotFindApplicationForURL 
                        inDomain:WebErrorDomainWebKit failingURL: url];
            [policyHandler unableToImplementURLPolicyForURL: url error: error];
        }
        return NO;
    }
    else if(urlPolicy == WebURLPolicyIgnore){
        return NO;
    }
    else{
        [NSException raise:NSInvalidArgumentException format:@"URLPolicyForURL: returned an invalid WebURLPolicy"];
        return NO;
    }
}

- (void)_setProvisionalDataSource:(WebDataSource *)d
{
    [_private setProvisionalDataSource:d];
}

@end
