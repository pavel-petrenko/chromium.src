/*
    WebDataProtocol.m
    Copyright 2003, Apple, Inc. All rights reserved.
*/
#import <WebKit/WebDataProtocol.h>

#import <WebFoundation/NSURLResponse.h>
#import <WebFoundation/NSURLResponsePrivate.h>
#import <WebFoundation/WebError.h>
#import <WebFoundation/NSURLProtocolClient.h>

NSString *WebDataProtocolScheme = @"applewebdata";

@interface WebDataRequestParameters : NSObject <NSCopying>
{
@public
    NSData *data;
    NSString *encoding;
    NSURL *baseURL;
}
@end

@implementation WebDataRequestParameters

-(id)copyWithZone:(NSZone *)zone
{
    WebDataRequestParameters *newInstance = [[WebDataRequestParameters allocWithZone:zone] init];
    newInstance->data = [data copyWithZone:zone];
    newInstance->encoding = [encoding copyWithZone:zone];
    newInstance->baseURL = [baseURL copyWithZone:zone];
    return newInstance;
}

- (void)dealloc
{
    [data release];
    [encoding release];
    [baseURL release];
    [super dealloc];
}

@end

@implementation NSURLRequest (WebDataRequest)

+ (NSURL *)_webDataRequestURLForData: (NSData *)data
{
    static unsigned int counter = 1;
    
    // The URL we generate is meaningless.  The only interesting properties of the URL
    // are it's scheme and that they be unique for the lifespan of the application.
    NSURL *fakeURL = [NSURL URLWithString: [NSString stringWithFormat: @"%@://%p", WebDataProtocolScheme, counter++, 0]];
    return fakeURL;
}

- (WebDataRequestParameters *)_webDataRequestParametersForReading
{
    Class theClass = [WebDataRequestParameters class];
    return [NSURLProtocol partOfRequest:self withClass:theClass createIfDoesNotExist:NO];
}

- (WebDataRequestParameters *)_webDataRequestParametersForWriting
{
    Class theClass = [WebDataRequestParameters class];
    return [NSURLProtocol partOfRequest:self withClass:theClass createIfDoesNotExist:YES];
}

- (NSData *)_webDataRequestData
{
    WebDataRequestParameters *parameters = [self _webDataRequestParametersForReading];
    return parameters ? parameters->data : nil;
}

- (void)_webDataRequestSetData: (NSData *)data
{
    WebDataRequestParameters *parameters = [self _webDataRequestParametersForWriting];
    [parameters->data release];
    parameters->data = [data retain];
}

- (NSString *)_webDataRequestEncoding
{
    WebDataRequestParameters *parameters = [self _webDataRequestParametersForReading];
    return parameters ? parameters->encoding: nil;
}

- (void)_webDataRequestSetEncoding:(NSString *)encoding
{
    WebDataRequestParameters *parameters = [self _webDataRequestParametersForWriting];
    [parameters->encoding release];
    parameters->encoding = [encoding retain];
}

- (NSURL *)_webDataRequestBaseURL
{
    WebDataRequestParameters *parameters = [self _webDataRequestParametersForReading];
    return parameters ? parameters->baseURL : nil;
}

- (void)_webDataRequestSetBaseURL:(NSURL *)baseURL
{
    WebDataRequestParameters *parameters = [self _webDataRequestParametersForWriting];
    [parameters->baseURL release];
    parameters->baseURL = [baseURL retain];
}

- (NSMutableURLRequest *)_webDataRequestExternalRequest
{
    WebDataRequestParameters *parameters = [self _webDataRequestParametersForReading];
    NSMutableURLRequest *newRequest = nil;
    
    if (parameters){
        newRequest = [[self mutableCopyWithZone: [self zone]] autorelease];
        NSURL *baseURL = [self _webDataRequestBaseURL];
        if (baseURL)
            [newRequest setURL: baseURL]; 
        else
            [newRequest setURL: [NSURL URLWithString: @"about:blank"]];
    } 
    return newRequest;
}


@end

// Implement the required methods for the concrete subclass of WebProtocolHandler
// that will handle our custom protocol.
@implementation WebDataProtocol

+ (void)load
{
    [NSURLProtocol registerClass: [self class]];
}

+ (BOOL)canHandleURL:(NSURL *)theURL
{
    return ([[theURL scheme] caseInsensitiveCompare: WebDataProtocolScheme] == NSOrderedSame);
}

+ (NSURL *)canonicalURLForURL:(NSURL *)URL
{
    return URL;
}

- (void)startLoadingWithCacheObject:(NSCachedURLResponse *)cacheObject
{
    NSObject<NSURLProtocolClient> *client = [self client];
    NSURLRequest *request = [self request];
    NSData *data = [request _webDataRequestData];

    if (data) {
        NSURLResponse *response = [[NSURLResponse alloc] init];
        [response setURL:[request URL]];
        [response setMIMEType:@"text/html"];
        [response setTextEncodingName:[request _webDataRequestEncoding]];
        [client responseAvailable:response];
        [client didLoadBytes:[data bytes] length:[data length]];
        [client finishedLoading];
        [response release];
    } else {
        int resultCode;

        resultCode = WebFoundationErrorResourceUnavailable;

        [client failedWithError:[WebError errorWithCode:resultCode inDomain:WebErrorDomainWebFoundation failingURL:[[request URL] absoluteString]]];
    }
}

- (void)stopLoading
{
}

@end

