/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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


#include "core/inspector/InjectedScriptBase.h"

#include "bindings/v8/ScriptFunctionCall.h"
#include "bindings/v8/ScriptState.h"
#include "core/inspector/InspectorInstrumentation.h"
#include "platform/JSONValues.h"
#include "wtf/text/WTFString.h"

using WebCore::TypeBuilder::Runtime::RemoteObject;

namespace WebCore {

InjectedScriptBase::InjectedScriptBase(const String& name)
    : m_name(name)
    , m_inspectedStateAccessCheck(0)
{
}

InjectedScriptBase::InjectedScriptBase(const String& name, ScriptObject injectedScriptObject, InspectedStateAccessCheck accessCheck)
    : m_name(name)
    , m_injectedScriptObject(injectedScriptObject)
    , m_inspectedStateAccessCheck(accessCheck)
{
}

void InjectedScriptBase::initialize(ScriptObject injectedScriptObject, InspectedStateAccessCheck accessCheck)
{
    m_injectedScriptObject = injectedScriptObject;
    m_inspectedStateAccessCheck = accessCheck;
}

bool InjectedScriptBase::canAccessInspectedWindow() const
{
    return m_inspectedStateAccessCheck(m_injectedScriptObject.scriptState());
}

const ScriptObject& InjectedScriptBase::injectedScriptObject() const
{
    return m_injectedScriptObject;
}

ScriptValue InjectedScriptBase::callFunctionWithEvalEnabled(ScriptFunctionCall& function, bool& hadException) const
{
    ExecutionContext* executionContext = m_injectedScriptObject.scriptState()->executionContext();
    InspectorInstrumentationCookie cookie = InspectorInstrumentation::willCallFunction(executionContext, 0, name(), 1);

    ScriptState* scriptState = m_injectedScriptObject.scriptState();
    bool evalIsDisabled = false;
    if (scriptState) {
        evalIsDisabled = !scriptState->evalEnabled();
        // Temporarily enable allow evals for inspector.
        if (evalIsDisabled)
            scriptState->setEvalEnabled(true);
    }

    ScriptValue resultValue = function.call(hadException);

    if (evalIsDisabled)
        scriptState->setEvalEnabled(false);

    InspectorInstrumentation::didCallFunction(cookie);
    return resultValue;
}

void InjectedScriptBase::makeCall(ScriptFunctionCall& function, RefPtr<JSONValue>* result)
{
    if (hasNoValue() || !canAccessInspectedWindow()) {
        *result = JSONValue::null();
        return;
    }

    bool hadException = false;
    ScriptValue resultValue = callFunctionWithEvalEnabled(function, hadException);

    ASSERT(!hadException);
    if (!hadException) {
        *result = resultValue.toJSONValue(m_injectedScriptObject.scriptState());
        if (!*result)
            *result = JSONString::create(String::format("Object has too long reference chain(must not be longer than %d)", JSONValue::maxDepth));
    } else {
        *result = JSONString::create("Exception while making a call.");
    }
}

void InjectedScriptBase::makeEvalCall(ErrorString* errorString, ScriptFunctionCall& function, RefPtr<TypeBuilder::Runtime::RemoteObject>* objectResult, TypeBuilder::OptOutput<bool>* wasThrown)
{
    RefPtr<JSONValue> result;
    makeCall(function, &result);
    if (!result) {
        *errorString = "Internal error: result value is empty";
        return;
    }
    if (result->type() == JSONValue::TypeString) {
        result->asString(errorString);
        ASSERT(errorString->length());
        return;
    }
    RefPtr<JSONObject> resultPair = result->asObject();
    if (!resultPair) {
        *errorString = "Internal error: result is not an Object";
        return;
    }
    RefPtr<JSONObject> resultObj = resultPair->getObject("result");
    bool wasThrownVal = false;
    if (!resultObj || !resultPair->getBoolean("wasThrown", &wasThrownVal)) {
        *errorString = "Internal error: result is not a pair of value and wasThrown flag";
        return;
    }
    *objectResult = TypeBuilder::Runtime::RemoteObject::runtimeCast(resultObj);
    *wasThrown = wasThrownVal;
}

} // namespace WebCore

