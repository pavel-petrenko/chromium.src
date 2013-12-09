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
#include "V8TestCustomAccessors.h"

#include "RuntimeEnabledFeatures.h"
#include "bindings/v8/ExceptionMessages.h"
#include "bindings/v8/ExceptionState.h"
#include "bindings/v8/V8DOMConfiguration.h"
#include "core/dom/ContextFeatures.h"
#include "core/dom/Document.h"
#include "platform/TraceEvent.h"

namespace WebCore {

static void initializeScriptWrappableForInterface(TestCustomAccessors* object)
{
    if (ScriptWrappable::wrapperCanBeStoredInObject(object))
        ScriptWrappable::setTypeInfoInObject(object, &V8TestCustomAccessors::wrapperTypeInfo);
    else
        ASSERT_NOT_REACHED();
}

} // namespace WebCore

// In ScriptWrappable::init, the use of a local function declaration has an issue on Windows:
// the local declaration does not pick up the surrounding namespace. Therefore, we provide this function
// in the global namespace.
// (More info on the MSVC bug here: http://connect.microsoft.com/VisualStudio/feedback/details/664619/the-namespace-of-local-function-declarations-in-c)
void webCoreInitializeScriptWrappableForInterface(WebCore::TestCustomAccessors* object)
{
    WebCore::initializeScriptWrappableForInterface(object);
}

namespace WebCore {
const WrapperTypeInfo V8TestCustomAccessors::wrapperTypeInfo = { gin::kEmbedderBlink, V8TestCustomAccessors::domTemplate, V8TestCustomAccessors::derefObject, 0, 0, 0, V8TestCustomAccessors::installPerContextEnabledMethods, 0, WrapperTypeObjectPrototype };

namespace TestCustomAccessorsV8Internal {

template <typename T> void V8_USE(T) { }

static void anotherFunctionMethod(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    if (UNLIKELY(info.Length() < 1)) {
        throwTypeError(ExceptionMessages::failedToExecute("anotherFunction", "TestCustomAccessors", ExceptionMessages::notEnoughArguments(1, info.Length())), info.GetIsolate());
        return;
    }
    TestCustomAccessors* imp = V8TestCustomAccessors::toNative(info.Holder());
    V8TRYCATCH_FOR_V8STRINGRESOURCE_VOID(V8StringResource<>, str, info[0]);
    imp->anotherFunction(str);
}

static void anotherFunctionMethodCallback(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMMethod");
    TestCustomAccessorsV8Internal::anotherFunctionMethod(info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void indexedPropertyGetterCallback(uint32_t index, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMIndexedProperty");
    V8TestCustomAccessors::indexedPropertyGetterCustom(index, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void indexedPropertySetterCallback(uint32_t index, v8::Local<v8::Value> jsValue, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMIndexedProperty");
    V8TestCustomAccessors::indexedPropertySetterCustom(index, jsValue, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void indexedPropertyDeleterCallback(uint32_t index, const v8::PropertyCallbackInfo<v8::Boolean>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMIndexedProperty");
    V8TestCustomAccessors::indexedPropertyDeleterCustom(index, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void namedPropertyGetterCallback(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMNamedProperty");
    V8TestCustomAccessors::namedPropertyGetterCustom(name, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void namedPropertySetterCallback(v8::Local<v8::String> name, v8::Local<v8::Value> jsValue, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMNamedProperty");
    V8TestCustomAccessors::namedPropertySetterCustom(name, jsValue, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void namedPropertyDeleterCallback(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Boolean>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMNamedProperty");
    V8TestCustomAccessors::namedPropertyDeleterCustom(name, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void namedPropertyEnumeratorCallback(const v8::PropertyCallbackInfo<v8::Array>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMNamedProperty");
    V8TestCustomAccessors::namedPropertyEnumeratorCustom(info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void namedPropertyQueryCallback(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Integer>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMNamedProperty");
    V8TestCustomAccessors::namedPropertyQueryCustom(name, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

} // namespace TestCustomAccessorsV8Internal

static const V8DOMConfiguration::MethodConfiguration V8TestCustomAccessorsMethods[] = {
    {"anotherFunction", TestCustomAccessorsV8Internal::anotherFunctionMethodCallback, 0, 1},
};

static v8::Handle<v8::FunctionTemplate> ConfigureV8TestCustomAccessorsTemplate(v8::Handle<v8::FunctionTemplate> functionTemplate, v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    functionTemplate->ReadOnlyPrototype();

    v8::Local<v8::Signature> defaultSignature;
    defaultSignature = V8DOMConfiguration::installDOMClassTemplate(functionTemplate, "TestCustomAccessors", v8::Local<v8::FunctionTemplate>(), V8TestCustomAccessors::internalFieldCount,
        0, 0,
        0, 0,
        V8TestCustomAccessorsMethods, WTF_ARRAY_LENGTH(V8TestCustomAccessorsMethods),
        isolate, currentWorldType);
    v8::Local<v8::ObjectTemplate> ALLOW_UNUSED instanceTemplate = functionTemplate->InstanceTemplate();
    v8::Local<v8::ObjectTemplate> ALLOW_UNUSED prototypeTemplate = functionTemplate->PrototypeTemplate();
    functionTemplate->InstanceTemplate()->SetIndexedPropertyHandler(TestCustomAccessorsV8Internal::indexedPropertyGetterCallback, TestCustomAccessorsV8Internal::indexedPropertySetterCallback, 0, TestCustomAccessorsV8Internal::indexedPropertyDeleterCallback, 0);
    functionTemplate->InstanceTemplate()->SetNamedPropertyHandler(TestCustomAccessorsV8Internal::namedPropertyGetterCallback, TestCustomAccessorsV8Internal::namedPropertySetterCallback, TestCustomAccessorsV8Internal::namedPropertyQueryCallback, TestCustomAccessorsV8Internal::namedPropertyDeleterCallback, TestCustomAccessorsV8Internal::namedPropertyEnumeratorCallback);

    // Custom toString template
    functionTemplate->Set(v8::String::NewFromUtf8(isolate, "toString", v8::String::kInternalizedString), V8PerIsolateData::current()->toStringTemplate());
    return functionTemplate;
}

v8::Handle<v8::FunctionTemplate> V8TestCustomAccessors::domTemplate(v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    V8PerIsolateData* data = V8PerIsolateData::from(isolate);
    V8PerIsolateData::TemplateMap::iterator result = data->templateMap(currentWorldType).find(&wrapperTypeInfo);
    if (result != data->templateMap(currentWorldType).end())
        return result->value.newLocal(isolate);

    TRACE_EVENT_SCOPED_SAMPLING_STATE("Blink", "BuildDOMTemplate");
    v8::EscapableHandleScope handleScope(isolate);
    v8::Local<v8::FunctionTemplate> templ =
        ConfigureV8TestCustomAccessorsTemplate(data->rawDOMTemplate(&wrapperTypeInfo, currentWorldType), isolate, currentWorldType);
    data->templateMap(currentWorldType).add(&wrapperTypeInfo, UnsafePersistent<v8::FunctionTemplate>(isolate, templ));
    return handleScope.Escape(templ);
}

bool V8TestCustomAccessors::hasInstance(v8::Handle<v8::Value> jsValue, v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    return V8PerIsolateData::from(isolate)->hasInstance(&wrapperTypeInfo, jsValue, currentWorldType);
}

bool V8TestCustomAccessors::hasInstanceInAnyWorld(v8::Handle<v8::Value> jsValue, v8::Isolate* isolate)
{
    return V8PerIsolateData::from(isolate)->hasInstance(&wrapperTypeInfo, jsValue, MainWorld)
        || V8PerIsolateData::from(isolate)->hasInstance(&wrapperTypeInfo, jsValue, IsolatedWorld)
        || V8PerIsolateData::from(isolate)->hasInstance(&wrapperTypeInfo, jsValue, WorkerWorld);
}

v8::Handle<v8::Object> V8TestCustomAccessors::createWrapper(PassRefPtr<TestCustomAccessors> impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    ASSERT(impl);
    ASSERT(!DOMDataStore::containsWrapper<V8TestCustomAccessors>(impl.get(), isolate));
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
    V8DOMWrapper::associateObjectWithWrapper<V8TestCustomAccessors>(impl, &wrapperTypeInfo, wrapper, isolate, WrapperConfiguration::Independent);
    return wrapper;
}

void V8TestCustomAccessors::derefObject(void* object)
{
    fromInternalPointer(object)->deref();
}

template<>
v8::Handle<v8::Value> toV8NoInline(TestCustomAccessors* impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    return toV8(impl, creationContext, isolate);
}

} // namespace WebCore
