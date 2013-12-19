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

#ifndef Console_h
#define Console_h

#include "bindings/v8/ScriptState.h"
#include "bindings/v8/ScriptWrappable.h"
#include "core/frame/ConsoleBase.h"
#include "core/frame/DOMWindowProperty.h"
#include "wtf/Forward.h"
#include "wtf/PassRefPtr.h"
#include "wtf/RefCounted.h"

namespace WebCore {

class Frame;
class MemoryInfo;
class Page;
class ScriptArguments;

class Console : public RefCounted<Console>, public ConsoleBase, public ScriptWrappable, public DOMWindowProperty {
public:
    using RefCounted<Console>::ref;
    using RefCounted<Console>::deref;

    static PassRefPtr<Console> create(Frame* frame) { return adoptRef(new Console(frame)); }
    virtual ~Console();

    PassRefPtr<MemoryInfo> memory() const;

protected:
    virtual ExecutionContext* context();
    virtual void reportMessageToClient(MessageLevel, const String& message, PassRefPtr<ScriptCallStack>) OVERRIDE;

private:
    explicit Console(Frame*);

    virtual void refConsole() { ref(); }
    virtual void derefConsole() { deref(); }
};

} // namespace WebCore

#endif // Console_h
