/*	
        WebLocationChangeDelegate.m
	Copyright 2002, Apple, Inc. All rights reserved.
*/

#import "WebLocationChangeDelegate.h"

@implementation WebLocationChangeDelegate

- (void)locationChangeStartedForDataSource:(WebDataSource *)dataSource { }

- (void)serverRedirectedForDataSource:(WebDataSource *)dataSource { }

- (void)locationChangeCommittedForDataSource:(WebDataSource *)dataSource { }

- (void)receivedPageTitle:(NSString *)title forDataSource:(WebDataSource *)dataSource { }
- (void)receivedPageIcon:(NSImage *)image forDataSource:(WebDataSource *)dataSource { }

- (void)locationChangeDone:(WebError *)error forDataSource:(WebDataSource *)dataSource { }

- (void)willCloseLocationForDataSource:(WebDataSource *)dataSource { }

- (void)locationChangedWithinPageForDataSource:(WebDataSource *)dataSource { }

- (void)clientWillRedirectTo:(NSURL *)URL delay:(NSTimeInterval)seconds fireDate:(NSDate *)date forFrame:(WebFrame *)frame { }
- (void)clientRedirectCancelledForFrame:(WebFrame *)frame { }

@end
