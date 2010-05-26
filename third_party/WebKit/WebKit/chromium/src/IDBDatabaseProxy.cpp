/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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

#include "config.h"
#include "IDBDatabaseProxy.h"

#include "DOMStringList.h"
#include "IDBDatabaseError.h"
#include "WebIDBDatabase.h"
#include "WebIDBDatabaseError.h"

#if ENABLE(INDEXED_DATABASE)

namespace WebCore {

PassRefPtr<IDBDatabase> IDBDatabaseProxy::create(PassOwnPtr<WebKit::WebIDBDatabase> database)
{
    return adoptRef(new IDBDatabaseProxy(database));
}

IDBDatabaseProxy::IDBDatabaseProxy(PassOwnPtr<WebKit::WebIDBDatabase> database)
    : m_webIDBDatabase(database)
{
}

IDBDatabaseProxy::~IDBDatabaseProxy()
{
}

String IDBDatabaseProxy::name()
{
    return m_webIDBDatabase->name();
}

String IDBDatabaseProxy::description()
{
    return m_webIDBDatabase->description();
}

String IDBDatabaseProxy::version()
{
    return m_webIDBDatabase->version();
}

PassRefPtr<DOMStringList> IDBDatabaseProxy::objectStores()
{
    WebKit::WebVector<WebKit::WebString> webStrings = m_webIDBDatabase->objectStores();
    RefPtr<DOMStringList> strings = DOMStringList::create();
    for (size_t i = 0; i < webStrings.size(); ++i)
        strings->append(webStrings[i]);
    return strings.release();
}

} // namespace WebCore

#endif // ENABLE(INDEXED_DATABASE)

