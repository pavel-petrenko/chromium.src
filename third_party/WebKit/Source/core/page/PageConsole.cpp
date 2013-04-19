/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

#include "config.h"
#include "PageConsole.h"

#include "Chrome.h"
#include "ChromeClient.h"
#include "ConsoleAPITypes.h"
#include "ConsoleTypes.h"
#include "Document.h"
#include "Frame.h"
#include "InspectorConsoleInstrumentation.h"
#include "InspectorController.h"
#include "Page.h"
#include "ScriptArguments.h"
#include "ScriptCallStack.h"
#include "ScriptCallStackFactory.h"
#include "ScriptValue.h"
#include "ScriptableDocumentParser.h"
#include "Settings.h"
#include <stdio.h>
#include <wtf/UnusedParam.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

#include "TraceEvent.h"

namespace WebCore {

namespace {

int muteCount = 0;

// Ensure that this stays in sync with the DeprecatedFeature enum.
static const char* const deprecationMessages[] = {
    "The 'X-WebKit-CSP' headers are deprecated; please consider using the canonical 'Content-Security-Policy' header instead.",

    // HTMLMediaElement
    "'HTMLMediaElement.webkitAddKey()' is deprecated. Please use 'MediaKeySession.update()' instead.",
    "'HTMLMediaElement.webkitGenerateKeyRequest()' is deprecated. Please use 'MediaKeys.createSession()' instead.",

    // Quota
    "'window.webkitStorageInfo' is deprecated. Please use 'navigator.webkitTemporaryStorage' or 'navigator.webkitPersistentStorage' instead.",

    // Web Audio
    "AudioBufferSourceNode 'looping' attribute is deprecated.  Use 'loop' instead.",
};

COMPILE_ASSERT(WTF_ARRAY_LENGTH(deprecationMessages) == static_cast<int>(WebCore::PageConsole::NumberOfFeatures), DeprecationMessages_matches_enum);

}

PageConsole::PageConsole(Page* page)
    : m_page(page)
    , m_deprecationNotifications(NumberOfFeatures)
{
}

PageConsole::~PageConsole() { }

void PageConsole::addMessage(MessageSource source, MessageLevel level, const String& message, unsigned long requestIdentifier, Document* document)
{
    String url;
    if (document)
        url = document->url().string();
    unsigned line = 0;
    if (document && document->parsing() && !document->isInDocumentWrite() && document->scriptableDocumentParser()) {
        ScriptableDocumentParser* parser = document->scriptableDocumentParser();
        if (!parser->isWaitingForScripts() && !parser->isExecutingScript())
            line = parser->lineNumber().oneBasedInt();
    }
    addMessage(source, level, message, url, line, 0, 0, requestIdentifier);
}

void PageConsole::addMessage(MessageSource source, MessageLevel level, const String& message, PassRefPtr<ScriptCallStack> callStack)
{
    addMessage(source, level, message, String(), 0, callStack, 0);
}

void PageConsole::addMessage(MessageSource source, MessageLevel level, const String& message, const String& url, unsigned lineNumber, PassRefPtr<ScriptCallStack> callStack, ScriptState* state, unsigned long requestIdentifier)
{
    if (muteCount && source != ConsoleAPIMessageSource)
        return;

    Page* page = this->page();
    if (!page)
        return;

    if (callStack)
        InspectorInstrumentation::addMessageToConsole(page, source, LogMessageType, level, message, callStack, requestIdentifier);
    else
        InspectorInstrumentation::addMessageToConsole(page, source, LogMessageType, level, message, url, lineNumber, state, requestIdentifier);

    if (source == CSSMessageSource)
        return;

    page->chrome()->client()->addMessageToConsole(source, level, message, lineNumber, url);
}

// static
void PageConsole::mute()
{
    muteCount++;
}

// static
void PageConsole::unmute()
{
    ASSERT(muteCount > 0);
    muteCount--;
}

// static
void PageConsole::reportDeprecation(ScriptExecutionContext* context, DeprecatedFeature feature)
{
    if (!context || !context->isDocument())
        return;
    PageConsole::reportDeprecation(toDocument(context), feature);
}

// static
void PageConsole::reportDeprecation(Document* document, DeprecatedFeature feature)
{
    if (!document)
        return;

    Page* page = document->page();
    if (!page || !page->console())
        return;

    page->console()->addDeprecationMessage(feature);
}

void PageConsole::addDeprecationMessage(DeprecatedFeature feature)
{
    ASSERT(feature < NumberOfFeatures);

    if (m_deprecationNotifications.quickGet(feature))
        return;

    m_deprecationNotifications.quickSet(feature);
    addMessage(DeprecationMessageSource, WarningMessageLevel, ASCIILiteral(deprecationMessages[feature]));
}

} // namespace WebCore
