/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
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

#include "config.h"
#include "public/platform/WebCryptoAlgorithm.h"

#include "public/platform/WebCryptoAlgorithmParams.h"
#include "wtf/OwnPtr.h"
#include "wtf/ThreadSafeRefCounted.h"

namespace WebKit {

class WebCryptoAlgorithmPrivate : public ThreadSafeRefCounted<WebCryptoAlgorithmPrivate> {
public:
    WebCryptoAlgorithmPrivate(WebCryptoAlgorithmId id, const char* name, PassOwnPtr<WebCryptoAlgorithmParams> params)
        : id(id)
        , name(name)
        , params(params)
    {
    }

    WebCryptoAlgorithmId id;
    const char* const name;
    OwnPtr<WebCryptoAlgorithmParams> params;
};

WebCryptoAlgorithm::WebCryptoAlgorithm(WebCryptoAlgorithmId id, const char* name, PassOwnPtr<WebCryptoAlgorithmParams> params)
    : m_private(adoptRef(new WebCryptoAlgorithmPrivate(id, name, params)))
{
}

WebCryptoAlgorithmId WebCryptoAlgorithm::id() const
{
    return m_private->id;
}

const char* WebCryptoAlgorithm::name() const
{
    return m_private->name;
}

WebCryptoAlgorithmParamsType WebCryptoAlgorithm::paramsType() const
{
    if (!m_private->params)
        return WebCryptoAlgorithmParamsTypeNone;
    return m_private->params->type();
}

WebCryptoAesCbcParams* WebCryptoAlgorithm::aesCbcParams() const
{
    if (paramsType() == WebCryptoAlgorithmParamsTypeAesCbcParams)
        return static_cast<WebCryptoAesCbcParams*>(m_private->params.get());
    return 0;
}

WebCryptoAesKeyGenParams* WebCryptoAlgorithm::aesKeyGenParams() const
{
    if (paramsType() == WebCryptoAlgorithmParamsTypeAesKeyGenParams)
        return static_cast<WebCryptoAesKeyGenParams*>(m_private->params.get());
    return 0;
}

void WebCryptoAlgorithm::assign(const WebCryptoAlgorithm& other)
{
    m_private = other.m_private;
}

void WebCryptoAlgorithm::reset()
{
    m_private.reset();
}

} // namespace WebKit
