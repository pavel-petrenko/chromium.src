/*	
        IFWebFrame.m
	    
	    Copyright 2001, Apple, Inc. All rights reserved.
*/

#import <WebKit/IFWebFrame.h>

#import <Cocoa/Cocoa.h>

#import <WebKit/IFWebCoreFrame.h>
#import <WebKit/IFWebFramePrivate.h>
#import <WebKit/IFWebViewPrivate.h>
#import <WebKit/IFWebDataSourcePrivate.h>
#import <WebKit/IFWebControllerPrivate.h>
#import <WebKit/IFWebController.h>
#import <WebKit/IFLocationChangeHandler.h>
#import <WebKit/IFHTMLView.h>
#import <WebKit/IFHTMLViewPrivate.h>

#import <WebFoundation/WebFoundation.h>

#import <WebKit/WebKitDebug.h>

#import <rendering/render_frames.h>

@implementation IFWebFrame

- init
{
    return [self initWithName: nil webView: nil provisionalDataSource: nil controller: nil];
}

- initWithName: (NSString *)n webView: (IFWebView *)v provisionalDataSource: (IFWebDataSource *)d controller: (IFWebController *)c
{
    [super init];

    _private = [[IFWebFramePrivate alloc] init];
    _private->bridgeFrame = [[IFWebCoreFrame alloc] initWithWebFrame:self];

    [self _setState: IFWEBFRAMESTATE_UNINITIALIZED];    

    [self setController: c];

    if (d == nil) {
	// set a dummy data source so that the main from for a
	// newly-created empty window has a KHTMLPart. JavaScript
	// always creates new windows initially empty, and then wants
	// to use the main frame's part to make the new window load
	// it's URL, so we need to make sure empty frames have a part.
	// However, we don't want to do the spinner, so we do this
	// weird thing:

	IFWebDataSource *dummyDataSource = [[IFWebDataSource alloc] initWithURL:nil];
        [dummyDataSource _setController: [self controller]];
        [_private setProvisionalDataSource: dummyDataSource];

    // Allow controller to override?
    } else if ([self setProvisionalDataSource: d] == NO){
        [self autorelease];
        return nil;
    }
    
    [_private setName: n];
    
    if (v)
        [self setWebView: v];
    
    return self;
}

- (void)dealloc
{
    [_private release];
    [super dealloc];
}

- (NSString *)name
{
    return [_private name];
}


- (void)setWebView: (IFWebView *)v
{
    [_private setWebView: v];
    [v _setController: [self controller]];
}

- (IFWebView *)webView
{
    return [_private webView];
}

- (IFWebController *)controller
{
    return [_private controller];
}


- (void)setController: (IFWebController *)controller
{
    [_private setController: controller];
}


- (IFWebDataSource *)provisionalDataSource
{
    return [_private provisionalDataSource];
}


- (IFWebDataSource *)dataSource
{
    return [_private dataSource];
}


//    Will return NO and not set the provisional data source if the controller
//    disallows by returning a IFURLPolicyIgnore.
- (BOOL)setProvisionalDataSource: (IFWebDataSource *)newDataSource
{
    IFWebDataSource *oldDataSource;
    id <IFLocationChangeHandler>locationChangeHandler;
    IFURLPolicy urlPolicy;
    
    WEBKIT_ASSERT ([self controller] != nil);

    // Unfortunately the view must be non-nil, this is ultimately due
    // to KDE parser requiring a KHTMLView.  Once we settle on a final
    // KDE drop we should fix this dependency.
    WEBKIT_ASSERT ([self webView] != nil);

    urlPolicy = [[[self controller] policyHandler] URLPolicyForURL:[newDataSource inputURL]];

    if(urlPolicy == IFURLPolicyUseContentPolicy){
            
        if ([self _state] != IFWEBFRAMESTATE_COMPLETE){
            [self stopLoading];
        }
        
        locationChangeHandler = [[[self controller] policyHandler] provideLocationChangeHandlerForFrame: self];
    
        [newDataSource _setLocationChangeHandler: locationChangeHandler];
    
        oldDataSource = [self dataSource];
        
        // Is this the top frame?  If so set the data source's parent to nil.
        if (self == [[self controller] mainFrame])
            [newDataSource _setParent: nil];
            
        // Otherwise set the new data source's parent to the old data source's parent.
        else if (oldDataSource && oldDataSource != newDataSource)
            [newDataSource _setParent: [oldDataSource parent]];
                
        [newDataSource _setController: [self controller]];
        
        [_private setProvisionalDataSource: newDataSource];
        
        // We tell the documentView provisionalDataSourceChanged:
        // once it has been created by the controller.
            
        [self _setState: IFWEBFRAMESTATE_PROVISIONAL];
    }
    else if(urlPolicy == IFURLPolicyOpenExternally){
        return [[NSWorkspace sharedWorkspace] openURL:[newDataSource inputURL]];
    }
    else if (urlPolicy == IFURLPolicyIgnore)
        return NO;
        
    return YES;
}


- (void)startLoading
{
    if (self == [[self controller] mainFrame])
        WEBKITDEBUGLEVEL (WEBKIT_LOG_DOCUMENTLOAD, "loading %s", [[[[self provisionalDataSource] inputURL] absoluteString] cString]);

    // Force refresh is irrelevant, as this will always be the first load.
    // The controller will transition the provisional data source to the
    // committed data source.
    [_private->provisionalDataSource startLoading: NO];
}


- (void)stopLoading
{
    [_private->provisionalDataSource stopLoading];
    [_private->dataSource stopLoading];
}


- (void)reload: (BOOL)forceRefresh
{
    [_private->dataSource _clearErrors];

    [_private->dataSource startLoading: forceRefresh];
}


- (void)reset
{
    [_private setDataSource: nil];
    if([[[self webView] documentView] isKindOfClass: NSClassFromString(@"IFHTMLView")])
        [[[self webView] documentView] _resetWidget];
    [_private setWebView: nil];
}

+ _frameNamed:(NSString *)name fromFrame: (IFWebFrame *)aFrame
{
    int i, count;
    IFWebFrame *foundFrame;
    NSArray *children;

    if ([[aFrame name] isEqualToString: name])
        return aFrame;

    children = [[aFrame dataSource] children];
    count = [children count];
    for (i = 0; i < count; i++){
        aFrame = [children objectAtIndex: i];
        foundFrame = [IFWebFrame _frameNamed: name fromFrame: aFrame];
        if (foundFrame)
            return foundFrame;
    }
    
    // FIXME:  Need to look in other controller's frame namespaces.

    // FIXME:  What do we do if a frame name isn't found?  create a new window
    
    return nil;
}

- (IFWebFrame *)frameNamed:(NSString *)name
{
    // First, deal with 'special' names.
    if([name isEqualToString:@"_self"] || [name isEqualToString:@"_current"]){
        return self;
    }
    
    else if([name isEqualToString:@"_top"]) {
        return [[self controller] mainFrame];
    }
    
    else if([name isEqualToString:@"_parent"]){
        IFWebDataSource *parent = [[self dataSource] parent];
        if(parent){
            return [parent webFrame];
        }
        else{
            return self;
        }
    }
    
    else if ([name isEqualToString:@"_blank"]){
        IFWebController *newController = [[self controller] openNewWindowWithURL: nil];
        return [newController mainFrame];
    }
    
    // Now search the namespace associated with this frame's controller.
    return [IFWebFrame _frameNamed: name fromFrame: [[self controller] mainFrame]];
}

@end
