/*
 * Copyright (C) 2010 Google, Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef HTMLScriptRunner_h
#define HTMLScriptRunner_h

#include "PendingScript.h"
#include <wtf/Noncopyable.h>
#include <wtf/PassRefPtr.h>

namespace WebCore {

class CachedResource;
class CachedScript;
class Document;
class Element;
class Frame;
class HTMLScriptRunnerHost;
class ScriptSourceCode;

class HTMLScriptRunner : public Noncopyable {
public:
    HTMLScriptRunner(Document*, HTMLScriptRunnerHost*);
    ~HTMLScriptRunner();

    void detach();

    // Processes the passed in script and any pending scripts if possible.
    bool execute(PassRefPtr<Element> scriptToProcess, int scriptStartLine);

    bool executeScriptsWaitingForLoad(CachedResource*);
    bool hasScriptsWaitingForStylesheets() const { return m_hasScriptsWaitingForStylesheets; }
    bool executeScriptsWaitingForStylesheets();

    bool isExecutingScript() const { return !!m_scriptNestingLevel; }

private:
    Frame* frame() const;

    void executeParsingBlockingScript();
    void executePendingScriptAndDispatchEvent(PendingScript&);
    void executeScript(Element*, const ScriptSourceCode&) const;
    bool haveParsingBlockingScript() const;
    bool executeParsingBlockingScripts();

    void requestParsingBlockingScript(Element*);
    bool requestPendingScript(PendingScript&, Element*) const;

    void runScript(Element*, int startingLineNumber);

    // Helpers for dealing with HTMLScriptRunnerHost
    void watchForLoad(PendingScript&);
    void stopWatchingForLoad(PendingScript&);
    bool isPendingScriptReady(const PendingScript&);
    ScriptSourceCode sourceFromPendingScript(const PendingScript&, bool& errorOccurred) const;

    Document* m_document;
    HTMLScriptRunnerHost* m_host;
    PendingScript m_parsingBlockingScript;
    unsigned m_scriptNestingLevel;

    // We only want stylesheet loads to trigger script execution if script
    // execution is currently stopped due to stylesheet loads, otherwise we'd
    // cause nested script execution when parsing <style> tags since </style>
    // tags can cause Document to call executeScriptsWaitingForStylesheets.
    bool m_hasScriptsWaitingForStylesheets;
};

}

#endif
