//
//  IFDownloadHandler.m
//  WebKit
//
//  Created by Chris Blumenberg on Thu Apr 11 2002.
//  Copyright (c) 2002 __MyCompanyName__. All rights reserved.
//

#import <WebKit/IFDownloadHandler.h>
#import <WebKit/IFWebDataSourcePrivate.h>
#import <WebKit/WebKitDebug.h>

@implementation IFDownloadHandler

- initWithDataSource:(IFWebDataSource *)dSource
{
    dataSource = [dSource retain];
    WEBKITDEBUGLEVEL(WEBKIT_LOG_DOWNLOAD, "Download started for: %s", [[[dSource inputURL] absoluteString] cString]);
    return self;
}

- (void)dealloc
{
    [dataSource release];
}

- (void)downloadCompletedWithData:(NSData *)data;
{
    NSString *path = [dataSource _downloadPath];
    NSFileManager *fileManager;
       
    // FIXME: Should probably not replace existing file
    // FIXME: Should report error if there is one
    fileManager = [NSFileManager defaultManager];
    [fileManager createFileAtPath:path contents:data attributes:nil];
    WEBKITDEBUGLEVEL(WEBKIT_LOG_DOWNLOAD, "Download complete. Saved to: %s", [path cString]);
    
    // Send Finder notification
    WEBKITDEBUGLEVEL(WEBKIT_LOG_DOWNLOAD, "Notifying Finder");
    FNNotifyByPath((UInt8 *)[[path stringByDeletingLastPathComponent] UTF8String], kFNDirectoryModifiedMessage, kNilOptions);
    
    if([dataSource contentPolicy] == IFContentPolicyOpenExternally){
        [[self class] launchURL:[NSURL fileURLWithPath:path]];
    }
}

+ (void) launchURL:(NSURL *) url{
    WEBKITDEBUGLEVEL(WEBKIT_LOG_DOWNLOAD,"Launching: %s", [[url absoluteString] cString]);
    LSOpenCFURLRef((CFURLRef)url, NULL);
}

@end
