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

#include "config.h"
#include "JSCustomSQLStatementErrorCallback.h"

#include "CString.h"
#include "Frame.h"
#include "kjs_proxy.h"
#include "JSSQLError.h"
#include "JSSQLTransaction.h"
#include "Page.h"

namespace WebCore {
    
using namespace KJS;
    
JSCustomSQLStatementErrorCallback::JSCustomSQLStatementErrorCallback(JSObject* callback, Frame* frame)
    : m_callback(callback)
    , m_frame(frame)
{
}
    
bool JSCustomSQLStatementErrorCallback::handleEvent(SQLTransaction* transaction, SQLError* error)
{
    ASSERT(m_callback);
    ASSERT(m_frame);
        
    KJSProxy* proxy = m_frame->scriptProxy();
    if (!proxy)
        return true;
        
    ScriptInterpreter* interpreter = proxy->interpreter();
    ExecState* exec = interpreter->globalExec();
        
    KJS::JSLock lock;
        
    JSValue* handleEventFuncValue = m_callback->get(exec, "handleEvent");
    JSObject* handleEventFunc = 0;
    if (handleEventFuncValue->isObject()) {
        handleEventFunc = static_cast<JSObject*>(handleEventFuncValue);
        if (!handleEventFunc->implementsCall())
            handleEventFunc = 0;
    }
        
    if (!handleEventFunc && !m_callback->implementsCall()) {
        // FIXME: Should an exception be thrown here?
        return true;
    }
        
    RefPtr<JSCustomSQLStatementErrorCallback> protect(this);
        
    List args;
    args.append(toJS(exec, transaction));
    args.append(toJS(exec, error));
        
    JSValue *result;
    interpreter->startTimeoutCheck();
    if (handleEventFunc)
        result = handleEventFunc->call(exec, m_callback, args);
    else
        result = m_callback->call(exec, m_callback, args);
    interpreter->stopTimeoutCheck();
        
    if (exec->hadException()) {
        JSObject* exception = exec->exception()->toObject(exec);
        String message = exception->get(exec, exec->propertyNames().message)->toString(exec);
        int lineNumber = exception->get(exec, "line")->toInt32(exec);
        String sourceURL = exception->get(exec, "sourceURL")->toString(exec);
        if (Interpreter::shouldPrintExceptions())
            printf("SQLStatementErrorCallback: %s\n", message.utf8().data());
        if (Page* page = m_frame->page())
            page->chrome()->addMessageToConsole(JSMessageSource, ErrorMessageLevel, message, lineNumber, sourceURL);
        exec->clearException();
            
        // The spec says:
        // "If the error callback returns false, then move on to the next statement..."
        // "Otherwise, the error callback did not return false, or there was no error callback"
        // Therefore an exception and returning true are the same thing - so, return true on an exception
        return true;
    }
        
    Document::updateDocumentsRendering();

    return result->toBoolean(exec);
}

}
