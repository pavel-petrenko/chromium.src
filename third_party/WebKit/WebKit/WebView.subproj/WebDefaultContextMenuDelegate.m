/*
      WebDefaultContextMenuDelegate.m
      Copyright 2002, Apple, Inc. All rights reserved.
*/

#import <WebKit/WebContextMenuDelegate.h>

#import <WebKit/WebBridge.h>
#import <WebKit/WebControllerPolicyDelegate.h>
#import <WebKit/WebControllerPrivate.h>
#import <WebKit/WebDataSourcePrivate.h>
#import <WebKit/WebDefaultContextMenuDelegate.h>
#import <WebKit/WebFramePrivate.h>
#import <WebKit/WebNSPasteboardExtras.h>
#import <WebKit/WebView.h>
#import <WebKit/WebWindowOperationsDelegate.h>

#import <WebFoundation/WebHTTPRequest.h>
#import <WebFoundation/WebLocalizableStrings.h>
#import <WebFoundation/WebResource.h>
#import <WebFoundation/WebRequest.h>

@implementation WebDefaultContextMenuDelegate

static WebDefaultContextMenuDelegate *sharedDelegate = nil;

// Return a object with vanilla implementations of the protocol's methods
// Note this feature relies on our default delegate being stateless
+ (WebDefaultContextMenuDelegate *)sharedContextMenuDelegate
{
    if (!sharedDelegate) {
        sharedDelegate = [[WebDefaultContextMenuDelegate alloc] init];
    }
    return sharedDelegate;
}

- (void)dealloc
{
    [element release];
    [super dealloc];
}

- (NSMenuItem *)menuItemWithTag:(int)tag
{
    NSMenuItem *menuItem = [[NSMenuItem alloc] init];
    [menuItem setTarget:self];
    [menuItem setTag:tag];
    
    NSString *title;
    SEL action;
    
    switch(tag) {
        case WebMenuItemTagOpenLinkInNewWindow:
            title = UI_STRING("Open Link in New Window", "Open in New Window context menu item");
            action = @selector(openLinkInNewWindow:);
            break;
        case WebMenuItemTagDownloadLinkToDisk:
            title = UI_STRING("Download Link to Disk", "Download Link to Disk context menu item");
            action = @selector(downloadLinkToDisk:);
            break;
        case WebMenuItemTagCopyLinkToClipboard:
            title = UI_STRING("Copy Link to Clipboard", "Copy Link to Clipboard context menu item");
            action = @selector(copyLinkToClipboard:);
            break;
        case WebMenuItemTagOpenImageInNewWindow:
            title = UI_STRING("Open Image in New Window", "Open Image in New Window context menu item");
            action = @selector(openImageInNewWindow:);
            break;
        case WebMenuItemTagDownloadImageToDisk:
            title = UI_STRING("Download Image To Disk", "Download Image To Disk context menu item");
            action = @selector(downloadImageToDisk:);
            break;
        case WebMenuItemTagCopyImageToClipboard:
            title = UI_STRING("Copy Image to Clipboard", "Copy Image to Clipboard context menu item");
            action = @selector(copyImageToClipboard:);
            break;
        case WebMenuItemTagOpenFrameInNewWindow:
            title = UI_STRING("Open Frame in New Window", "Open Frame in New Window context menu item");
            action = @selector(openFrameInNewWindow:);
            break;
        case WebMenuItemTagCopy:
            title = UI_STRING("Copy", "Copy context menu item");
            action = @selector(copy:);
            [menuItem setTarget:nil];
            break;
        default:
            [menuItem release];
            return nil;
    }

    [menuItem setTitle:title];
    [menuItem setAction:action];
    
    return [menuItem autorelease];
}

- (NSArray *)contextMenuItemsForElement: (NSDictionary *)theElement  defaultMenuItems: (NSArray *)defaultMenuItems
{
    NSMutableArray *menuItems = [NSMutableArray array];
    NSURL *linkURL, *imageURL;
    
    [element release];
    element = [theElement retain];

    linkURL = [element objectForKey:WebElementLinkURLKey];

    if (linkURL) {
        if([WebResource canInitWithRequest:[WebRequest requestWithURL:linkURL]]){
            [menuItems addObject:[self menuItemWithTag:WebMenuItemTagOpenLinkInNewWindow]];
            [menuItems addObject:[self menuItemWithTag:WebMenuItemTagDownloadLinkToDisk]];
            [menuItems addObject:[self menuItemWithTag:WebMenuItemTagCopyLinkToClipboard]];
        }
    }

    imageURL = [element objectForKey:WebElementImageURLKey];
    if(imageURL){
        if(linkURL){
            [menuItems addObject:[NSMenuItem separatorItem]];
        }
        [menuItems addObject:[self menuItemWithTag:WebMenuItemTagOpenImageInNewWindow]];
        [menuItems addObject:[self menuItemWithTag:WebMenuItemTagDownloadImageToDisk]];
        [menuItems addObject:[self menuItemWithTag:WebMenuItemTagCopyImageToClipboard]];
    }

    if (!imageURL && !linkURL) {
        if ([[element objectForKey:WebElementIsSelectedTextKey] boolValue]) {
            [menuItems addObject:[self menuItemWithTag:WebMenuItemTagCopy]];
        } else {        
            WebFrame *webFrame = [element objectForKey:WebElementFrameKey];
    
            if(webFrame != [[webFrame controller] mainFrame]){
                [menuItems addObject:[self menuItemWithTag:WebMenuItemTagOpenFrameInNewWindow]];
            }
        }
    }

    return menuItems;
}

- (void)openNewWindowWithURL:(NSURL *)URL
{
    WebFrame *webFrame = [element objectForKey:WebElementFrameKey];
    WebController *controller = [webFrame controller];
    
    WebRequest *request = [WebRequest requestWithURL:URL];
    NSString *referrer = [[webFrame _bridge] referrer];
    if (referrer) {
	[request setReferrer:referrer];
    }
    
    [controller _openNewWindowWithRequest:request];
}

- (void)downloadURL:(NSURL *)URL
{
    WebFrame *webFrame = [element objectForKey:WebElementFrameKey];
    WebController *controller = [webFrame controller];
    [controller _downloadURL:URL];
}

- (void)openLinkInNewWindow:(id)sender
{
    [self openNewWindowWithURL:[element objectForKey:WebElementLinkURLKey]];
}

- (void)downloadLinkToDisk:(id)sender
{
    [self downloadURL:[element objectForKey:WebElementLinkURLKey]];
}

- (void)copyLinkToClipboard:(id)sender
{
    NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
    [pasteboard _web_writeURL:[element objectForKey:WebElementLinkURLKey]
                     andTitle:[element objectForKey:WebElementLinkLabelKey]
                    withOwner:self];
}

- (void)openImageInNewWindow:(id)sender
{
    [self openNewWindowWithURL:[element objectForKey:WebElementImageURLKey]];
}

- (void)downloadImageToDisk:(id)sender
{
    [self downloadURL:[element objectForKey:WebElementImageURLKey]];
}

- (void)copyImageToClipboard:(id)sender
{
    NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
    NSData *tiff = [[element objectForKey:WebElementImageKey] TIFFRepresentation];
    
    [pasteboard declareTypes:[NSArray arrayWithObject:NSTIFFPboardType] owner:nil];
    [pasteboard setData:tiff forType:NSTIFFPboardType];
}

- (void)openFrameInNewWindow:(id)sender
{
    WebFrame *webFrame = [element objectForKey:WebElementFrameKey];
    WebDataSource *dataSource = [webFrame dataSource];
    NSURL *URL = [dataSource _URL];
    [self openNewWindowWithURL:URL];
}


@end
