/*	
    IFPluginView.mm
	Copyright 2002, Apple, Inc. All rights reserved.
*/

#define USE_CARBON 1

// FIXME: clean up these includes

#import "IFPluginView.h"

#import <WebKit/IFWebController.h>
#import <WebKit/IFWebFrame.h>
#import <WebKit/IFWebFramePrivate.h>

#import <AppKit/NSWindow_Private.h>
#import <Carbon/Carbon.h>

#import <IFPluginDatabase.h>
#import <IFPluginStream.h>
#import <IFWebDataSource.h>
#import <WebFoundation/IFError.h>
#import <WebKitDebug.h>

#import <IFPlugin.h>
#import <qwidget.h>
#import <IFWebView.h>
#import <IFBaseWebController.h>
#import <IFPluginNullEventSender.h>
#import "IFNullPluginView.h"

#import <WebFoundation/IFNSStringExtensions.h>

@implementation IFPluginView

#pragma mark EVENTS

+ (void)getCarbonEvent:(EventRecord *)carbonEvent
{
    carbonEvent->what = nullEvent;
    carbonEvent->message = 0;
    carbonEvent->when = TickCount();
    GetGlobalMouse(&carbonEvent->where);
    carbonEvent->modifiers = GetCurrentKeyModifiers();
    if (!Button())
        carbonEvent->modifiers |= btnState;
}

- (void)getCarbonEvent:(EventRecord *)carbonEvent
{
    [[self class] getCarbonEvent:carbonEvent];
}

- (EventModifiers)modifiersForEvent:(NSEvent *)event
{
    EventModifiers modifiers;
    unsigned int modifierFlags = [event modifierFlags];
    NSEventType eventType = [event type];
    
    modifiers = 0;
    
    if (eventType != NSLeftMouseDown && eventType != NSRightMouseDown)
        modifiers |= btnState;
    
    if (modifierFlags & NSCommandKeyMask)
        modifiers |= cmdKey;
    
    if (modifierFlags & NSShiftKeyMask)
        modifiers |= shiftKey;

    if (modifierFlags & NSAlphaShiftKeyMask)
        modifiers |= alphaLock;

    if (modifierFlags & NSAlternateKeyMask)
        modifiers |= optionKey;

    if (modifierFlags & NSControlKeyMask || eventType == NSRightMouseDown)
        modifiers |= controlKey;

    return modifiers;
}

- (void)getCarbonEvent:(EventRecord *)carbonEvent withEvent:(NSEvent *)cocoaEvent
{
    NSPoint where;
    
    where = [[cocoaEvent window] convertBaseToScreen:[cocoaEvent locationInWindow]];
    
    carbonEvent->what = nullEvent;
    carbonEvent->message = 0;
    carbonEvent->when = (UInt32)([cocoaEvent timestamp] * 60); // seconds to ticks
    carbonEvent->where.h = (short)where.x;
    carbonEvent->where.v = (short)(NSMaxY([[[NSScreen screens] objectAtIndex:0] frame]) - where.y);
    carbonEvent->modifiers = [self modifiersForEvent:cocoaEvent];    
}

-(void)sendActivateEvent:(BOOL)activate
{
    EventRecord event;
    bool acceptedEvent;
    
    [self getCarbonEvent:&event];
    event.what = activateEvt;
    event.message = (UInt32)[[self window] _windowRef];
    if (activate)
        event.modifiers |= activMask;
    
    acceptedEvent = NPP_HandleEvent(instance, &event); 
    
    WEBKITDEBUGLEVEL(WEBKIT_LOG_PLUGINS, "NPP_HandleEvent(activateEvent): %d  isActive: %d\n", acceptedEvent, (event.modifiers & activeFlag));
}

- (void)sendUpdateEvent
{
    EventRecord event;
    bool acceptedEvent;
    
    [self getCarbonEvent:&event];
    event.what = updateEvt;
    event.message = (UInt32)[[self window] _windowRef];

    acceptedEvent = NPP_HandleEvent(instance, &event); 
    
    WEBKITDEBUGLEVEL(WEBKIT_LOG_PLUGINS, "NPP_HandleEvent(updateEvt): %d\n", acceptedEvent);
}

-(BOOL)acceptsFirstResponder
{
    return YES;
}

- (BOOL)becomeFirstResponder
{
    EventRecord event;
    bool acceptedEvent;
    
    [self getCarbonEvent:&event];
    event.what = getFocusEvent;
    
    acceptedEvent = NPP_HandleEvent(instance, &event); 
    
    WEBKITDEBUGLEVEL(WEBKIT_LOG_PLUGINS, "NPP_HandleEvent(getFocusEvent): %d\n", acceptedEvent);
    return YES;
}

- (BOOL)resignFirstResponder
{
    EventRecord event;
    bool acceptedEvent;
    
    [self getCarbonEvent:&event];
    event.what = loseFocusEvent;
    
    acceptedEvent = NPP_HandleEvent(instance, &event);
    
    WEBKITDEBUGLEVEL(WEBKIT_LOG_PLUGINS, "NPP_HandleEvent(loseFocusEvent): %d\n", acceptedEvent);
    return YES;
}


-(void)mouseDown:(NSEvent *)theEvent
{
    EventRecord event;
    bool acceptedEvent;
    
    [self getCarbonEvent:&event withEvent:theEvent];
    event.what = mouseDown;
    
    acceptedEvent = NPP_HandleEvent(instance, &event);
    
    WEBKITDEBUGLEVEL(WEBKIT_LOG_PLUGINS, "NPP_HandleEvent(mouseDown): %d pt.v=%d, pt.h=%d\n", acceptedEvent, event.where.v, event.where.h);
}

-(void)mouseUp:(NSEvent *)theEvent
{
    EventRecord event;
    bool acceptedEvent;
    
    [self getCarbonEvent:&event withEvent:theEvent];
    event.what = mouseUp;

    acceptedEvent = NPP_HandleEvent(instance, &event);
    
    WEBKITDEBUGLEVEL(WEBKIT_LOG_PLUGINS, "NPP_HandleEvent(mouseUp): %d pt.v=%d, pt.h=%d\n", acceptedEvent, event.where.v, event.where.h);
}

- (void)mouseEntered:(NSEvent *)theEvent
{
    EventRecord event;
    bool acceptedEvent;
    
    [self getCarbonEvent:&event withEvent:theEvent];
    event.what = adjustCursorEvent;

    acceptedEvent = NPP_HandleEvent(instance, &event);
    
    WEBKITDEBUGLEVEL(WEBKIT_LOG_PLUGINS, "NPP_HandleEvent(mouseEntered): %d\n", acceptedEvent);
}

- (void)mouseExited:(NSEvent *)theEvent
{
    EventRecord event;
    bool acceptedEvent;
        
    [self getCarbonEvent:&event withEvent:theEvent];
    event.what = adjustCursorEvent;

    acceptedEvent = NPP_HandleEvent(instance, &event);
    
    WEBKITDEBUGLEVEL(WEBKIT_LOG_PLUGINS, "NPP_HandleEvent(mouseExited): %d\n", acceptedEvent);
    
    // Set cursor back to arrow cursor.
    [[NSCursor arrowCursor] set];
}

- (void)keyUp:(NSEvent *)theEvent
{
    EventRecord event;
    bool acceptedEvent;
        
    [self getCarbonEvent:&event withEvent:theEvent];
    event.what = keyUp;
    // FIXME: Unicode characters vs. Macintosh ASCII character codes?
    // FIXME: Multiple characters?
    // FIXME: Key codes?
    event.message = [[theEvent characters] characterAtIndex:0];

    acceptedEvent = NPP_HandleEvent(instance, &event);
    
    WEBKITDEBUGLEVEL(WEBKIT_LOG_PLUGINS, "NPP_HandleEvent(keyUp): %d key:%c\n", acceptedEvent, (char) (event.message & charCodeMask));
}

- (void)keyDown:(NSEvent *)theEvent
{
    EventRecord event;
    bool acceptedEvent;
    
    [self getCarbonEvent:&event withEvent:theEvent];
    event.what = keyDown;
    // FIXME: Unicode characters vs. Macintosh ASCII character codes?
    // FIXME: Multiple characters?
    // FIXME: Key codes?
    event.message = [[theEvent characters] characterAtIndex:0];
    
    acceptedEvent = NPP_HandleEvent(instance, &event);
    
    WEBKITDEBUGLEVEL(WEBKIT_LOG_PLUGINS, "NPP_HandleEvent(keyDown): %d key:%c\n", acceptedEvent, (char) (event.message & charCodeMask));
}

#pragma mark IFPLUGINVIEW

// Could do this as a category on NSString if we wanted.
static char *newCString(NSString *string)
{
    char *cString = new char[[string cStringLength] + 1];
    [string getCString:cString];
    return cString;
}

- (id)initWithFrame:(NSRect)r plugin:(IFPlugin *)plugin url:(NSURL *)theURL mime:(NSString *)mimeType arguments:(NSDictionary *)arguments mode:(uint16)mode
{
    NSString *baseURLString;

    [super initWithFrame: r];
    
    // The following line doesn't work for Flash, so I have create a NPP_t struct and point to it
    //instance = malloc(sizeof(NPP_t));
    instance = &instanceStruct;
    instance->ndata = self;

    mime = [mimeType retain];
    srcURL = [theURL retain];
    
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

    // get base URL which was added in the args in the part
    baseURLString = [arguments objectForKey:@"WebKitBaseURL"];
    if (baseURLString)
        baseURL = [[NSURL URLWithString:baseURLString] retain];
            
    isHidden = [arguments objectForKey:@"hidden"] != nil;
    fullMode = [arguments objectForKey:@"wkfullmode"] != nil;
    
    argsCount = 0;
    if (fullMode) {
        cAttributes = 0;
        cValues = 0;
    } else {
        // Convert arguments dictionary to 2 string arrays.
        // These arrays are passed to NPP_New, but the strings need to be
        // modifiable and live the entire life of the plugin.
        
        cAttributes = new char * [[arguments count]];
        cValues = new char * [[arguments count]];
        
        NSEnumerator *e = [arguments keyEnumerator];
        NSString *key;
        while ((key = [e nextObject])) {
            if (![key isEqualToString:@"wkfullmode"]) {
                cAttributes[argsCount] = newCString(key);
                cValues[argsCount] = newCString([arguments objectForKey:key]);
                argsCount++;
            }
        }
    }
    
    streams = [[NSMutableArray alloc] init];
    notificationData = [[NSMutableDictionary dictionaryWithCapacity:1] retain];
    
    // Initialize globals
    canRestart = YES;
    isStarted = NO;
    
    return self;
}

-(void)dealloc
{
    unsigned i;

    [self stop];
    
    for (i = 0; i < argsCount; i++) {
        delete [] cAttributes[i];
        delete [] cValues[i];
    }
    [streams removeAllObjects];
    [streams release];
    [mime release];
    [srcURL release];
    [notificationData release];
    delete [] cAttributes;
    delete [] cValues;
    [super dealloc];
}

- (void) setWindow
{
    NPError npErr;
    NSRect windowFrame, frameInWindow, visibleRectInWindow;
    
    windowFrame = [[self window] frame];
    frameInWindow = [self convertRect:[self bounds] toView:nil];
    visibleRectInWindow = [self convertRect:[self visibleRect] toView:nil];
    
    // flip Y coordinates
    frameInWindow.origin.y =  windowFrame.size.height - frameInWindow.origin.y - frameInWindow.size.height; 
    visibleRectInWindow.origin.y =  windowFrame.size.height - visibleRectInWindow.origin.y - visibleRectInWindow.size.height;
    
    nPort.port = GetWindowPort([[self window] _windowRef]);
    
    // FIXME: Are these values correct? Without them, Flash freaks.
    nPort.portx = -(int32)frameInWindow.origin.x;
    nPort.porty = -(int32)frameInWindow.origin.y;   
    window.window = &nPort;
    
    window.x = (int32)frameInWindow.origin.x; 
    window.y = (int32)frameInWindow.origin.y;

    window.width = (uint32)frameInWindow.size.width;
    window.height = (uint32)frameInWindow.size.height;

    window.clipRect.top = (uint16)visibleRectInWindow.origin.y;
    window.clipRect.left = (uint16)visibleRectInWindow.origin.x;
    window.clipRect.bottom = (uint16)(visibleRectInWindow.origin.y + visibleRectInWindow.size.height);
    window.clipRect.right = (uint16)(visibleRectInWindow.origin.x + visibleRectInWindow.size.width);
    
    window.type = NPWindowTypeWindow;
    
    npErr = NPP_SetWindow(instance, &window);
    WEBKITDEBUGLEVEL(WEBKIT_LOG_PLUGINS, "NPP_SetWindow: %d, port=0x%08lx\n", npErr, (int)nPort.port);
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

-(void)start
{
    NPSavedData saved;
    NPError npErr;
    NSNotificationCenter *notificationCenter;
    NSWindow *theWindow;
    IFPluginStream *stream;
        
    if(isStarted || !canRestart)
        return;
    
    isStarted = YES;
    
    npErr = NPP_New((char *)[mime cString], instance, fullMode ? NP_FULL : NP_EMBED, argsCount, cAttributes, cValues, &saved);
    WEBKITDEBUGLEVEL(WEBKIT_LOG_PLUGINS, "NPP_New: %d\n", npErr);
    
    // Create a WindowRef is one doesn't already exist
    [[self window] _windowRef];
        
    [self setWindow];
    
    theWindow = [self window];
    notificationCenter = [NSNotificationCenter defaultCenter];
    [notificationCenter addObserver:self selector:@selector(viewHasMoved:) name:@"NSViewBoundsDidChangeNotification" object:[self findSuperview:@"NSClipView"]];
    [notificationCenter addObserver:self selector:@selector(viewHasMoved:) name:@"NSWindowDidResizeNotification" object:theWindow];
    [notificationCenter addObserver:self selector:@selector(windowWillClose:) name:@"NSWindowWillCloseNotification" object:theWindow];
    [notificationCenter addObserver:self selector:@selector(windowBecameKey:) name:@"NSWindowDidBecomeKeyNotification" object:theWindow];
    [notificationCenter addObserver:self selector:@selector(windowResignedKey:) name:@"NSWindowDidResignKeyNotification" object:theWindow];
    [notificationCenter addObserver:self selector:@selector(defaultsHaveChanged:) name:@"NSUserDefaultsDidChangeNotification" object:nil];
    
    if ([theWindow isKeyWindow])
        [self sendActivateEvent:YES];

    if(srcURL){
        stream = [[IFPluginStream alloc] initWithURL:srcURL pluginPointer:instance];
        if(stream){
            [streams addObject:stream];
            [stream release];
        }
    }
    
    eventSender = [[IFPluginNullEventSender alloc] initializeWithNPP:instance functionPointer:NPP_HandleEvent window:theWindow];
    [eventSender sendNullEvents];
    trackingTag = [self addTrackingRect:[self bounds] owner:self userData:nil assumeInside:NO];
    
    id webView = [self findSuperview:@"IFWebView"];
    webController = [[webView controller] retain];
    webFrame = 	    [[webController frameForView:webView] retain];
    webDataSource = [[webFrame dataSource] retain];
}

- (void)stop
{
    NPError npErr;
    
    if (!isStarted)
        return;
    isStarted = NO;

    [self removeTrackingRect:trackingTag];
            
    // Stop any active streams
    [streams makeObjectsPerformSelector:@selector(stop)];
    
    // Stop the null events
    [eventSender stop];
    [eventSender release];

    // Set cursor back to arrow cursor
    [[NSCursor arrowCursor] set];
    
    // Stop notifications
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    
    // Release web objects here to avoid circular retain
    [webController release];
    [webFrame release];
    [webDataSource release];    
    
    npErr = NPP_Destroy(instance, NULL);
    WEBKITDEBUGLEVEL(WEBKIT_LOG_PLUGINS, "NPP_Destroy: %d\n", npErr);
}

- (IFWebDataSource *)webDataSource
{
    return webDataSource;
}

- (id <IFWebController>) webController
{
    return webController;
}

#pragma mark NSVIEW

- (void)drawRect:(NSRect)rect
{
    if(!isStarted)
        [self start];
    if(isStarted)
        [self sendUpdateEvent];
}

- (BOOL)isFlipped
{
    return YES;
}

#pragma mark NOTIFICATIONS

-(void) viewHasMoved:(NSNotification *)notification
{
    [self sendUpdateEvent];
    [self setWindow];
    
    // reset the tracking rect
    [self removeTrackingRect:trackingTag];
    trackingTag = [self addTrackingRect:[self bounds] owner:self userData:nil assumeInside:NO];
}

-(void) windowBecameKey:(NSNotification *)notification
{
    [self sendActivateEvent:YES];
}

-(void) windowResignedKey:(NSNotification *)notification
{
    [self sendActivateEvent:NO];
}

- (void) windowWillClose:(NSNotification *)notification
{
    [self stop];
}

- (void) defaultsHaveChanged:(NSNotification *)notification
{
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    if([defaults boolForKey:@"WebKitPluginsEnabled"]){
        canRestart = YES;
        [self start];
    }else{
        canRestart = NO;
        [self stop];
        [self setNeedsDisplay:YES];
    }
}

- (void) frameStateChanged:(NSNotification *)notification
{
    IFWebFrame *frame;
    IFWebFrameState frameState;
    NSValue *notifyDataValue;
    void *notifyData;
    NSURL *url;
    
    frame = [notification object];
    url = [[frame dataSource] inputURL];
    notifyDataValue = [notificationData objectForKey:url];
    
    if(!notifyDataValue)
        return;
    
    notifyData = [notifyDataValue pointerValue];
    frameState = (IFWebFrameState)[[[notification userInfo] objectForKey:IFCurrentFrameState] intValue];
    if(frameState == IFWEBFRAMESTATE_COMPLETE){
        NPP_URLNotify(instance, [[url absoluteString] cString], NPRES_DONE, notifyData);
    }
    //FIXME: Need to send other NPReasons
}

#pragma mark PLUGIN-TO-BROWSER

- (NPError) loadURL:(NSString *)URLString inTarget:(NSString *)target withNotifyData:(void *)notifyData andHandleAttributes:(NSDictionary *)attributes
{
    IFPluginStream *stream;
    IFWebDataSource *dataSource;
    IFWebFrame *frame;
    NSURL *url;
    
    if([URLString _IF_looksLikeAbsoluteURL])
        url = [NSURL URLWithString:URLString];
    else
        url = [NSURL URLWithString:URLString relativeToURL:baseURL];
    
    if(!url)
        return NPERR_INVALID_URL;
    
    if(!target){
        stream = [[IFPluginStream alloc] initWithURL:url pluginPointer:instance notifyData:notifyData attributes:attributes];
        if(stream){
            [streams addObject:stream];
            [stream release];
        }else{
            return NPERR_INVALID_URL;
        }
    }else{
        frame = [webFrame frameNamed:target];
        if(!frame){
            //FIXME: Create new window here (2931449)
            return NPERR_GENERIC_ERROR;
        }
        if(notifyData){
            if([target isEqualToString:@"_self"] || [target isEqualToString:@"_current"] || 
               [target isEqualToString:@"_parent"] || [target isEqualToString:@"_top"]){
                // return error since notification can't be sent to a plug-in that will no longer exist
                return NPERR_INVALID_PARAM;
            }
            [notificationData setObject:[NSValue valueWithPointer:notifyData] forKey:url];
            [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(frameStateChanged:) name:IFFrameStateChangedNotification object:frame];
        }
        dataSource = [[[IFWebDataSource alloc] initWithURL:url attributes:attributes] autorelease];
        [frame setProvisionalDataSource:dataSource];
        [frame startLoading];
    }
    return NPERR_NO_ERROR;
}

-(NPError)getURLNotify:(const char *)url target:(const char *)target notifyData:(void *)notifyData
{
    NSString *theTarget = nil;
        
    WEBKITDEBUGLEVEL(WEBKIT_LOG_PLUGINS, "NPN_GetURLNotify: %s target: %s\n", url, target);
        
    if(!url)
        return NPERR_INVALID_URL;
        
    if(target)
        theTarget = [NSString stringWithCString:target];
    
    return [self loadURL:[NSString stringWithCString:url] inTarget:theTarget withNotifyData:notifyData andHandleAttributes:nil];
}

-(NPError)getURL:(const char *)url target:(const char *)target
{
    NSString *theTarget = nil;
    
    WEBKITDEBUGLEVEL(WEBKIT_LOG_PLUGINS, "NPN_GetURL: %s target: %s\n", url, target);
    
    if(!url)
        return NPERR_INVALID_URL;
        
    if(target)
        theTarget = [NSString stringWithCString:target];
    
    return [self loadURL:[NSString stringWithCString:url] inTarget:theTarget withNotifyData:NULL andHandleAttributes:nil];
}

-(NPError)postURLNotify:(const char *)url target:(const char *)target len:(UInt32)len buf:(const char *)buf file:(NPBool)file notifyData:(void *)notifyData
{
    NSDictionary *attributes=nil;
    NSData *postData;
    NSURL *tempURL;
    NSString *path, *theTarget = nil;
    
    WEBKITDEBUGLEVEL(WEBKIT_LOG_PLUGINS, "NPN_PostURLNotify: %s\n", url);
 
    if(!url)
        return NPERR_INVALID_URL;
        
    if(target)
        theTarget = [NSString stringWithCString:target];
 
    if(file){
        if([[NSString stringWithCString:buf] _IF_looksLikeAbsoluteURL]){
            tempURL = [NSURL fileURLWithPath:[NSString stringWithCString:url]];
            path = [tempURL path];
        }else{
            path = [NSString stringWithCString:buf];
        }
        postData = [NSData dataWithContentsOfFile:path];
    }else{
        postData = [NSData dataWithBytes:buf length:len];
    }
    
    attributes = [NSDictionary dictionaryWithObjectsAndKeys:
        postData,	IFHTTPURLHandleRequestData,
        @"POST", 	IFHTTPURLHandleRequestMethod, nil];
                
    return [self loadURL:[NSString stringWithCString:url] inTarget:theTarget 
                withNotifyData:notifyData andHandleAttributes:attributes];
}

-(NPError)postURL:(const char *)url target:(const char *)target len:(UInt32)len buf:(const char *)buf file:(NPBool)file
{
    NSString *theTarget = nil;
        
    WEBKITDEBUGLEVEL(WEBKIT_LOG_PLUGINS, "NPN_PostURL: %s\n", url);
    
    if(!url)
        return NPERR_INVALID_URL;
        
    if(target)
        theTarget = [NSString stringWithCString:target];
        
    return [self postURLNotify:url target:target len:len buf:buf file:file notifyData:NULL];
}

-(NPError)newStream:(NPMIMEType)type target:(const char *)target stream:(NPStream**)stream
{
    WEBKITDEBUGLEVEL(WEBKIT_LOG_PLUGINS, "NPN_NewStream\n");
    return NPERR_GENERIC_ERROR;
}

-(NPError)write:(NPStream*)stream len:(SInt32)len buffer:(void *)buffer
{
    WEBKITDEBUGLEVEL(WEBKIT_LOG_PLUGINS, "NPN_Write\n");
    return NPERR_GENERIC_ERROR;
}

-(NPError)destroyStream:(NPStream*)stream reason:(NPReason)reason
{
    WEBKITDEBUGLEVEL(WEBKIT_LOG_PLUGINS, "NPN_DestroyStream\n");
    if(!stream->ndata)
        return NPERR_INVALID_INSTANCE_ERROR;
        
    [(IFPluginStream *)stream->ndata stop];
    return NPERR_NO_ERROR;
}

-(void)status:(const char *)message
{
    WEBKITDEBUGLEVEL(WEBKIT_LOG_PLUGINS, "NPN_Status: %s\n", message);
    if(webController){
        [webController setStatusText:[NSString stringWithCString:message] forDataSource:webDataSource];
    }
}

-(void)invalidateRect:(NPRect *)invalidRect
{
    WEBKITDEBUGLEVEL(WEBKIT_LOG_PLUGINS, "NPN_InvalidateRect\n");
    [self sendUpdateEvent];
}

-(void)invalidateRegion:(NPRegion)invalidateRegion
{
    WEBKITDEBUGLEVEL(WEBKIT_LOG_PLUGINS, "NPN_InvalidateRegion\n");
    [self sendUpdateEvent];
}

-(void)forceRedraw
{
    WEBKITDEBUGLEVEL(WEBKIT_LOG_PLUGINS, "forceRedraw\n");
    [self sendUpdateEvent];
}


- (NPP_NewStreamProcPtr)NPP_NewStream
{
    return NPP_NewStream;
}

- (NPP_WriteReadyProcPtr)NPP_WriteReady
{
    return NPP_WriteReady;
}


- (NPP_WriteProcPtr)NPP_Write
{
    return NPP_Write;
}


- (NPP_StreamAsFileProcPtr)NPP_StreamAsFile
{
    return NPP_StreamAsFile;
}


- (NPP_DestroyStreamProcPtr)NPP_DestroyStream
{
    return NPP_DestroyStream;
}


- (NPP_URLNotifyProcPtr)NPP_URLNotify
{
    return NPP_URLNotify;
}

@end
