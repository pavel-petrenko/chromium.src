/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#include "NetscapePluginModule.h"

namespace WebKit {

void NetscapePluginModule::unload()
{
    if (m_bundle) {
        CFBundleUnloadExecutable(m_bundle.get());
        m_bundle = 0;
    }
}

template<typename FuncType> 
static inline FuncType pointerToFunction(CFBundleRef bundle, const char* functionName)
{
    RetainPtr<CFStringRef> functionNameString(AdoptCF, CFStringCreateWithCStringNoCopy(kCFAllocatorDefault, functionName, kCFStringEncodingASCII, kCFAllocatorNull));
    return reinterpret_cast<FuncType>(CFBundleGetFunctionPointerForName(bundle, functionNameString.get()));
}
    
bool NetscapePluginModule::tryLoad()
{
    RetainPtr<CFStringRef> bundlePath(AdoptCF, m_pluginPath.createCFString());
    RetainPtr<CFURLRef> bundleURL(AdoptCF, CFURLCreateWithFileSystemPath(0, bundlePath.get(), kCFURLPOSIXPathStyle, FALSE));
    if (!bundleURL)
        return false;

    m_bundle.adoptCF(CFBundleCreate(kCFAllocatorDefault, bundleURL.get()));
    if (!m_bundle)
        return false;
    
    if (!CFBundleLoadExecutable(m_bundle.get()))
        return false;

    NP_InitializeFuncPtr initializeFuncPtr = pointerToFunction<NP_InitializeFuncPtr>(m_bundle.get(), "NP_Initialize");
    if (!initializeFuncPtr)
        return false;

    NP_GetEntryPointsFuncPtr getEntryPointsFuncPtr = pointerToFunction<NP_GetEntryPointsFuncPtr>(m_bundle.get(), "NP_GetEntryPoints");
    if (!getEntryPointsFuncPtr)
        return false;

    m_shutdownProcPtr = pointerToFunction<NPP_ShutdownProcPtr>(m_bundle.get(), "NP_Shutdown");
    if (!m_shutdownProcPtr)
        return false;

    return true;
}

} // namespace WebKit
