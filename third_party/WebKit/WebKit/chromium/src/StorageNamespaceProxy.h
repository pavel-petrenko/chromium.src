/*
 * Copyright (C) 2009 Google Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY GOOGLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GOOGLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef StorageNamespaceProxy_h
#define StorageNamespaceProxy_h

#if ENABLE(DOM_STORAGE)

#include "StorageArea.h"
#include "StorageNamespace.h"

namespace WebKit { class WebStorageNamespace; }

namespace WebCore {

class StorageNamespaceProxy : public StorageNamespace {
public:
    StorageNamespaceProxy(WebKit::WebStorageNamespace*, StorageType);
    virtual ~StorageNamespaceProxy();
    virtual PassRefPtr<StorageArea> storageArea(PassRefPtr<SecurityOrigin>);
    virtual PassRefPtr<StorageNamespace> copy();
    virtual void close();
    virtual void unlock();

private:
    OwnPtr<WebKit::WebStorageNamespace> m_storageNamespace;
    StorageType m_storageType;
};

} // namespace WebCore

#endif // ENABLE(DOM_STORAGE)

#endif // StorageNamespaceProxy_h
