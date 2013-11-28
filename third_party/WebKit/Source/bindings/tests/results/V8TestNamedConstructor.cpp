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

// This file has been auto-generated by code_generator_v8.pm. DO NOT MODIFY!

#include "config.h"
#include "V8TestNamedConstructor.h"

#include "RuntimeEnabledFeatures.h"
#include "V8Document.h"
#include "bindings/v8/ExceptionMessages.h"
#include "bindings/v8/ExceptionState.h"
#include "bindings/v8/ScriptController.h"
#include "bindings/v8/V8Binding.h"
#include "bindings/v8/V8DOMConfiguration.h"
#include "bindings/v8/V8DOMWrapper.h"
#include "bindings/v8/V8ObjectConstructor.h"
#include "core/dom/ContextFeatures.h"
#include "core/dom/Document.h"
#include "platform/TraceEvent.h"
#include "wtf/UnusedParam.h"

namespace WebCore {

static void initializeScriptWrappableForInterface(TestNamedConstructor* object)
{
    if (ScriptWrappable::wrapperCanBeStoredInObject(object))
        ScriptWrappable::setTypeInfoInObject(object, &V8TestNamedConstructor::wrapperTypeInfo);
    else
        ASSERT_NOT_REACHED();
}

} // namespace WebCore

// In ScriptWrappable::init, the use of a local function declaration has an issue on Windows:
// the local declaration does not pick up the surrounding namespace. Therefore, we provide this function
// in the global namespace.
// (More info on the MSVC bug here: http://connect.microsoft.com/VisualStudio/feedback/details/664619/the-namespace-of-local-function-declarations-in-c)
void webCoreInitializeScriptWrappableForInterface(WebCore::TestNamedConstructor* object)
{
    WebCore::initializeScriptWrappableForInterface(object);
}

namespace WebCore {
const WrapperTypeInfo V8TestNamedConstructor::wrapperTypeInfo = { gin::kEmbedderBlink, V8TestNamedConstructor::GetTemplate, V8TestNamedConstructor::derefObject, V8TestNamedConstructor::toActiveDOMObject, 0, 0, V8TestNamedConstructor::installPerContextEnabledMethods, 0, WrapperTypeObjectPrototype };

namespace TestNamedConstructorV8Internal {

template <typename T> void V8_USE(T) { }

} // namespace TestNamedConstructorV8Internal

const WrapperTypeInfo V8TestNamedConstructorConstructor::wrapperTypeInfo = { gin::kEmbedderBlink, V8TestNamedConstructorConstructor::GetTemplate, V8TestNamedConstructor::derefObject, V8TestNamedConstructor::toActiveDOMObject, 0, 0, V8TestNamedConstructor::installPerContextEnabledMethods, 0, WrapperTypeObjectPrototype };

static void V8TestNamedConstructorConstructorCallback(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    if (!info.IsConstructCall()) {
        throwTypeError(ExceptionMessages::failedToConstruct("Audio", "Please use the 'new' operator, this DOM object constructor cannot be called as a function."), info.GetIsolate());
        return;
    }

    if (ConstructorMode::current() == ConstructorMode::WrapExistingObject) {
        info.GetReturnValue().Set(info.Holder());
        return;
    }

    Document* document = currentDocument();
    ASSERT(document);

    // Make sure the document is added to the DOM Node map. Otherwise, the TestNamedConstructor instance
    // may end up being the only node in the map and get garbage-collected prematurely.
    toV8(document, info.Holder(), info.GetIsolate());

    if (UNLIKELY(info.Length() < 1)) {
        throwTypeError(ExceptionMessages::failedToExecute("NamedConstructor", "TestNamedConstructor", ExceptionMessages::notEnoughArguments(1, info.Length())), info.GetIsolate());
        return;
    }
    ExceptionState exceptionState(info.Holder(), info.GetIsolate());
    V8TRYCATCH_FOR_V8STRINGRESOURCE_VOID(V8StringResource<>, str1, info[0]);
    V8TRYCATCH_FOR_V8STRINGRESOURCE_VOID(V8StringResource<>, str2, info[1]);
    V8TRYCATCH_FOR_V8STRINGRESOURCE_VOID(V8StringResource<>, str3, argumentOrNull(info, 2));

    RefPtr<TestNamedConstructor> impl = TestNamedConstructor::createForJSConstructor(*document, str1, str2, str3, exceptionState);
    v8::Handle<v8::Object> wrapper = info.Holder();
    if (exceptionState.throwIfNeeded())
        return;

    V8DOMWrapper::associateObjectWithWrapper<V8TestNamedConstructor>(impl.release(), &V8TestNamedConstructorConstructor::wrapperTypeInfo, wrapper, info.GetIsolate(), WrapperConfiguration::Dependent);
    info.GetReturnValue().Set(wrapper);
}

v8::Handle<v8::FunctionTemplate> V8TestNamedConstructorConstructor::GetTemplate(v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    // This is only for getting a unique pointer which we can pass to privateTemplate.
    static int privateTemplateUniqueKey;
    V8PerIsolateData* data = V8PerIsolateData::from(isolate);
    v8::Handle<v8::FunctionTemplate> result = data->privateTemplateIfExists(currentWorldType, &privateTemplateUniqueKey);
    if (!result.IsEmpty())
        return result;

    TRACE_EVENT_SCOPED_SAMPLING_STATE("Blink", "BuildDOMTemplate");
    v8::HandleScope scope(isolate);
    result = v8::FunctionTemplate::New(isolate, V8TestNamedConstructorConstructorCallback);

    v8::Local<v8::ObjectTemplate> instanceTemplate = result->InstanceTemplate();
    instanceTemplate->SetInternalFieldCount(V8TestNamedConstructor::internalFieldCount);
    result->SetClassName(v8::String::NewFromUtf8(isolate, "TestNamedConstructor", v8::String::kInternalizedString));
    result->Inherit(V8TestNamedConstructor::GetTemplate(isolate, currentWorldType));
    data->setPrivateTemplate(currentWorldType, &privateTemplateUniqueKey, result);

    return scope.Close(result);
}

static v8::Handle<v8::FunctionTemplate> ConfigureV8TestNamedConstructorTemplate(v8::Handle<v8::FunctionTemplate> functionTemplate, v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    functionTemplate->ReadOnlyPrototype();

    v8::Local<v8::Signature> defaultSignature;
    defaultSignature = V8DOMConfiguration::installDOMClassTemplate(functionTemplate, "TestNamedConstructor", v8::Local<v8::FunctionTemplate>(), V8TestNamedConstructor::internalFieldCount,
        0, 0,
        0, 0,
        0, 0,
        isolate, currentWorldType);
    UNUSED_PARAM(defaultSignature);
    v8::Local<v8::ObjectTemplate> instanceTemplate = functionTemplate->InstanceTemplate();
    v8::Local<v8::ObjectTemplate> prototypeTemplate = functionTemplate->PrototypeTemplate();
    UNUSED_PARAM(instanceTemplate);
    UNUSED_PARAM(prototypeTemplate);

    // Custom toString template
    functionTemplate->Set(v8::String::NewFromUtf8(isolate, "toString", v8::String::kInternalizedString), V8PerIsolateData::current()->toStringTemplate());
    return functionTemplate;
}

v8::Handle<v8::FunctionTemplate> V8TestNamedConstructor::GetTemplate(v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    V8PerIsolateData* data = V8PerIsolateData::from(isolate);
    V8PerIsolateData::TemplateMap::iterator result = data->templateMap(currentWorldType).find(&wrapperTypeInfo);
    if (result != data->templateMap(currentWorldType).end())
        return result->value.newLocal(isolate);

    TRACE_EVENT_SCOPED_SAMPLING_STATE("Blink", "BuildDOMTemplate");
    v8::EscapableHandleScope handleScope(isolate);
    v8::Local<v8::FunctionTemplate> templ =
        ConfigureV8TestNamedConstructorTemplate(data->rawTemplate(&wrapperTypeInfo, currentWorldType), isolate, currentWorldType);
    data->templateMap(currentWorldType).add(&wrapperTypeInfo, UnsafePersistent<v8::FunctionTemplate>(isolate, templ));
    return handleScope.Escape(templ);
}

bool V8TestNamedConstructor::hasInstance(v8::Handle<v8::Value> jsValue, v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    return V8PerIsolateData::from(isolate)->hasInstance(&wrapperTypeInfo, jsValue, currentWorldType);
}

bool V8TestNamedConstructor::hasInstanceInAnyWorld(v8::Handle<v8::Value> jsValue, v8::Isolate* isolate)
{
    return V8PerIsolateData::from(isolate)->hasInstance(&wrapperTypeInfo, jsValue, MainWorld)
        || V8PerIsolateData::from(isolate)->hasInstance(&wrapperTypeInfo, jsValue, IsolatedWorld)
        || V8PerIsolateData::from(isolate)->hasInstance(&wrapperTypeInfo, jsValue, WorkerWorld);
}

ActiveDOMObject* V8TestNamedConstructor::toActiveDOMObject(v8::Handle<v8::Object> wrapper)
{
    return toNative(wrapper);
}

v8::Handle<v8::Object> V8TestNamedConstructor::createWrapper(PassRefPtr<TestNamedConstructor> impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    ASSERT(impl);
    ASSERT(!DOMDataStore::containsWrapper<V8TestNamedConstructor>(impl.get(), isolate));
    if (ScriptWrappable::wrapperCanBeStoredInObject(impl.get())) {
        const WrapperTypeInfo* actualInfo = ScriptWrappable::getTypeInfoFromObject(impl.get());
        // Might be a XXXConstructor::wrapperTypeInfo instead of an XXX::wrapperTypeInfo. These will both have
        // the same object de-ref functions, though, so use that as the basis of the check.
        RELEASE_ASSERT_WITH_SECURITY_IMPLICATION(actualInfo->derefObjectFunction == wrapperTypeInfo.derefObjectFunction);
    }

    v8::Handle<v8::Object> wrapper = V8DOMWrapper::createWrapper(creationContext, &wrapperTypeInfo, toInternalPointer(impl.get()), isolate);
    if (UNLIKELY(wrapper.IsEmpty()))
        return wrapper;

    installPerContextEnabledProperties(wrapper, impl.get(), isolate);
    V8DOMWrapper::associateObjectWithWrapper<V8TestNamedConstructor>(impl, &wrapperTypeInfo, wrapper, isolate, WrapperConfiguration::Dependent);
    return wrapper;
}

void V8TestNamedConstructor::derefObject(void* object)
{
    fromInternalPointer(object)->deref();
}

template<>
v8::Handle<v8::Value> toV8NoInline(TestNamedConstructor* impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    return toV8(impl, creationContext, isolate);
}

} // namespace WebCore
