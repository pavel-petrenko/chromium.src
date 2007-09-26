/*
 * Copyright (C) 2007 Apple Inc. All rights reserved.
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

#include "WebKitInitializer.h"

// Needed to get SetDllDirectory.
#define _WIN32_WINNT 0x0502

#include <shlwapi.h>
#include <tchar.h>
#include <windows.h>

#define LOG(header, ...) \
    do { \
        _ftprintf(stderr, header); \
        _ftprintf(stderr, __VA_ARGS__); \
    } while (0)
#define LOG_WARNING(...) LOG(TEXT("WARNING: "), __VA_ARGS__)
#define LOG_ERROR(...) LOG(TEXT("ERROR: "), __VA_ARGS__)

#define DEBUGDLLSUFFIX TEXT("_debug")
#define RELEASEDLLSUFFIX

#define EITHERDLL(name, suffix) (TEXT( #name ) suffix TEXT(".dll"))
#define RELEASEDLL(name) EITHERDLL(name, RELEASEDLLSUFFIX)
#define DEBUGDLL(name) EITHERDLL(name, DEBUGDLLSUFFIX)

#ifdef USE_DEBUG_SUPPORT_LIBS
#define DLL(name) DEBUGDLL(name)
#else
#define DLL(name) RELEASEDLL(name)
#endif

static TCHAR* getStringValue(HKEY key, LPCTSTR valueName)
{
    DWORD type = 0;
    DWORD bufferSize = 0;
    if (RegQueryValueEx(key, valueName, 0, &type, 0, &bufferSize) != ERROR_SUCCESS || type != REG_SZ)
        return 0;

    TCHAR* buffer = (TCHAR*)malloc(bufferSize);
    if (RegQueryValueEx(key, 0, 0, &type, reinterpret_cast<LPBYTE>(buffer), &bufferSize) != ERROR_SUCCESS) {
        free(buffer);
        return 0;
    }

    return buffer;
}

static LPOLESTR getWebViewCLSID()
{
    // FIXME <rdar://5505062>: Once WebKit switches to truly version-independent
    // ProgIDs, this should just become "WebKit.WebView".
    LPCTSTR webViewProgID = TEXT("WebKit.WebView.3");

    CLSID clsid = CLSID_NULL;
    HRESULT hr = CLSIDFromProgID(webViewProgID, &clsid);
    if (FAILED(hr)) {
        LOG_WARNING(TEXT("Failed to get CLSID for %s\n"), webViewProgID);
        return 0;
    }

    LPOLESTR clsidString = 0;
    if (FAILED(StringFromCLSID(clsid, &clsidString))) {
        LOG_WARNING(TEXT("Failed to get string representation of CLSID for WebView\n"));
        return 0;
    }

    return clsidString;
}

static TCHAR* getInstalledWebKitDirectory()
{
    LPCTSTR keyPrefix = TEXT("SOFTWARE\\Classes\\CLSID\\");
    LPCTSTR keySuffix = TEXT("\\InprocServer32");

    LPOLESTR clsid = getWebViewCLSID();
    if (!clsid)
        return 0;

    size_t keyBufferLength = _tcslen(keyPrefix) + _tcslen(clsid) + _tcslen(keySuffix) + 1;
    TCHAR* keyString = (TCHAR*)malloc(keyBufferLength * sizeof(TCHAR));

    int ret = _sntprintf_s(keyString, keyBufferLength, keyBufferLength - 1, TEXT("%s%s%s"), keyPrefix, clsid, keySuffix);
    CoTaskMemFree(clsid);
    if (ret == -1) {
        LOG_WARNING(TEXT("Failed to construct InprocServer32 key\n"));
        return 0;
    }

    HKEY serverKey = 0;
    LONG error = RegOpenKeyEx(HKEY_LOCAL_MACHINE, keyString, 0, KEY_READ, &serverKey);
    free(keyString);
    if (error != ERROR_SUCCESS) {
        LOG_WARNING(TEXT("Failed to open registry key %s\n"), keyString);
        return 0;
    }

    TCHAR* webKitPath = getStringValue(serverKey, 0);
    RegCloseKey(serverKey);
    if (!webKitPath) {
        LOG_WARNING(TEXT("Couldn't retrieve value for registry key %s\n"), keyString);
        return 0;
    }

    TCHAR* startOfFileName = PathFindFileName(webKitPath);
    if (startOfFileName == webKitPath) {
        LOG_WARNING(TEXT("Couldn't find filename from path %s\n"), webKitPath);
        free(webKitPath);
        return 0;
    }

    *startOfFileName = '\0';
    return webKitPath;
}

bool initializeWebKit()
{
    static bool haveInitialized;
    static bool success;
    if (haveInitialized)
        return success;

    haveInitialized = true;

    if (TCHAR* directory = getInstalledWebKitDirectory()) {
        SetDllDirectory(directory);
        free(directory);
    } else
        LOG_WARNING(TEXT("Couldn't determine installed WebKit directory\n"));

    LPCTSTR webKitDependencies[] = {
        DLL(CFNetwork),
        DLL(CoreFoundation),
        DLL(CoreGraphics),
        DLL(SQLite3),
        DLL(SafariTheme),
        RELEASEDLL(icudt36),
        DLL(icuin36),
        DLL(icuuc36),
        DLL(libxml2),
        DLL(libxslt),
        DLL(pthreadVC2),
        DLL(zlib1),
    };

    for (int i = 0; i < ARRAYSIZE(webKitDependencies); ++i)
        if (!LoadLibrary(webKitDependencies[i])) {
            LOG_ERROR(TEXT("LoadLibrary(%s) failed\n"), webKitDependencies[i]);
            return false;
        }

    HMODULE webKitModule = LoadLibrary(DLL(WebKit));
    if (!webKitModule) {
        LOG_ERROR(TEXT("LoadLibrary(%s) failed\n"), DLL(WebKit));
        return false;
    }

    FARPROC dllRegisterServer = GetProcAddress(webKitModule, "DllRegisterServer");
    if (!dllRegisterServer) {
        LOG_ERROR(TEXT("GetProcAddress(webKitModule, \"DllRegisterServer\") failed\n"));
        return false;
    }

    dllRegisterServer();

    success = true;
    return success;
}
