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
#include "core/dom/DocumentLifecycle.h"

#include "wtf/Assertions.h"
#include <stdio.h>

namespace WebCore {

DocumentLifecycle::DocumentLifecycle()
    : m_state(Uninitialized)
{
}

DocumentLifecycle::~DocumentLifecycle()
{
}

bool DocumentLifecycle::canAdvanceTo(State state) const
{
    if (state > m_state)
        return true;
    // FIXME: We can dispose a document multiple times. This seems wrong.
    // See https://code.google.com/p/chromium/issues/detail?id=301668.
    if (m_state == Disposed)
        return state == Disposed;
    if (m_state == Clean) {
        // We can synchronously enter recalc style without rewinding to
        // StyleRecalcPending.
        return state == InStyleRecalc;
    }
    return false;
}

void DocumentLifecycle::advanceTo(State state)
{
    ASSERT(canAdvanceTo(state));
    m_state = state;
}

void DocumentLifecycle::rewindTo(State state)
{
    ASSERT(m_state == Clean);
    ASSERT(state < m_state);
    m_state = state;
    ASSERT(isActive());
}

}
