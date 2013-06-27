/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef MemoryInstrumentationString_h
#define MemoryInstrumentationString_h

#include "wtf/MemoryInstrumentation.h"
#include "wtf/text/AtomicString.h"
#include "wtf/text/CString.h"
#include "wtf/text/StringBuilder.h"
#include "wtf/text/WTFString.h"

namespace WTF {

inline void reportMemoryUsage(const StringImpl* stringImpl, MemoryObjectInfo* memoryObjectInfo)
{
    size_t selfSize = sizeof(StringImpl);

    size_t length = stringImpl->length() + (stringImpl->hasTerminatingNullCharacter() ? 1 : 0);
    size_t bufferSize = length * (stringImpl->is8Bit() ? sizeof(LChar) : sizeof(UChar));
    const void* buffer = stringImpl->is8Bit() ? static_cast<const void*>(stringImpl->characters8()) : static_cast<const void*>(stringImpl->characters16());

    // Count size used by internal buffer but skip strings that were constructed from literals.
    if (stringImpl->hasInternalBuffer() && buffer == stringImpl + 1)
        selfSize += bufferSize;

    MemoryClassInfo info(memoryObjectInfo, stringImpl, 0, selfSize);

    if (StringImpl* baseString = stringImpl->baseString())
        info.addMember(baseString, "baseString", RetainingPointer);
    else {
        if (stringImpl->hasOwnedBuffer())
            info.addRawBuffer(buffer, bufferSize, "char[]", "ownedBuffer");

        if (stringImpl->has16BitShadow())
            info.addRawBuffer(stringImpl->bloatedCharacters(), length * sizeof(UChar), "UChar[]", "16bitShadow");
    }
}

inline void reportMemoryUsage(const String* string, MemoryObjectInfo* memoryObjectInfo)
{
    MemoryClassInfo info(memoryObjectInfo, string);
    info.addMember(string->impl(), "stringImpl", RetainingPointer);
}

template <typename CharType> class StringBuffer;
template <typename CharType>
inline void reportMemoryUsage(const StringBuffer<CharType>* stringBuffer, MemoryObjectInfo* memoryObjectInfo)
{
    MemoryClassInfo info(memoryObjectInfo, stringBuffer);
    info.addRawBuffer(stringBuffer->characters(), sizeof(CharType) * stringBuffer->length(), "CharType[]", "data");
}

inline void reportMemoryUsage(const AtomicString* atomicString, MemoryObjectInfo* memoryObjectInfo)
{
    MemoryClassInfo info(memoryObjectInfo, atomicString);
    info.addMember(atomicString->string(), "string");
}

inline void reportMemoryUsage(const CStringBuffer* cStringBuffer, MemoryObjectInfo* memoryObjectInfo)
{
    MemoryClassInfo info(memoryObjectInfo, cStringBuffer, 0, sizeof(*cStringBuffer) + cStringBuffer->length());
}

inline void reportMemoryUsage(const CString* cString, MemoryObjectInfo* memoryObjectInfo)
{
    MemoryClassInfo info(memoryObjectInfo, cString);
    info.addMember(cString->buffer(), "buffer", RetainingPointer);
}

}

#endif // !defined(MemoryInstrumentationVector_h)
