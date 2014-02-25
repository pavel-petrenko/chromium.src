/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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

#ifndef MemoryInfo_h
#define MemoryInfo_h

#include "bindings/v8/ScriptGCEvent.h"
#include "bindings/v8/ScriptWrappable.h"
#include "heap/Handle.h"
#include "wtf/PassRefPtr.h"
#include "wtf/RefCounted.h"

namespace WebCore {

class Frame;

class MemoryInfo : public RefCountedWillBeGarbageCollectedFinalized<MemoryInfo>, public ScriptWrappable {
public:
    static PassRefPtrWillBeRawPtr<MemoryInfo> create(Frame* frame)
    {
        return adoptRefWillBeNoop(new MemoryInfo(frame));
    }

    size_t totalJSHeapSize() const { return m_info.totalJSHeapSize; }
    size_t usedJSHeapSize() const { return m_info.usedJSHeapSize; }
    size_t jsHeapSizeLimit() const { return m_info.jsHeapSizeLimit; }

    void trace(Visitor*) { }

private:
    explicit MemoryInfo(Frame*);

    HeapInfo m_info;
};

size_t quantizeMemorySize(size_t);

} // namespace WebCore

#endif // MemoryInfo_h
