/*	
        IFWebFrame.h
	    
	    Copyright 2001, Apple, Inc. All rights reserved.

        Public header file.
*/

#import <Foundation/Foundation.h>

@class IFError;
@class IFWebDataSource;
@class IFWebController;

@class IFWebFramePrivate;

@interface IFWebFrame : NSObject
{
@private
    IFWebFramePrivate *_private;
}

- initWithName: (NSString *)name view: view provisionalDataSource: (IFWebDataSource *)dataSource controller: (IFWebController *)controller;
- (NSString *)name;

- (void)setController: (IFWebController *)controller;
- (IFWebController *)controller;

- (void)setView: (id)view;
- (id)view;

/*
    Sets the frame's data source.  Note that the data source will be
    considered 'provisional' until it has been loaded, and at least
    ~some~ data has been received.
    
    Will return NO and not set the provisional data source if the controller
    disallows by returning a IFURLPolicyIgnore.
*/
- (BOOL)setProvisionalDataSource: (IFWebDataSource *)ds;

/*
    Returns the committed data source.  Will return nil if the
    provisional data source hasn't yet been loaded.
*/
- (IFWebDataSource *)dataSource;

/*
    Will return the provisional data source.  The provisional data source will
    return nil if no data source has been set on the frame, or the data source
    has successfully transitioned to the committed data source.
*/
- (IFWebDataSource *)provisionalDataSource;


/*
    If a frame has a provisional data source this method will begin
    loading data for that provisional data source.  If the frame
    has no provisional data source this method will do nothing.

    To reload an existing data source call reload.
*/
- (void)startLoading;


/*
    Stop any pending loads on the frame's data source,
    and it's children.
*/
- (void)stopLoading;


/*
*/
- (void)reload: (BOOL)forceRefresh;


/*
    This method removes references the underlying resources.
    FIXME:  I think this should be private.
*/
- (void)reset;

/*
    This method returns a frame with the given name. frameNamed returns self 
    for _self and _current, the parent frame for _parent and the main frame for _top. 
    frameNamed returns self for _parent and _top if the receiver it is the mainFrame. 
    nil is returned if a frame with the given name is not found.
*/
- (IFWebFrame *)frameNamed:(NSString *)name;

@end
