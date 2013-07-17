/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
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
 * 3.  Neither the name of Google, Inc. ("Google") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GOOGLE AND ITS CONTRIBUTORS "AS IS" AND ANY
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

#ifndef SubtleCrypto_h
#define SubtleCrypto_h

#include "bindings/v8/ScriptWrappable.h"
#include "wtf/Forward.h"
#include "wtf/PassRefPtr.h"
#include "wtf/RefCounted.h"

namespace WebCore {

class CryptoOperation;
class Dictionary;
class ExceptionState;

class SubtleCrypto : public ScriptWrappable, public RefCounted<SubtleCrypto> {
public:
    static PassRefPtr<SubtleCrypto> create() { return adoptRef(new SubtleCrypto()); }

    PassRefPtr<CryptoOperation> encrypt(const Dictionary&, ExceptionState&);
    PassRefPtr<CryptoOperation> decrypt(const Dictionary&, ExceptionState&);
    PassRefPtr<CryptoOperation> sign(const Dictionary&, ExceptionState&);
    // Note that this is not named "verify" because when compiling on Mac that expands to a macro and breaks.
    PassRefPtr<CryptoOperation> verifySignature(const Dictionary&, ExceptionState&);
    PassRefPtr<CryptoOperation> digest(const Dictionary&, ExceptionState&);

private:
    SubtleCrypto();
};

} // namespace WebCore

#endif
