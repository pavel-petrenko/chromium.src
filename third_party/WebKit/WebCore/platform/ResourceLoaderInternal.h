// -*- mode: c++; c-basic-offset: 4 -*-
/*
 * Copyright (C) 2004, 2006 Apple Computer, Inc.  All rights reserved.
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

#ifndef ResourceLoaderInternal_h
#define ResourceLoaderInternal_h

#include "FormData.h"
#include "KURL.h"
#include <wtf/HashMap.h>
#include <wtf/Platform.h>

#if USE(CFNETWORK)
#include <CFNetwork/CFURLConnectionPriv.h>
#endif

#if USE(WININET)
#include <windows.h>
#endif

#if PLATFORM(GDK)
typedef void CURL;
#endif

#if PLATFORM(QT)
#include <QString>
#endif

// The allocations and releases in ResourceLoaderInternal are
// Cocoa-exception-free (either simple Foundation classes or
// WebCoreResourceLoaderImp which avoids doing work in dealloc).

namespace WebCore {

    class ResourceLoaderInternal
    {
    public:
        ResourceLoaderInternal(ResourceLoader* job, ResourceLoaderClient* c, const String& method, const KURL& u)
            : client(c)
            , status(0)
            , URL(u)
            , method(method)
            , assembledResponseHeaders(true)
            , retrievedCharset(true)
#if USE(CFNETWORK)
            , m_connection(0)
#elif PLATFORM(MAC)
            , loader(nil)
            , response(nil)
#endif
#if USE(WININET)
            , m_fileHandle(INVALID_HANDLE_VALUE)
            , m_fileLoadTimer(job, &ResourceLoader::fileLoadTimer)
            , m_resourceHandle(0)
            , m_secondaryHandle(0)
            , m_jobId(0)
            , m_threadId(0)
            , m_writing(false)
            , m_formDataString(0)
            , m_formDataLength(0)
            , m_bytesRemainingToWrite(0)
#endif
#if PLATFORM(GDK)
            , m_handle(0)
#endif
        {
        }
        
        ResourceLoaderInternal(ResourceLoader* job, ResourceLoaderClient* c, const String& method, const KURL& u, const FormData& p)
            : client(c)
            , status(0)
            , URL(u)
            , method(method)
            , postData(p)
            , assembledResponseHeaders(true)
            , retrievedCharset(true)
#if USE(CFNETWORK)
            , m_connection(0)
#elif PLATFORM(MAC)
            , loader(nil)
            , response(nil)
#endif
#if USE(WININET)
            , m_fileHandle(INVALID_HANDLE_VALUE)
            , m_fileLoadTimer(job, &ResourceLoader::fileLoadTimer)
            , m_resourceHandle(0)
            , m_secondaryHandle(0)
            , m_jobId(0)
            , m_threadId(0)
            , m_writing(false)
            , m_formDataString(0)
            , m_formDataLength(0)
            , m_bytesRemainingToWrite(0)
#endif
#if PLATFORM(GDK)
            , m_handle(0)
#endif
        {
        }

        ~ResourceLoaderInternal();
        
        ResourceLoaderClient* client;
        
        int status;
        HashMap<String, String> metaData;
        KURL URL;
        String method;
        FormData postData;
        
        bool assembledResponseHeaders;
        bool retrievedCharset;
        DeprecatedString responseHeaders;
        
#if USE(CFNETWORK)
        CFURLConnectionRef m_connection;
#elif PLATFORM(MAC)
        WebCoreResourceLoaderImp* loader;
        NSURLResponse* response;
#endif
#if USE(WININET)
        HANDLE m_fileHandle;
        Timer<ResourceLoader> m_fileLoadTimer;
        HINTERNET m_resourceHandle;
        HINTERNET m_secondaryHandle;
        unsigned m_jobId;
        DWORD m_threadId;
        bool m_writing;
        char* m_formDataString;
        int m_formDataLength;
        int m_bytesRemainingToWrite;
        String m_postReferrer;
#endif
#if PLATFORM(GDK)
        CURL *m_handle;
#endif
#if PLATFORM(QT)
        QString m_mimetype;
        QString m_response;
#endif
        };

} // namespace WebCore

#endif // ResourceLoaderInternal_h
