//
//  WebPluginPackage.m
//  WebKit
//
//  Created by Chris Blumenberg on Tue Oct 22 2002.
//  Copyright (c) 2002 Apple Computer, Inc. All rights reserved.
//

#import <WebKit/WebPluginPackage.h>

#import <Foundation/NSBundle_Private.h>

@implementation WebPluginPackage

- initWithPath:(NSString *)pluginPath
{
    [super initWithPath:pluginPath];

    if (!bundle) {
        [self release];
        return nil;
    }
    
    UInt32 type = 0;
    CFBundleGetPackageInfo([bundle _cfBundle], &type, NULL);
    
    if (type != FOUR_CHAR_CODE('WBPL')) {
        [self release];
        return nil;
    }

    if (![self getPluginInfoFromBundleAndMIMEDictionary:nil]) {
        [self release];
        return nil;
    }

    return self;
}

- (Class)viewFactory
{
    return [bundle principalClass];
}

- (BOOL)load
{
    [bundle principalClass];
    return YES;
}

- (void)unload
{
}

- (BOOL)isLoaded
{
    return [bundle isLoaded];
}

@end
