/*
      WebDefaultContextMenuHandler.h

      Copyright 2002, Apple, Inc. All rights reserved.

*/

#import <WebKit/WebController.h>
#import <WebKit/WebDataSource.h>
#import <WebKit/WebDefaultContextMenuHandler.h>
#import <WebKit/WebFrame.h>

@implementation WebDefaultContextMenuHandler

- (void)dealloc
{
    [element release];
    [super dealloc];
}

- (void)addMenuItemWithTitle:(NSString *)title action:(SEL)selector toArray:(NSMutableArray *)menuItems
{
    NSMenuItem *menuItem = [[NSMenuItem alloc] initWithTitle:title action:selector keyEquivalent:@""];
    [menuItem setTarget:self];
    [menuItems addObject:menuItem];
    [menuItem release];
}

- (NSArray *)contextMenuItemsForElement: (NSDictionary *)theElement  defaultMenuItems: (NSArray *)defaultMenuItems
{
    NSMutableArray *menuItems = [NSMutableArray array];
    NSURL *linkURL, *imageURL;
    
    element = [theElement retain];

    linkURL = [element objectForKey:WebContextLinkURL];

    if(linkURL){
    
        [self addMenuItemWithTitle:NSLocalizedString(@"Open Link in New Window", @"Open in New Window context menu item") 				            action:@selector(openLinkInNewWindow:)
                           toArray:menuItems];
        
        [self addMenuItemWithTitle:NSLocalizedString(@"Download Link to Disk", @"Download Link to Disk context menu item") 				    action:@selector(downloadLinkToDisk:)
                           toArray:menuItems];

        [self addMenuItemWithTitle:NSLocalizedString(@"Copy Link to Clipboard", @"Copy Link to Clipboard context menu item") 				    action:@selector(copyLinkToClipboard:)
                           toArray:menuItems];
    }

    imageURL = [element objectForKey:WebContextImageURL];

    if(imageURL){
    
        if(linkURL){
            [menuItems addObject:[NSMenuItem separatorItem]];
        }
        
        [self addMenuItemWithTitle:NSLocalizedString(@"Open Image in New Window", @"Open Image in New Window context menu item") 		             
                            action:@selector(openImageInNewWindow:)
                           toArray:menuItems];
        
        [self addMenuItemWithTitle:NSLocalizedString(@"Download Image To Disk", @"Download Image To Disk context menu item") 				    action:@selector(downloadImageToDisk:)
                           toArray:menuItems];
        
        [self addMenuItemWithTitle:NSLocalizedString(@"Copy Image to Clipboard", @"Copy Image to Clipboard context menu item") 				    action:@selector(copyImageToClipboard:)
                           toArray:menuItems];
        
        [self addMenuItemWithTitle:NSLocalizedString(@"Reload Image", @"Reload Image context menu item") 				                    
                            action:@selector(reloadImage:)
                           toArray:menuItems];
    }

    if(!imageURL && !linkURL){
    
        WebFrame *webFrame = [element objectForKey:WebContextFrame];

        if([[webFrame dataSource] isMainDocument]){
            [self addMenuItemWithTitle:NSLocalizedString(@"View Source", @"View Source context menu item") 				                        
                                action:@selector(viewSource:)
                               toArray:menuItems];
        }else{
            [self addMenuItemWithTitle:NSLocalizedString(@"Open Frame in New Window", @"Open Frame in New Window context menu item") 				action:@selector(openFrameInNewWindow:)
                               toArray:menuItems];
            
            [self addMenuItemWithTitle:NSLocalizedString(@"View Frame Source", @"View Frame Source context menu item") 				                
                                action:@selector(viewSource:)
                               toArray:menuItems];
        }
    }

    return menuItems;
}

- (BOOL)validateUserInterfaceItem:(id <NSValidatedUserInterfaceItem>)anItem
{
    return YES;
}

- (void)openNewWindowWithURL:(NSURL *)URL
{
    WebFrame *webFrame = [element objectForKey:WebContextFrame];
    WebController *controller = [webFrame controller];
    [[controller windowContext] openNewWindowWithURL:URL];
}

- (void)openLinkInNewWindow:(id)sender
{
    [self openNewWindowWithURL:[element objectForKey:WebContextLinkURL]];
}

- (void)copyLinkToClipboard:(id)sender
{
    NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
    NSURL *URL = [element objectForKey:WebContextLinkURL];
    
    [pasteboard declareTypes:[NSArray arrayWithObjects:NSURLPboardType, NSStringPboardType, nil] owner:nil];
    [pasteboard setString:[URL absoluteString] forType:NSStringPboardType];
    [URL writeToPasteboard:pasteboard];
}

- (void)openImageInNewWindow:(id)sender
{
    [self openNewWindowWithURL:[element objectForKey:WebContextImageURL]];
}

- (void)copyImageToClipboard:(id)sender
{
    NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
    NSData *tiff = [[element objectForKey:WebContextImage] TIFFRepresentation];
    
    [pasteboard declareTypes:[NSArray arrayWithObject:NSTIFFPboardType] owner:nil];
    [pasteboard setData:tiff forType:NSTIFFPboardType];
}

- (void)openFrameInNewWindow:(id)sender
{
    WebFrame *webFrame = [element objectForKey:WebContextFrame];
    WebDataSource *dataSource = [webFrame dataSource];
    NSURL *URL = [dataSource wasRedirected] ? [dataSource redirectedURL] : [dataSource inputURL];
    [self openNewWindowWithURL:URL];
}


@end
