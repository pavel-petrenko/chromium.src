/*
 * Copyright (C) 2004 Apple Computer, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#import "DOMInternal.h"

#import <dom/dom2_range.h>
#import <dom/dom_exception.h>
#import <dom/dom_string.h>
#import <xml/dom_stringimpl.h>

#import "KWQAssertions.h"

using DOM::DOMString;
using DOM::DOMStringImpl;
using DOM::RangeException;

//------------------------------------------------------------------------------------------
// Wrapping khtml implementation objects

static CFMutableDictionaryRef wrapperCache = NULL;

id getDOMWrapperForImpl(const void *impl)
{
    if (!wrapperCache)
        return nil;
    return (id)CFDictionaryGetValue(wrapperCache, impl);
}

void setDOMWrapperForImpl(id wrapper, const void *impl)
{
    if (!wrapperCache) {
        // No need to retain/free either impl key, or id value.  Items will be removed
        // from the cache in dealloc methods.
        wrapperCache = CFDictionaryCreateMutable(NULL, 0, NULL, NULL);
    }
    CFDictionarySetValue(wrapperCache, impl, wrapper);
}

void removeDOMWrapperForImpl(const void *impl)
{
    if (!wrapperCache)
        return;
    CFDictionaryRemoveValue(wrapperCache, impl);
}

//------------------------------------------------------------------------------------------
// Exceptions

NSString * const DOMException = @"DOMException";
NSString * const DOMRangeException = @"DOMRangeException";

void raiseDOMException(int code)
{
    ASSERT(code);
    
    NSString *name;
    if (code >= RangeException::_EXCEPTION_OFFSET) {
        name = DOMRangeException;
        code -= RangeException::_EXCEPTION_OFFSET;
    }
    else {
        name = DOMException;
    }
    NSString *reason = [NSString stringWithFormat:@"*** Exception received from DOM API: %d", code];
    NSException *exception = [NSException exceptionWithName:name reason:reason
        userInfo:[NSDictionary dictionaryWithObject:[NSNumber numberWithInt:code] forKey:name]];
    [exception raise];
}

//------------------------------------------------------------------------------------------
// DOMString/NSString bridging

DOMString::operator NSString *() const
{
    return [NSString stringWithCharacters:reinterpret_cast<const unichar *>(unicode()) length:length()];
}

DOMString::DOMString(NSString *str)
{
    ASSERT(str);

    CFIndex size = CFStringGetLength(reinterpret_cast<CFStringRef>(str));
    if (size == 0)
        impl = DOMStringImpl::empty();
    else {
        UniChar fixedSizeBuffer[1024];
        UniChar *buffer;
        if (size > static_cast<CFIndex>(sizeof(fixedSizeBuffer) / sizeof(UniChar))) {
            buffer = static_cast<UniChar *>(malloc(size * sizeof(UniChar)));
        } else {
            buffer = fixedSizeBuffer;
        }
        CFStringGetCharacters(reinterpret_cast<CFStringRef>(str), CFRangeMake(0, size), buffer);
        impl = new DOMStringImpl(reinterpret_cast<const QChar *>(buffer), (uint)size);
        if (buffer != fixedSizeBuffer) {
            free(buffer);
        }
    }
    impl->ref();
}

//------------------------------------------------------------------------------------------

