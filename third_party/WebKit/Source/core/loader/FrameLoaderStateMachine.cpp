/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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
 * 3.  Neither the name of Google, Inc. nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
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

#include "config.h"
#include "core/loader/FrameLoaderStateMachine.h"

#include <wtf/Assertions.h>

namespace WebCore {

    
FrameLoaderStateMachine::FrameLoaderStateMachine() 
    : m_state(CreatingInitialEmptyDocument)
{ 
}
    
bool FrameLoaderStateMachine::committingFirstRealLoad() const 
{
    return m_state == DisplayingInitialEmptyDocument;
}

bool FrameLoaderStateMachine::committedFirstRealDocumentLoad() const 
{
    return m_state >= CommittedFirstRealLoad;
}

bool FrameLoaderStateMachine::creatingInitialEmptyDocument() const 
{
    return m_state == CreatingInitialEmptyDocument;
}

bool FrameLoaderStateMachine::isDisplayingInitialEmptyDocument() const 
{
    return m_state == DisplayingInitialEmptyDocument;
}

void FrameLoaderStateMachine::advanceTo(State state)
{
    ASSERT(State(m_state + 1) == state);
    m_state = state;
}

} // namespace WebCore
