/*	
    IFPluginView.mm
	Copyright 2002, Apple, Inc. All rights reserved.
*/

#define USE_CARBON 1

#import "IFPluginView.h"

#import <WebKit/IFLoadProgress.h>
#import <WebKit/IFWebController.h>
#import <WebKit/IFWebFrame.h>

#import <AppKit/NSWindow_Private.h>
#import <Carbon/Carbon.h>

#import <WCPluginWidget.h>
#import <WebFoundation/IFURLHandle.h>
#import <IFPluginStream.h>
#import <IFWebDataSource.h>
#import <WebFoundation/IFError.h>
#import <WebKitDebug.h>

#import <WCPlugin.h>
#import <qwidget.h>
#import <IFWebView.h>
#import <IFBaseWebController.h>
#import <IFPluginNullEventSender.h>

extern "C" {
#import <CoreGraphics/CoreGraphics.h>
#import <CoreGraphics/CoreGraphicsPrivate.h>
}

@implementation IFPluginView

#pragma mark IFPLUGINVIEW

- initWithFrame:(NSRect)r plugin:(WCPlugin *)plug url:(NSString *)location mime:(NSString *)mimeType arguments:(NSDictionary *)arguments mode:(uint16)mode
{
    NPError npErr;
    char *cMime, *s;
    NPSavedData saved;
    NSArray *attributes, *values;
    NSString *attributeString, *baseURLString;
    uint i;
        
    [super initWithFrame: r];
    
    // The following line doesn't work for Flash, so I have create a NPP_t struct and point to it
    //instance = malloc(sizeof(NPP_t));
    instance = &instanceStruct;
    instance->ndata = self;

    mime = [mimeType retain];
    URL = [location retain];
    plugin = [plug retain];
    
    // load the plug-in if it is not already loaded
    [plugin load];
    
    // copy function pointers
    NPP_New = 		[plugin NPP_New];
    NPP_Destroy = 	[plugin NPP_Destroy];
    NPP_SetWindow = 	[plugin NPP_SetWindow];
    NPP_NewStream = 	[plugin NPP_NewStream];
    NPP_WriteReady = 	[plugin NPP_WriteReady];
    NPP_Write = 	[plugin NPP_Write];
    NPP_StreamAsFile = 	[plugin NPP_StreamAsFile];
    NPP_DestroyStream = [plugin NPP_DestroyStream];
    NPP_HandleEvent = 	[plugin NPP_HandleEvent];
    NPP_URLNotify = 	[plugin NPP_URLNotify];
    NPP_GetValue = 	[plugin NPP_GetValue];
    NPP_SetValue = 	[plugin NPP_SetValue];
    NPP_Print = 	[plugin NPP_Print]; 
    
    cMime = malloc([mime length]+1);
    [mime getCString:cMime];

    // get base URL which was added in the args in the part
    baseURLString = [arguments objectForKey:@"WebKitBaseURL"];
    if(baseURLString)
        baseURL = [[NSURL URLWithString:baseURLString] retain];
            
    attributes = [arguments allKeys];
    values = 	 [arguments allValues];
        
    if([attributes containsObject:@"hidden"])
        hidden = TRUE;
    else
        hidden = FALSE;
    
    if(![attributes containsObject:@"wkfullmode"]){
        // convert arugments dictionary to 2 string arrays
        
        cAttributes = malloc(sizeof(char *) * [arguments count]);
        cValues = malloc(sizeof(char *) * [arguments count]);
        
        for(i=0; i<[arguments count]; i++){ 
            attributeString = [attributes objectAtIndex:i];
            s = malloc([attributeString length]+1);
            [attributeString getCString:s];
            cAttributes[i] = s;
            
            attributeString = [values objectAtIndex:i];
            s = malloc([attributeString length]+1);
            [attributeString getCString:s];
            cValues[i] = s;
        }
        npErr = NPP_New(cMime, instance, NP_EMBED, [arguments count], cAttributes, cValues, &saved);
    }else{
        npErr = NPP_New(cMime, instance, NP_FULL, 0, NULL, NULL, &saved);
    }

    WEBKITDEBUG("NPP_New: %d\n", npErr);
    
    free(cMime);
    
    // Initialize globals
    transferred = FALSE;
    stopped = FALSE;
    filesToErase = [[NSMutableArray arrayWithCapacity:2] retain];
    activeURLHandles = [[NSMutableArray arrayWithCapacity:1] retain];
    
    // Create a WindowRef is one doesn't already exist
    [[self window] _windowRef];
    
    return self;
}

-(void)dealloc
{
    unsigned i;
    NSFileManager *fileManager;
    
    [self stop];
    
    // remove downloaded files
    fileManager = [NSFileManager defaultManager];
    for(i=0; i<[filesToErase count]; i++){  
        [fileManager removeFileAtPath:[filesToErase objectAtIndex:i] handler:nil]; 
    }
    
    [filesToErase release];
    [activeURLHandles release];
    [mime release];
    [URL release];
    [plugin release];
    free(cAttributes);
    free(cValues);
    [super dealloc];
}

- (void) setWindow
{
    NPError npErr;
    NSRect windowFrame, frameInWindow, visibleRectInWindow;
    
    windowFrame = [[self window] frame];
    frameInWindow = [self convertRect:[self bounds] toView:nil];
    visibleRectInWindow = [self convertRect:[self visibleRect] toView:nil];
    frameInWindow.origin.y =  windowFrame.size.height - frameInWindow.origin.y - frameInWindow.size.height; // flip y coord
    visibleRectInWindow.origin.y =  windowFrame.size.height - visibleRectInWindow.origin.y - visibleRectInWindow.size.height;
    
    nPort.port = GetWindowPort([[self window] _windowRef]);
    nPort.portx = -(int32)frameInWindow.origin.x; // these values are ignored by QT
    nPort.porty = -(int32)frameInWindow.origin.y;   
    window.window = &nPort;
    
    window.x = (int32)frameInWindow.origin.x; 
    window.y = (int32)frameInWindow.origin.y;

    window.width = (uint32)frameInWindow.size.width;
    window.height = (uint32)frameInWindow.size.height;

    window.clipRect.top = (uint16)visibleRectInWindow.origin.y; // clip rect
    window.clipRect.left = (uint16)visibleRectInWindow.origin.x;
    window.clipRect.bottom = (uint16)(visibleRectInWindow.origin.y + visibleRectInWindow.size.height);
    window.clipRect.right = (uint16)(visibleRectInWindow.origin.x + visibleRectInWindow.size.width);
    
    window.type = NPWindowTypeWindow;
    
    npErr = NPP_SetWindow(instance, &window);
    WEBKITDEBUG("NPP_SetWindow: %d, port=%d\n", npErr, (int)nPort.port);
}

- (void) newStream:(NSURL *)streamURL mimeType:(NSString *)mimeType notifyData:(void *)notifyData
{
    IFPluginStream *stream;
    NPStream *npStream;
    NPError npErr;    
    uint16 transferMode;
    IFURLHandle *urlHandle;
    NSDictionary *attributes;
    char *cMime;
    
    stream = [[IFPluginStream alloc] initWithURL:streamURL mimeType:mimeType notifyData:notifyData];
    npStream = [stream npStream];
    
    cMime = malloc([mimeType length]+1);
    [mimeType getCString:cMime];
    
    npErr = NPP_NewStream(instance, cMime, npStream, FALSE, &transferMode);
    WEBKITDEBUG("NPP_NewStream: %d\n", npErr);
    [stream setTransferMode:transferMode];
    free(cMime);
    
    if(transferMode == NP_NORMAL){
        WEBKITDEBUG("Stream type: NP_NORMAL\n");
    }else if(transferMode == NP_ASFILEONLY || transferMode == NP_ASFILE){
        if(transferMode == NP_ASFILEONLY)
            WEBKITDEBUG("Stream type: NP_ASFILEONLY\n");
        if(transferMode == NP_ASFILE)
            WEBKITDEBUG("Stream type: NP_ASFILE\n");
        [stream setFilename:[[streamURL path] lastPathComponent]];
    }else if(transferMode == NP_SEEK){
        WEBKITDEBUG("Stream type: NP_SEEK not yet supported\n");
        return;
    }
    attributes = [NSDictionary dictionaryWithObject:stream forKey:IFURLHandleUserData];
    urlHandle = [[IFURLHandle alloc] initWithURL:streamURL attributes:attributes flags:0];
    if(urlHandle!=nil){
        [urlHandle addClient:self];
        [activeURLHandles addObject:urlHandle];
        [urlHandle loadInBackground];
    }
}

-(void)start
{

}

- (void)stop
{
    NPError npErr;
    
    if (!stopped){
        [activeURLHandles makeObjectsPerformSelector:@selector(cancelLoadInBackground)];
        [eventSender stop];
        [eventSender release];
        [[NSNotificationCenter defaultCenter] removeObserver:self];
        [self removeTrackingRect:trackingTag];
        npErr = NPP_Destroy(instance, NULL);
        WEBKITDEBUG("NPP_Destroy: %d\n", npErr);
        stopped = TRUE;
    }
}

- (NSView *) findSuperview:(NSString *)viewName
{
    NSView *view;
    
    view = self;
    while(view){
        view = [view superview];
        if([[view className] isEqualToString:viewName]){
            return view;
        }
    }
    return nil;
}

#pragma mark EVENTS

-(void)sendActivateEvent:(BOOL)isActive;
{
    EventRecord event;
    bool acceptedEvent;
    UnsignedWide msecs;
    
    event.what = activateEvt;
    event.message = (uint32)GetWindowPort([[self window] _windowRef]);
    Microseconds(&msecs);
    event.when = (uint32)((double)UnsignedWideToUInt64(msecs) / 1000000 * 60); // microseconds to ticks
    event.modifiers = isActive;
    acceptedEvent = NPP_HandleEvent(instance, &event); 
    WEBKITDEBUG("NPP_HandleEvent(activateEvent): %d  isActive: %d\n", acceptedEvent, (event.modifiers & activeFlag));
}

-(void)sendUpdateEvent
{
    EventRecord event;
    bool acceptedEvent;
    UnsignedWide msecs;
    
    event.what = updateEvt;
    event.message = (uint32)GetWindowPort([[self window] _windowRef]);
    Microseconds(&msecs);
    event.when = (uint32)((double)UnsignedWideToUInt64(msecs) / 1000000 * 60); // microseconds to ticks
    acceptedEvent = NPP_HandleEvent(instance, &event); 
    WEBKITDEBUG("NPP_HandleEvent(updateEvt): %d  when: %lu\n", acceptedEvent, event.when);
}


#pragma mark NSVIEW

- (void)drawRect:(NSRect)rect
{
    if(!transferred){
        NSNotificationCenter *notificationCenter;
        NSWindow *theWindow;
        
        [self setWindow];
        theWindow = [self window];
        notificationCenter = [NSNotificationCenter defaultCenter];
        [notificationCenter addObserver:self selector:@selector(viewHasMoved:) name:@"NSViewBoundsDidChangeNotification" object:[self findSuperview:@"NSClipView"]];
        [notificationCenter addObserver:self selector:@selector(viewHasMoved:) name:@"NSWindowDidResizeNotification" object:theWindow];
        [notificationCenter addObserver:self selector:@selector(windowWillClose:) name:@"NSWindowWillCloseNotification" object:theWindow];
        [notificationCenter addObserver:self selector:@selector(windowBecameKey:) name:@"NSWindowDidBecomeKeyNotification" object:theWindow];
        [notificationCenter addObserver:self selector:@selector(windowResignedKey:) name:@"NSWindowDidResignKeyNotification" object:theWindow];
        [self sendActivateEvent:[theWindow isKeyWindow]];
        if(URL)
            [self newStream:[NSURL URLWithString:URL] mimeType:mime notifyData:NULL];
        eventSender = [[IFPluginNullEventSender alloc] initializeWithNPP:instance functionPointer:NPP_HandleEvent];
        [eventSender sendNullEvents];
        transferred = TRUE;
        trackingTag = [self addTrackingRect:[self bounds] owner:self userData:nil assumeInside:NO];
        
        id webView = [self findSuperview:@"IFWebView"];
        webController = [webView controller];
        webDataSource = [[webController frameForView:webView] dataSource];
    }
    [self sendUpdateEvent];
}

- (BOOL)isFlipped
{
    return YES;
}

#pragma mark NSRESPONDER

-(BOOL)acceptsFirstResponder
{
    return YES;
}

- (BOOL)becomeFirstResponder
{
    EventRecord event;
    bool acceptedEvent;
    UnsignedWide msecs;
    
    event.what = getFocusEvent;
    Microseconds(&msecs);
    event.when = (uint32)((double)UnsignedWideToUInt64(msecs) / 1000000 * 60); // microseconds to ticks
    acceptedEvent = NPP_HandleEvent(instance, &event); 
    WEBKITDEBUG("NPP_HandleEvent(getFocusEvent): %d  when: %lu\n", acceptedEvent, event.when);
    return YES;
}

- (BOOL)resignFirstResponder
{
    EventRecord event;
    bool acceptedEvent;
    UnsignedWide msecs;
    
    event.what = loseFocusEvent;
    Microseconds(&msecs);
    event.when = (uint32)((double)UnsignedWideToUInt64(msecs) / 1000000 * 60); // microseconds to ticks
    acceptedEvent = NPP_HandleEvent(instance, &event); 
    WEBKITDEBUG("NPP_HandleEvent(loseFocusEvent): %d  when: %lu\n", acceptedEvent, event.when);
    return YES;
}


-(void)mouseDown:(NSEvent *)theEvent
{
    EventRecord event;
    bool acceptedEvent;
    Point pt;
    CGPoint mousePoint = CGSCurrentInputPointerPosition();
    
    pt.v = (short)mousePoint.y;
    pt.h = (short)mousePoint.x;
    event.what = mouseDown;
    event.where = pt;
    event.when = (uint32)([theEvent timestamp] * 60); // seconds to ticks
    event.modifiers = 0;
    acceptedEvent = NPP_HandleEvent(instance, &event);
    WEBKITDEBUG("NPP_HandleEvent(mouseDown): %d pt.v=%d, pt.h=%d ticks=%lu\n", acceptedEvent, pt.v, pt.h, event.when);
}

-(void)mouseUp:(NSEvent *)theEvent
{
    EventRecord event;
    bool acceptedEvent;
    Point pt;
    CGPoint mousePoint = CGSCurrentInputPointerPosition();
    
    pt.v = (short)mousePoint.y;
    pt.h = (short)mousePoint.x;
    event.what = mouseUp;
    event.where = pt;
    event.when = (uint32)([theEvent timestamp] * 60); 
    event.modifiers = 0;
    acceptedEvent = NPP_HandleEvent(instance, &event);
    WEBKITDEBUG("NPP_HandleEvent(mouseUp): %d pt.v=%d, pt.h=%d ticks=%lu\n", acceptedEvent, pt.v, pt.h, event.when);
}

- (void)mouseEntered:(NSEvent *)theEvent
{
    EventRecord event;
    bool acceptedEvent;
    
    event.what = adjustCursorEvent;
    event.when = (uint32)([theEvent timestamp] * 60);
    event.modifiers = 1;
    acceptedEvent = NPP_HandleEvent(instance, &event);
    WEBKITDEBUG("NPP_HandleEvent(mouseEntered): %d\n", acceptedEvent);
}

- (void)mouseExited:(NSEvent *)theEvent
{
    EventRecord event;
    bool acceptedEvent;
     
    event.what = adjustCursorEvent;
    event.when = (uint32)([theEvent timestamp] * 60);
    event.modifiers = 0;
    acceptedEvent = NPP_HandleEvent(instance, &event);
    WEBKITDEBUG("NPP_HandleEvent(mouseExited): %d\n", acceptedEvent);
}

- (void)keyUp:(NSEvent *)theEvent
{
    EventRecord event;
    bool acceptedEvent;
    
    event.what = keyUp;
    event.message = [[theEvent charactersIgnoringModifiers] characterAtIndex:0];
    event.when = (uint32)([theEvent timestamp] * 60);
    acceptedEvent = NPP_HandleEvent(instance, &event);
    WEBKITDEBUG("NPP_HandleEvent(keyUp): %d key:%c\n", acceptedEvent, (char) (event.message & charCodeMask));
    //Note: QT Plug-in doesn't use keyUp's
}

- (void)keyDown:(NSEvent *)theEvent
{
    EventRecord event;
    bool acceptedEvent;
    
    event.what = keyDown;
    event.message = [[theEvent charactersIgnoringModifiers] characterAtIndex:0];
    event.when = (uint32)([theEvent timestamp] * 60);
    acceptedEvent = NPP_HandleEvent(instance, &event);
    WEBKITDEBUG("NPP_HandleEvent(keyDown): %d key:%c\n", acceptedEvent, (char) (event.message & charCodeMask));
}

#pragma mark NOTIFICATIONS

-(void) viewHasMoved:(NSNotification *)notification
{
    [self sendUpdateEvent];
    [self setWindow];
}

-(void) windowBecameKey:(NSNotification *)notification
{
    [self sendActivateEvent:TRUE];
}

-(void) windowResignedKey:(NSNotification *)notification
{
    [self sendActivateEvent:FALSE];
}

- (void) windowWillClose:(NSNotification *)notification
{
    [self stop];
}


#pragma mark IFURLHANDLE

- (void)IFURLHandleResourceDidBeginLoading:(IFURLHandle *)sender
{

}

- (void)IFURLHandle:(IFURLHandle *)sender resourceDataDidBecomeAvailable:(NSData *)data
{
    int32 bytes;
    IFPluginStream *stream;
    uint16 transferMode;
    NPStream *npStream;
    
    stream = [[sender attributes] objectForKey:IFURLHandleUserData];
    transferMode = [stream transferMode];
    npStream = [stream npStream];
    
    if(transferMode != NP_ASFILEONLY){
        bytes = NPP_WriteReady(instance, npStream);
        //WEBKITDEBUG("NPP_WriteReady bytes=%u\n", bytes);
        bytes = NPP_Write(instance, npStream, [stream offset], [data length], (void *)[data bytes]);
        //WEBKITDEBUG("NPP_Write bytes=%u\n", bytes);
        [stream incrementOffset:[data length]];
    }
    if(transferMode == NP_ASFILE || transferMode == NP_ASFILEONLY){
        [[stream data] appendData:data];
    }
        
    IFLoadProgress *loadProgress = [[IFLoadProgress alloc] init];
    loadProgress->totalToLoad = [sender contentLength];
    loadProgress->bytesSoFar = [sender contentLengthReceived];
    loadProgress->type = IF_LOAD_TYPE_PLUGIN;
    [webController receivedProgress: (IFLoadProgress *)loadProgress
        forResource: [[sender url] absoluteString] fromDataSource: webDataSource];
    [loadProgress release];
    
}

- (void)IFURLHandleResourceDidFinishLoading:(IFURLHandle *)sender data: (NSData *)data
{
    NPError npErr;
    char *cFilename;
    NSMutableString *filenameClassic, *path;
    IFPluginStream *stream;
    NSFileManager *fileManager;
    uint16 transferMode;
    NPStream *npStream;
    NSString *filename;
    
    stream = [[sender attributes] objectForKey:IFURLHandleUserData];
    transferMode = [stream transferMode];
    npStream = [stream npStream];
    filename = [stream filename];
    
    if(transferMode == NP_ASFILE || transferMode == NP_ASFILEONLY){
        path = [NSString stringWithFormat:@"%@%@", @"/tmp/", filename];        
        [filesToErase addObject:path];
        
        fileManager = [NSFileManager defaultManager];
        [fileManager removeFileAtPath:path handler:nil];
        [fileManager createFileAtPath:path contents:[stream data] attributes:nil];
        
        filenameClassic = [NSString stringWithFormat:@"%@%@%@", startupVolumeName(), @":private:tmp:", filename];        	cFilename = malloc([filenameClassic length]+1);
        [filenameClassic getCString:cFilename];
        
        NPP_StreamAsFile(instance, npStream, cFilename);
        WEBKITDEBUG("NPP_StreamAsFile: %s\n", cFilename);
    }
    npErr = NPP_DestroyStream(instance, npStream, NPRES_DONE);
    WEBKITDEBUG("NPP_DestroyStream: %d\n", npErr);
    
    if(npStream->notifyData){
        NPP_URLNotify(instance, npStream->url, NPRES_DONE, npStream->notifyData);
        WEBKITDEBUG("NPP_URLNotify\n");
    }
    [stream release];
    [activeURLHandles removeObject:sender];
    
    IFLoadProgress *loadProgress = [[IFLoadProgress alloc] init];
    loadProgress->totalToLoad = [data length];
    loadProgress->bytesSoFar = [data length];
    loadProgress->type = IF_LOAD_TYPE_PLUGIN;
    [webController receivedProgress: (IFLoadProgress *)loadProgress 
        forResource: [[sender url] absoluteString] fromDataSource: webDataSource];
    [loadProgress release];
}

- (void)IFURLHandleResourceDidCancelLoading:(IFURLHandle *)sender
{
    IFPluginStream *stream;
    
    stream = [[sender attributes] objectForKey:IFURLHandleUserData];
    [stream release];
    [self stop];
    
    IFLoadProgress *loadProgress = [[IFLoadProgress alloc] init];
    loadProgress->totalToLoad = -1;
    loadProgress->bytesSoFar = -1;
    loadProgress->type = IF_LOAD_TYPE_PLUGIN;
    [webController receivedProgress: (IFLoadProgress *)loadProgress 
        forResource: [[sender url] absoluteString] fromDataSource: webDataSource];
    [loadProgress release];
}

- (void)IFURLHandle:(IFURLHandle *)sender resourceDidFailLoadingWithResult:(int)result
{
    IFPluginStream *stream;
    
    stream = [[sender attributes] objectForKey:IFURLHandleUserData];
    [stream release];
    [self stop];
    
    IFLoadProgress *loadProgress = [[IFLoadProgress alloc] init];
    loadProgress->totalToLoad = [sender contentLength];
    loadProgress->bytesSoFar = [sender contentLengthReceived];
    loadProgress->type = IF_LOAD_TYPE_PLUGIN;
    
    IFError *error = [[IFError alloc] initWithErrorCode: result failingURL: [sender url]];
    [webController receivedError: error forResource: [[sender url] absoluteString] 
        partialProgress: loadProgress fromDataSource: webDataSource];
    [error release];
    [loadProgress release];
}

- (void)IFURLHandle:(IFURLHandle *)sender didRedirectToURL:(NSURL *)url
{
    
}

#pragma mark PLUGIN-TO-BROWSER

-(NPError)getURLNotify:(const char *)url target:(const char *)target notifyData:(void *)notifyData
{
    NSURL *newURL;
    IFWebDataSource *dataSource;
    NSURL *requestedURL;
    
    WEBKITDEBUG("NPN_GetURLNotify: %s target: %s\n", url, target);
 
    if(!strcmp(url, "")){
        return NPERR_INVALID_URL;
    }else if(strstr(url, "://")){ //check if this is an absolute URL
        requestedURL = [NSURL URLWithString:[NSString stringWithCString:url]];
    }else{
        requestedURL = [NSURL URLWithString:[NSString stringWithCString:url] relativeToURL:baseURL];
    }
    
    if(target == NULL){ // send data to plug-in if target is null
        [self newStream:requestedURL mimeType:[plugin mimeTypeForURL:[NSString stringWithCString:url]] notifyData:(void *)notifyData];
    }else if(!strcmp(target, "_self") || !strcmp(target, "_current") || !strcmp(target, "_parent") || !strcmp(target, "_top")){
        if(webController){
            newURL = [NSURL URLWithString:[NSString stringWithCString:url]];
            dataSource = [[[IFWebDataSource alloc] initWithURL:newURL] autorelease];
            [[webController mainFrame] setProvisionalDataSource:dataSource];
            [[webController mainFrame] startLoading];
        }
    }else if(!strcmp(target, "_blank") || !strcmp(target, "_new")){
        printf("Error: No API to open new browser window\n");
    }
    return NPERR_NO_ERROR;
}

-(NPError)getURL:(const char *)url target:(const char *)target
{
    WEBKITDEBUG("NPN_GetURL: %s target: %s\n", url, target);
    return [self getURLNotify:url target:target notifyData:NULL];
}

-(NPError)postURLNotify:(const char *)url target:(const char *)target len:(UInt32)len buf:(const char *)buf file:(NPBool)file notifyData:(void *)notifyData
{
    WEBKITDEBUG("NPN_PostURLNotify\n");
    return NPERR_GENERIC_ERROR;
}

-(NPError)postURL:(const char *)url target:(const char *)target len:(UInt32)len buf:(const char *)buf file:(NPBool)file
{
    WEBKITDEBUG("NPN_PostURL\n");
    return NPERR_GENERIC_ERROR;
}

-(NPError)newStream:(NPMIMEType)type target:(const char *)target stream:(NPStream**)stream
{
    WEBKITDEBUG("NPN_NewStream\n");
    return NPERR_GENERIC_ERROR;
}

-(NPError)write:(NPStream*)stream len:(SInt32)len buffer:(void *)buffer
{
    WEBKITDEBUG("NPN_Write\n");
    return NPERR_GENERIC_ERROR;
}

-(NPError)destroyStream:(NPStream*)stream reason:(NPReason)reason
{
    WEBKITDEBUG("NPN_DestroyStream\n");
    return NPERR_GENERIC_ERROR;
}

-(void)status:(const char *)message
{
    IFWebDataSource *dataSource;
    
    WEBKITDEBUG("NPN_Status: %s\n", message);
    if(webController){
        dataSource = [[webController mainFrame] dataSource];
        [webController setStatusText:[NSString stringWithCString:message] forDataSource:dataSource];
    }
}

-(NPError)getValue:(NPNVariable)variable value:(void *)value
{
    WEBKITDEBUG("NPN_GetValue\n");
    return NPERR_GENERIC_ERROR;
}

-(NPError)setValue:(NPPVariable)variable value:(void *)value
{
    WEBKITDEBUG("NPN_SetValue\n");
    return NPERR_GENERIC_ERROR;
}

-(void)invalidateRect:(NPRect *)invalidRect
{
    WEBKITDEBUG("NPN_InvalidateRect\n");
}

-(void)invalidateRegion:(NPRegion)invalidateRegion
{
    WEBKITDEBUG("NPN_InvalidateRegion\n");
}

-(void)forceRedraw
{
    WEBKITDEBUG("forceRedraw\n");
}

#pragma mark PREBINDING

static id IFPluginMake(NSRect rect, WCPlugin *plugin, NSString *url, NSString *mimeType, NSDictionary *arguments, uint16 mode) 
{
    return [[[IFPluginView alloc] initWithFrame:rect plugin:plugin url:url mime:mimeType arguments:arguments mode:mode] autorelease];
}

+(void) load
{
    WCSetIFPluginMakeFunc(IFPluginMake);
}

@end

NSString* startupVolumeName(void)
{
    NSString* rootName = nil;
    FSRef rootRef;
    if (FSPathMakeRef ((const UInt8 *) "/", & rootRef, NULL /*isDirectory*/) == noErr) {         
        HFSUniStr255  nameString;
        if (FSGetCatalogInfo (&rootRef, kFSCatInfoNone, NULL /*catalogInfo*/, &nameString, NULL /*fsSpec*/, NULL /*parentRef*/) == noErr) {
            rootName = [NSString stringWithCharacters:nameString.unicode length:nameString.length];
        }
    }
    return rootName;
}
