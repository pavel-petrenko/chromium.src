/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef V8ValueCache_h
#define V8ValueCache_h

#include "bindings/v8/ScopedPersistent.h"
#include "bindings/v8/UnsafePersistent.h"
#include <v8.h>
#include "wtf/HashMap.h"
#include "wtf/RefPtr.h"
#include "wtf/text/AtomicString.h"
#include "wtf/text/WTFString.h"

namespace WebCore {

class StringCache {
public:
    StringCache() { }

    v8::Handle<v8::String> v8ExternalString(StringImpl* stringImpl, v8::Isolate* isolate)
    {
        if (m_lastStringImpl.get() == stringImpl && m_lastV8String.isWeak())
            return m_lastV8String.newLocal(isolate);
        return v8ExternalStringSlow(stringImpl, isolate);
    }

    template <class T>
    void setReturnValueFromString(const T& info, StringImpl* stringImpl, v8::Isolate* isolate)
    {
        if (m_lastStringImpl.get() == stringImpl && m_lastV8String.isWeak())
            info.GetReturnValue().Set(*m_lastV8String.persistent());
        else
            setReturnValueFromStringSlow(info, stringImpl, isolate);
    }

    void clearOnGC() 
    {
        m_lastStringImpl = 0;
        m_lastV8String.clear();
    }

    void remove(StringImpl*);
    void reportMemoryUsage(MemoryObjectInfo*) const;

private:
    static v8::Local<v8::String> makeExternalString(const String&);
    static void makeWeakCallback(v8::Isolate*, v8::Persistent<v8::String>*, StringImpl*);

    v8::Handle<v8::String> v8ExternalStringSlow(StringImpl*, v8::Isolate*);

    template <class T>
    void setReturnValueFromStringSlow(const T& info, StringImpl* stringImpl, v8::Isolate* isolate)
    {
        if (!stringImpl->length()) {
            info.GetReturnValue().SetEmptyString();
            return;
        }

        UnsafePersistent<v8::String> cachedV8String = m_stringCache.get(stringImpl);
        if (cachedV8String.isWeak()) {
            m_lastStringImpl = stringImpl;
            m_lastV8String = cachedV8String;
            info.GetReturnValue().Set(*cachedV8String.persistent());
            return;
        }

        v8::Local<v8::String> newString = createStringAndInsertIntoCache(stringImpl, isolate);
        if (newString.IsEmpty())
            info.GetReturnValue().SetEmptyString();
        else
            info.GetReturnValue().Set(newString);
    }

    v8::Local<v8::String> createStringAndInsertIntoCache(StringImpl*, v8::Isolate*);

    HashMap<StringImpl*, UnsafePersistent<v8::String> > m_stringCache;
    UnsafePersistent<v8::String> m_lastV8String;
    // Note: RefPtr is a must as we cache by StringImpl* equality, not identity
    // hence lastStringImpl might be not a key of the cache (in sense of identity)
    // and hence it's not refed on addition.
    RefPtr<StringImpl> m_lastStringImpl;
};

} // namespace WebCore

#endif // V8ValueCache_h
