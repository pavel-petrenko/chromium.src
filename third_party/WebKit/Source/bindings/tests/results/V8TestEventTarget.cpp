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
#include "V8TestEventTarget.h"

#include "RuntimeEnabledFeatures.h"
#include "V8EventTarget.h"
#include "V8Node.h"
#include "bindings/v8/ExceptionMessages.h"
#include "bindings/v8/ExceptionState.h"
#include "bindings/v8/ScriptController.h"
#include "bindings/v8/V8Binding.h"
#include "bindings/v8/V8DOMConfiguration.h"
#include "bindings/v8/V8DOMWrapper.h"
#include "core/dom/ContextFeatures.h"
#include "core/dom/Document.h"
#include "platform/TraceEvent.h"
#include "wtf/GetPtr.h"
#include "wtf/RefPtr.h"
#include "wtf/UnusedParam.h"

namespace WebCore {

static void initializeScriptWrappableForInterface(TestEventTarget* object)
{
    if (ScriptWrappable::wrapperCanBeStoredInObject(object))
        ScriptWrappable::setTypeInfoInObject(object, &V8TestEventTarget::wrapperTypeInfo);
    else
        ASSERT_NOT_REACHED();
}

} // namespace WebCore

// In ScriptWrappable::init, the use of a local function declaration has an issue on Windows:
// the local declaration does not pick up the surrounding namespace. Therefore, we provide this function
// in the global namespace.
// (More info on the MSVC bug here: http://connect.microsoft.com/VisualStudio/feedback/details/664619/the-namespace-of-local-function-declarations-in-c)
void webCoreInitializeScriptWrappableForInterface(WebCore::TestEventTarget* object)
{
    WebCore::initializeScriptWrappableForInterface(object);
}

namespace WebCore {
const WrapperTypeInfo V8TestEventTarget::wrapperTypeInfo = { V8TestEventTarget::GetTemplate, V8TestEventTarget::derefObject, 0, V8TestEventTarget::toEventTarget, 0, V8TestEventTarget::installPerContextEnabledPrototypeProperties, &V8EventTarget::wrapperTypeInfo, WrapperTypeObjectPrototype };

namespace TestEventTargetV8Internal {

template <typename T> void V8_USE(T) { }

static void itemMethod(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    if (UNLIKELY(info.Length() < 1)) {
        throwTypeError(ExceptionMessages::failedToExecute("item", "TestEventTarget", ExceptionMessages::notEnoughArguments(1, info.Length())), info.GetIsolate());
        return;
    }
    TestEventTarget* imp = V8TestEventTarget::toNative(info.Holder());
    V8TRYCATCH_VOID(unsigned, index, toUInt32(info[0]));
    v8SetReturnValue(info, imp->item(index));
}

static void itemMethodCallback(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMMethod");
    TestEventTargetV8Internal::itemMethod(info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void namedItemMethod(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    if (UNLIKELY(info.Length() < 1)) {
        throwTypeError(ExceptionMessages::failedToExecute("namedItem", "TestEventTarget", ExceptionMessages::notEnoughArguments(1, info.Length())), info.GetIsolate());
        return;
    }
    TestEventTarget* imp = V8TestEventTarget::toNative(info.Holder());
    V8TRYCATCH_FOR_V8STRINGRESOURCE_VOID(V8StringResource<>, name, info[0]);
    v8SetReturnValue(info, imp->namedItem(name));
}

static void namedItemMethodCallback(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMMethod");
    TestEventTargetV8Internal::namedItemMethod(info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void indexedPropertyGetter(uint32_t index, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    ASSERT(V8DOMWrapper::maybeDOMWrapper(info.Holder()));
    TestEventTarget* collection = V8TestEventTarget::toNative(info.Holder());
    RefPtr<Node> element = collection->item(index);
    if (!element)
        return;
    v8SetReturnValueFast(info, element.release(), collection);
}

static void indexedPropertyGetterCallback(uint32_t index, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMIndexedProperty");
    TestEventTargetV8Internal::indexedPropertyGetter(index, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void indexedPropertySetter(uint32_t index, v8::Local<v8::Value> jsValue, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TestEventTarget* collection = V8TestEventTarget::toNative(info.Holder());
    V8TRYCATCH_VOID(Node*, propertyValue, V8Node::hasInstance(jsValue, info.GetIsolate(), worldType(info.GetIsolate())) ? V8Node::toNative(v8::Handle<v8::Object>::Cast(jsValue)) : 0);
    bool result = collection->anonymousIndexedSetter(index, propertyValue);
    if (!result)
        return;
    v8SetReturnValue(info, jsValue);
}

static void indexedPropertySetterCallback(uint32_t index, v8::Local<v8::Value> jsValue, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMIndexedProperty");
    TestEventTargetV8Internal::indexedPropertySetter(index, jsValue, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void indexedPropertyDeleter(unsigned index, const v8::PropertyCallbackInfo<v8::Boolean>& info)
{
    TestEventTarget* collection = V8TestEventTarget::toNative(info.Holder());
    ExceptionState es(info.GetIsolate());
    bool result = collection->anonymousIndexedDeleter(index, es);
    if (es.throwIfNeeded())
        return;
    return v8SetReturnValueBool(info, result);
}

static void indexedPropertyDeleterCallback(uint32_t index, const v8::PropertyCallbackInfo<v8::Boolean>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMIndexedProperty");
    TestEventTargetV8Internal::indexedPropertyDeleter(index, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void namedPropertyGetter(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    if (!info.Holder()->GetRealNamedPropertyInPrototypeChain(name).IsEmpty())
        return;
    if (info.Holder()->HasRealNamedCallbackProperty(name))
        return;
    if (info.Holder()->HasRealNamedProperty(name))
        return;

    ASSERT(V8DOMWrapper::maybeDOMWrapper(info.Holder()));
    TestEventTarget* collection = V8TestEventTarget::toNative(info.Holder());
    AtomicString propertyName = toWebCoreAtomicString(name);
    RefPtr<Node> element = collection->namedItem(propertyName);
    if (!element)
        return;
    v8SetReturnValueFast(info, element.release(), collection);
}

static void namedPropertyGetterCallback(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMNamedProperty");
    TestEventTargetV8Internal::namedPropertyGetter(name, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void namedPropertySetter(v8::Local<v8::String> name, v8::Local<v8::Value> jsValue, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    if (!info.Holder()->GetRealNamedPropertyInPrototypeChain(name).IsEmpty())
        return;
    if (info.Holder()->HasRealNamedCallbackProperty(name))
        return;
    if (info.Holder()->HasRealNamedProperty(name))
        return;
    TestEventTarget* collection = V8TestEventTarget::toNative(info.Holder());
    V8TRYCATCH_FOR_V8STRINGRESOURCE_VOID(V8StringResource<>, propertyName, name);
    V8TRYCATCH_FOR_V8STRINGRESOURCE_VOID(V8StringResource<>, propertyValue, jsValue);
    bool result;
    if (jsValue->IsUndefined())
        result = collection->anonymousNamedSetterUndefined(propertyName);
    else
        result = collection->anonymousNamedSetter(propertyName, propertyValue);
    if (!result)
        return;
    v8SetReturnValue(info, jsValue);
}

static void namedPropertySetterCallback(v8::Local<v8::String> name, v8::Local<v8::Value> jsValue, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMNamedProperty");
    TestEventTargetV8Internal::namedPropertySetter(name, jsValue, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void namedPropertyDeleter(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Boolean>& info)
{
    TestEventTarget* collection = V8TestEventTarget::toNative(info.Holder());
    AtomicString propertyName = toWebCoreAtomicString(name);
    bool result = collection->anonymousNamedDeleter(propertyName);
    return v8SetReturnValueBool(info, result);
}

static void namedPropertyDeleterCallback(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Boolean>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMNamedProperty");
    TestEventTargetV8Internal::namedPropertyDeleter(name, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void namedPropertyEnumerator(const v8::PropertyCallbackInfo<v8::Array>& info)
{
    ExceptionState es(info.GetIsolate());
    TestEventTarget* collection = V8TestEventTarget::toNative(info.Holder());
    Vector<String> names;
    collection->namedPropertyEnumerator(names, es);
    if (es.throwIfNeeded())
        return;
    v8::Handle<v8::Array> v8names = v8::Array::New(names.size());
    for (size_t i = 0; i < names.size(); ++i)
        v8names->Set(v8::Integer::New(i, info.GetIsolate()), v8String(names[i], info.GetIsolate()));
    v8SetReturnValue(info, v8names);
}

static void namedPropertyQuery(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Integer>& info)
{
    TestEventTarget* collection = V8TestEventTarget::toNative(info.Holder());
    AtomicString propertyName = toWebCoreAtomicString(name);
    ExceptionState es(info.GetIsolate());
    bool result = collection->namedPropertyQuery(propertyName, es);
    if (es.throwIfNeeded())
        return;
    if (!result)
        return;
    v8SetReturnValueInt(info, v8::None);
}

static void namedPropertyEnumeratorCallback(const v8::PropertyCallbackInfo<v8::Array>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMNamedProperty");
    TestEventTargetV8Internal::namedPropertyEnumerator(info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void namedPropertyQueryCallback(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Integer>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMNamedProperty");
    TestEventTargetV8Internal::namedPropertyQuery(name, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

} // namespace TestEventTargetV8Internal

static const V8DOMConfiguration::MethodConfiguration V8TestEventTargetMethods[] = {
    {"item", TestEventTargetV8Internal::itemMethodCallback, 0, 1},
    {"namedItem", TestEventTargetV8Internal::namedItemMethodCallback, 0, 1},
};

static v8::Handle<v8::FunctionTemplate> ConfigureV8TestEventTargetTemplate(v8::Handle<v8::FunctionTemplate> desc, v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    desc->ReadOnlyPrototype();

    v8::Local<v8::Signature> defaultSignature;
    defaultSignature = V8DOMConfiguration::installDOMClassTemplate(desc, "TestEventTarget", V8EventTarget::GetTemplate(isolate, currentWorldType), V8TestEventTarget::internalFieldCount,
        0, 0,
        V8TestEventTargetMethods, WTF_ARRAY_LENGTH(V8TestEventTargetMethods),
        isolate, currentWorldType);
    UNUSED_PARAM(defaultSignature);
    v8::Local<v8::ObjectTemplate> instance = desc->InstanceTemplate();
    v8::Local<v8::ObjectTemplate> proto = desc->PrototypeTemplate();
    UNUSED_PARAM(instance);
    UNUSED_PARAM(proto);
    desc->InstanceTemplate()->SetIndexedPropertyHandler(TestEventTargetV8Internal::indexedPropertyGetterCallback, TestEventTargetV8Internal::indexedPropertySetterCallback, 0, TestEventTargetV8Internal::indexedPropertyDeleterCallback, indexedPropertyEnumerator<TestEventTarget>);
    desc->InstanceTemplate()->SetNamedPropertyHandler(TestEventTargetV8Internal::namedPropertyGetterCallback, TestEventTargetV8Internal::namedPropertySetterCallback, TestEventTargetV8Internal::namedPropertyQueryCallback, TestEventTargetV8Internal::namedPropertyDeleterCallback, TestEventTargetV8Internal::namedPropertyEnumeratorCallback);
    desc->InstanceTemplate()->MarkAsUndetectable();

    // Custom toString template
    desc->Set(v8::String::NewSymbol("toString"), V8PerIsolateData::current()->toStringTemplate());
    return desc;
}

v8::Handle<v8::FunctionTemplate> V8TestEventTarget::GetTemplate(v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    V8PerIsolateData* data = V8PerIsolateData::from(isolate);
    V8PerIsolateData::TemplateMap::iterator result = data->templateMap(currentWorldType).find(&wrapperTypeInfo);
    if (result != data->templateMap(currentWorldType).end())
        return result->value.newLocal(isolate);

    TRACE_EVENT_SCOPED_SAMPLING_STATE("Blink", "BuildDOMTemplate");
    v8::HandleScope handleScope(isolate);
    v8::Handle<v8::FunctionTemplate> templ =
        ConfigureV8TestEventTargetTemplate(data->rawTemplate(&wrapperTypeInfo, currentWorldType), isolate, currentWorldType);
    data->templateMap(currentWorldType).add(&wrapperTypeInfo, UnsafePersistent<v8::FunctionTemplate>(isolate, templ));
    return handleScope.Close(templ);
}

bool V8TestEventTarget::hasInstance(v8::Handle<v8::Value> jsValue, v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    return V8PerIsolateData::from(isolate)->hasInstance(&wrapperTypeInfo, jsValue, currentWorldType);
}

bool V8TestEventTarget::hasInstanceInAnyWorld(v8::Handle<v8::Value> jsValue, v8::Isolate* isolate)
{
    return V8PerIsolateData::from(isolate)->hasInstance(&wrapperTypeInfo, jsValue, MainWorld)
        || V8PerIsolateData::from(isolate)->hasInstance(&wrapperTypeInfo, jsValue, IsolatedWorld)
        || V8PerIsolateData::from(isolate)->hasInstance(&wrapperTypeInfo, jsValue, WorkerWorld);
}

EventTarget* V8TestEventTarget::toEventTarget(v8::Handle<v8::Object> object)
{
    return toNative(object);
}

v8::Handle<v8::Object> V8TestEventTarget::createWrapper(PassRefPtr<TestEventTarget> impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    ASSERT(impl);
    ASSERT(!DOMDataStore::containsWrapper<V8TestEventTarget>(impl.get(), isolate));
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
    V8DOMWrapper::associateObjectWithWrapper<V8TestEventTarget>(impl, &wrapperTypeInfo, wrapper, isolate, WrapperConfiguration::Independent);
    return wrapper;
}

void V8TestEventTarget::derefObject(void* object)
{
    fromInternalPointer(object)->deref();
}

} // namespace WebCore
