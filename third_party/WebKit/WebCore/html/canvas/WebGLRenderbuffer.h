/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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

#ifndef WebGLRenderbuffer_h
#define WebGLRenderbuffer_h

#include "WebGLObject.h"

#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

namespace WebCore {
    
    class WebGLRenderbuffer : public WebGLObject {
    public:
        virtual ~WebGLRenderbuffer() { deleteObject(); }
        
        static PassRefPtr<WebGLRenderbuffer> create(WebGLRenderingContext*);

        void setInternalFormat(unsigned long internalformat) { m_internalFormat = internalformat; }
        unsigned long getInternalFormat() const { return m_internalFormat; }

        bool isInitialized() const { return m_initialized; }
        void setInitialized() { m_initialized = true; }

    protected:
        WebGLRenderbuffer(WebGLRenderingContext*);
        
        virtual void _deleteObject(Platform3DObject);

    private:
        virtual bool isRenderbuffer() const { return true; }

        unsigned long m_internalFormat;
        bool m_initialized;
    };
    
} // namespace WebCore

#endif // WebGLRenderbuffer_h
