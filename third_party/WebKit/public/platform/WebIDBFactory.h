/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WebIDBFactory_h
#define WebIDBFactory_h

#include "WebCommon.h"
#include "WebIDBCallbacks.h"
#include "WebIDBMetadata.h"
#include "WebString.h"
#include "WebVector.h"

namespace WebKit {

class WebIDBDatabase;
class WebIDBDatabaseCallbacks;

// The entry point into the IndexedDatabase API.  These classes match their Foo and
// FooSync counterparts in the spec, but operate only in an async manner.
// http://dev.w3.org/2006/webapi/WebSimpleDB/
class WebIDBFactory {
public:
    virtual ~WebIDBFactory() { }

    virtual void getDatabaseNames(WebIDBCallbacks* callbacks, const WebString& databaseIdentifier) { WEBKIT_ASSERT_NOT_REACHED(); }
    virtual void open(const WebString& name, long long version, long long transactionId, WebIDBCallbacks* callbacks, WebIDBDatabaseCallbacks* databaseCallbacks, const WebString& databaseIdentifier) { WEBKIT_ASSERT_NOT_REACHED(); }
    virtual void deleteDatabase(const WebString& name, WebIDBCallbacks* callbacks, const WebString& databaseIdentifier) { WEBKIT_ASSERT_NOT_REACHED(); }
};

} // namespace WebKit

#endif // WebIDBFactory_h
