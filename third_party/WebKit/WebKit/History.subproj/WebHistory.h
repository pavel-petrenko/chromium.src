/*	
        WebHistory.h
	Copyright 2001, 2002, Apple Computer, Inc.

        Public header file.
*/
#import <Foundation/Foundation.h>

@class WebHistoryItem;
@class WebHistoryPrivate;

/*
    @discussion Notifications sent when history is modified. 
    @constant WebHistoryItemsAddedNotification Posted from addItems:.  This 
    notification comes with a userInfo dictionary that contains the array of
    items added.  The key for the array is WebHistoryItemsKey.
    @constant WebHistoryItemsRemovedNotification Posted from and removeItems:.  
    This notification comes with a userInfo dictionary that contains the array of
    items removed.  The key for the array is WebHistoryItemsKey.
    @constant WebHistoryAllItemsRemovedNotification Posted from removeAllItems
    @constant WebHistoryLoadedNotification Posted from loadHistory.
*/
extern NSString *WebHistoryItemsAddedNotification;
extern NSString *WebHistoryItemsRemovedNotification;
extern NSString *WebHistoryAllItemsRemovedNotification;
extern NSString *WebHistoryLoadedNotification;

extern NSString *WebHistoryItemsKey;

/*!
    @class WebHistory
    @discussion WebHistory is used to track pages that have been loaded
    by WebKit.
*/
@interface WebHistory : NSObject {
@private
    WebHistoryPrivate *_historyPrivate;
}

/*!
    @method sharedHistory
    @abstract Returns a shared WebHistory instance initialized with the default history file.
    @result A WebHistory object.
*/
+ (WebHistory *)sharedHistory;

/*!
    @method createSharedHistoryWithFile:
    @param history The history to use for the global WebHistory.
    @result Returns a WebHistory initialized with the contents of file.
*/
+ (void)setSharedHistory:(WebHistory *)history;

/*!
    @method initWithContentsOfURL:
    @param URL The URL to use to initialize the WebHistory.
    @abstract The designated initializer for WebHistory.
    @result Returns an initialized WebHistory.
*/
- initWithContentsOfURL:(NSURL *)URL;

/*!
    @method addItems:
    @param newItems An array of WebHistoryItems to add to the WebHistory.
*/
- (void)addItems:(NSArray *)newItems;

/*!
    @method removeItems:
    @param items An array of WebHistoryItems to remove from the WebHistory.
*/
- (void)removeItems:(NSArray *)items;

/*!
    @method removeAllItems
*/
- (void)removeAllItems;

/*!
    @method orderedLastVisitedDays
    @discussion Get an array of NSCalendarDates, each one representing a unique day that contains one
    or more history items, ordered from most recent to oldest.
    @result Returns an array of NSCalendarDates for which history items exist in the WebHistory.
*/
- (NSArray *)orderedLastVisitedDays;

/*!
    @method orderedItemsLastVisitedOnDay:
    @discussion Get an array of WebHistoryItem that were last visited on the day represented by the
    specified NSCalendarDate, ordered from most recent to oldest.
    @param calendarDate A date identifying the unique day of interest.
    @result Returns an array of WebHistoryItems last visited on the indicated day.
*/
- (NSArray *)orderedItemsLastVisitedOnDay:(NSCalendarDate *)calendarDate;

/*!
    @method containsURL:
    @abstract Return whether a URL is in the WebHistory.
    @param URL The URL for which to search the WebHistory.
    @discussion This method is useful for implementing a visited-link mechanism.
    @result YES if WebHistory contains a history item for the given URL, otherwise NO.
*/
- (BOOL)containsURL:(NSURL *)URL;

/*!
    @method entryForURL:
    @abstract Get an item for a specific URL
    @param URL The URL of the history item to search for
    @result Returns an item matching the URL
*/
- (WebHistoryItem *)itemForURL:(NSURL *)URL;

/*!
    @method file
    @discussion The file path used for storing history, specified in -[WebHistory initWithFile:] or +[WebHistory webHistoryWithFile:]
    @result Returns the file path used to store the history.
*/
- (NSURL *)URL;

/*!
    @method loadHistory
    @discussion Load history from file. This happens automatically at init time, and need not normally be called.
    @result Returns YES if successful, NO otherwise.
*/
- (BOOL)loadHistory;

/*!
    @method saveHistory
    @discussion Save history to file. It is the client's responsibility to call this at appropriate times.
    @result Returns YES if successful, NO otherwise.
*/
- (BOOL)saveHistory;

@end
