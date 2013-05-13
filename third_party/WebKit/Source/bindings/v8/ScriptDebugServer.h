/*
 * Copyright (c) 2010, Google Inc. All rights reserved.
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

#ifndef ScriptDebugServer_h
#define ScriptDebugServer_h


#include "bindings/v8/ScopedPersistent.h"
#include "core/inspector/ScriptBreakpoint.h"
#include <v8-debug.h>
#include "wtf/HashMap.h"
#include "wtf/Noncopyable.h"
#include "wtf/PassOwnPtr.h"
#include "wtf/Vector.h"
#include "wtf/text/StringHash.h"
#include "wtf/text/WTFString.h"

namespace WebCore {

class ScriptDebugListener;
class ScriptObject;
class ScriptState;
class ScriptValue;

class ScriptDebugServer {
    WTF_MAKE_NONCOPYABLE(ScriptDebugServer);
public:
    String setBreakpoint(const String& sourceID, const ScriptBreakpoint&, int* actualLineNumber, int* actualColumnNumber);
    void removeBreakpoint(const String& breakpointId);
    void clearBreakpoints();
    void setBreakpointsActivated(bool activated);

    enum PauseOnExceptionsState {
        DontPauseOnExceptions,
        PauseOnAllExceptions,
        PauseOnUncaughtExceptions
    };
    PauseOnExceptionsState pauseOnExceptionsState();
    void setPauseOnExceptionsState(PauseOnExceptionsState pauseOnExceptionsState);

    void setPauseOnNextStatement(bool pause);
    void breakProgram();
    void continueProgram();
    void stepIntoStatement();
    void stepOverStatement();
    void stepOutOfFunction();

    bool setScriptSource(const String& sourceID, const String& newContent, bool preview, String* error, ScriptValue* newCallFrames, ScriptObject* result);
    void updateCallStack(ScriptValue* callFrame);

    void setScriptPreprocessor(const String& preprocessorBody);

    class Task {
    public:
        virtual ~Task() { }
        virtual void run() = 0;
    };
    static void interruptAndRun(PassOwnPtr<Task>, v8::Isolate*);
    void runPendingTasks();

    bool isPaused();
    bool runningNestedMessageLoop() { return m_runningNestedMessageLoop; }

    v8::Local<v8::Value> functionScopes(v8::Handle<v8::Function>);
    v8::Local<v8::Value> getInternalProperties(v8::Handle<v8::Object>&);
    v8::Local<v8::Value> setFunctionVariableValue(v8::Handle<v8::Value> functionValue, int scopeNumber, const String& variableName, v8::Handle<v8::Value> newValue);


    virtual void compileScript(ScriptState*, const String& expression, const String& sourceURL, String* scriptId, String* exceptionMessage);
    virtual void clearCompiledScripts();
    virtual void runScript(ScriptState*, const String& scriptId, ScriptValue* result, bool* wasThrown, String* exceptionMessage);

protected:
    explicit ScriptDebugServer(v8::Isolate*);
    virtual ~ScriptDebugServer();
    
    ScriptValue currentCallFrame();

    virtual ScriptDebugListener* getDebugListenerForContext(v8::Handle<v8::Context>) = 0;
    virtual void runMessageLoopOnPause(v8::Handle<v8::Context>) = 0;
    virtual void quitMessageLoopOnPause() = 0;

    static v8::Handle<v8::Value> breakProgramCallback(const v8::Arguments& args);
    void breakProgram(v8::Handle<v8::Object> executionState, v8::Handle<v8::Value> exception);

    static void v8DebugEventCallback(const v8::Debug::EventDetails& eventDetails);
    void handleV8DebugEvent(const v8::Debug::EventDetails& eventDetails);

    void dispatchDidParseSource(ScriptDebugListener* listener, v8::Handle<v8::Object> sourceObject);

    void ensureDebuggerScriptCompiled();

    v8::Local<v8::Value> callDebuggerMethod(const char* functionName, int argc, v8::Handle<v8::Value> argv[]);

    String preprocessSourceCode(const String& sourceCode);

    PauseOnExceptionsState m_pauseOnExceptionsState;
    ScopedPersistent<v8::Object> m_debuggerScript;
    ScopedPersistent<v8::Object> m_executionState;
    v8::Local<v8::Context> m_pausedContext;
    bool m_breakpointsActivated;
    ScopedPersistent<v8::FunctionTemplate> m_breakProgramCallbackTemplate;
    HashMap<String, OwnPtr<ScopedPersistent<v8::Script> > > m_compiledScripts;
    v8::Isolate* m_isolate;
    
private:
    class ScriptPreprocessor;
    OwnPtr<ScriptPreprocessor> m_scriptPreprocessor;
    bool m_runningNestedMessageLoop;
};

} // namespace WebCore


#endif // ScriptDebugServer_h
