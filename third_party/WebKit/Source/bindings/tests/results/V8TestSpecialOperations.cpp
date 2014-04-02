// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file has been auto-generated by code_generator_v8.py. DO NOT MODIFY!

#include "config.h"
#include "V8TestSpecialOperations.h"

#include "RuntimeEnabledFeatures.h"
#include "V8Node.h"
#include "V8NodeList.h"
#include "bindings/v8/ExceptionState.h"
#include "bindings/v8/V8DOMConfiguration.h"
#include "bindings/v8/V8HiddenValue.h"
#include "bindings/v8/V8ObjectConstructor.h"
#include "core/dom/ContextFeatures.h"
#include "core/dom/Document.h"
#include "platform/TraceEvent.h"
#include "wtf/GetPtr.h"
#include "wtf/RefPtr.h"

namespace WebCore {

static void initializeScriptWrappableForInterface(TestSpecialOperations* object)
{
    if (ScriptWrappable::wrapperCanBeStoredInObject(object))
        ScriptWrappable::setTypeInfoInObject(object, &V8TestSpecialOperations::wrapperTypeInfo);
    else
        ASSERT_NOT_REACHED();
}

} // namespace WebCore

void webCoreInitializeScriptWrappableForInterface(WebCore::TestSpecialOperations* object)
{
    WebCore::initializeScriptWrappableForInterface(object);
}

namespace WebCore {
const WrapperTypeInfo V8TestSpecialOperations::wrapperTypeInfo = { gin::kEmbedderBlink, V8TestSpecialOperations::domTemplate, V8TestSpecialOperations::derefObject, 0, 0, 0, V8TestSpecialOperations::installPerContextEnabledMethods, 0, WrapperTypeObjectPrototype, false };

namespace TestSpecialOperationsV8Internal {

template <typename T> void V8_USE(T) { }

static void namedItemMethod(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    if (UNLIKELY(info.Length() < 1)) {
        throwArityTypeErrorForMethod("namedItem", "TestSpecialOperations", 1, info.Length(), info.GetIsolate());
        return;
    }
    TestSpecialOperations* impl = V8TestSpecialOperations::toNative(info.Holder());
    V8TRYCATCH_FOR_V8STRINGRESOURCE_VOID(V8StringResource<>, name, info[0]);
    bool result0Enabled = false;
    RefPtr<Node> result0;
    bool result1Enabled = false;
    RefPtr<NodeList> result1;
    impl->getItem(name, result0Enabled, result0, result1Enabled, result1);
    if (result0Enabled) {
        v8SetReturnValue(info, result0.release());
        return;
    }
    if (result1Enabled) {
        v8SetReturnValue(info, result1.release());
        return;
    }
    v8SetReturnValueNull(info);
}

static void namedItemMethodCallback(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMMethod");
    TestSpecialOperationsV8Internal::namedItemMethod(info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "V8Execution");
}

static void namedPropertyGetter(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TestSpecialOperations* impl = V8TestSpecialOperations::toNative(info.Holder());
    AtomicString propertyName = toCoreAtomicString(name);
    bool result0Enabled = false;
    RefPtr<Node> result0;
    bool result1Enabled = false;
    RefPtr<NodeList> result1;
    impl->getItem(propertyName, result0Enabled, result0, result1Enabled, result1);
    if (!result0Enabled && !result1Enabled)
        return;
    if (result0Enabled) {
        v8SetReturnValueFast(info, WTF::getPtr(result0.release()), impl);
        return;
    }
    if (result1Enabled) {
        v8SetReturnValueFast(info, WTF::getPtr(result1.release()), impl);
        return;
    }
    v8SetReturnValueNull(info);
}

static void namedPropertyGetterCallback(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMNamedProperty");
    TestSpecialOperationsV8Internal::namedPropertyGetter(name, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "V8Execution");
}

static void namedPropertySetter(v8::Local<v8::String> name, v8::Local<v8::Value> v8Value, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TestSpecialOperations* impl = V8TestSpecialOperations::toNative(info.Holder());
    V8TRYCATCH_FOR_V8STRINGRESOURCE_VOID(V8StringResource<>, propertyName, name);
    V8TRYCATCH_VOID(Node*, propertyValue, V8Node::toNativeWithTypeCheck(info.GetIsolate(), v8Value));
    bool result = impl->anonymousNamedSetter(propertyName, propertyValue);
    if (!result)
        return;
    v8SetReturnValue(info, v8Value);
}

static void namedPropertySetterCallback(v8::Local<v8::String> name, v8::Local<v8::Value> v8Value, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMNamedProperty");
    TestSpecialOperationsV8Internal::namedPropertySetter(name, v8Value, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "V8Execution");
}

static void namedPropertyQuery(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Integer>& info)
{
    TestSpecialOperations* impl = V8TestSpecialOperations::toNative(info.Holder());
    AtomicString propertyName = toCoreAtomicString(name);
    v8::String::Utf8Value namedProperty(name);
    ExceptionState exceptionState(ExceptionState::GetterContext, *namedProperty, "TestSpecialOperations", info.Holder(), info.GetIsolate());
    bool result = impl->namedPropertyQuery(propertyName, exceptionState);
    if (exceptionState.throwIfNeeded())
        return;
    if (!result)
        return;
    v8SetReturnValueInt(info, v8::None);
}

static void namedPropertyQueryCallback(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Integer>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMNamedProperty");
    TestSpecialOperationsV8Internal::namedPropertyQuery(name, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "V8Execution");
}

static void namedPropertyEnumerator(const v8::PropertyCallbackInfo<v8::Array>& info)
{
    TestSpecialOperations* impl = V8TestSpecialOperations::toNative(info.Holder());
    v8::Isolate* isolate = info.GetIsolate();
    Vector<String> names;
    ExceptionState exceptionState(ExceptionState::EnumerationContext, "TestSpecialOperations", info.Holder(), isolate);
    impl->namedPropertyEnumerator(names, exceptionState);
    if (exceptionState.throwIfNeeded())
        return;
    v8::Handle<v8::Array> v8names = v8::Array::New(isolate, names.size());
    for (size_t i = 0; i < names.size(); ++i)
        v8names->Set(v8::Integer::New(isolate, i), v8String(isolate, names[i]));
    v8SetReturnValue(info, v8names);
}

static void namedPropertyEnumeratorCallback(const v8::PropertyCallbackInfo<v8::Array>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMNamedProperty");
    TestSpecialOperationsV8Internal::namedPropertyEnumerator(info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "V8Execution");
}

} // namespace TestSpecialOperationsV8Internal

static const V8DOMConfiguration::MethodConfiguration V8TestSpecialOperationsMethods[] = {
    {"namedItem", TestSpecialOperationsV8Internal::namedItemMethodCallback, 0, 1},
};

static void configureV8TestSpecialOperationsTemplate(v8::Handle<v8::FunctionTemplate> functionTemplate, v8::Isolate* isolate)
{
    functionTemplate->ReadOnlyPrototype();

    v8::Local<v8::Signature> defaultSignature;
    defaultSignature = V8DOMConfiguration::installDOMClassTemplate(functionTemplate, "TestSpecialOperations", v8::Local<v8::FunctionTemplate>(), V8TestSpecialOperations::internalFieldCount,
        0, 0,
        0, 0,
        V8TestSpecialOperationsMethods, WTF_ARRAY_LENGTH(V8TestSpecialOperationsMethods),
        isolate);
    v8::Local<v8::ObjectTemplate> ALLOW_UNUSED instanceTemplate = functionTemplate->InstanceTemplate();
    v8::Local<v8::ObjectTemplate> ALLOW_UNUSED prototypeTemplate = functionTemplate->PrototypeTemplate();
    functionTemplate->InstanceTemplate()->SetNamedPropertyHandler(TestSpecialOperationsV8Internal::namedPropertyGetterCallback, TestSpecialOperationsV8Internal::namedPropertySetterCallback, TestSpecialOperationsV8Internal::namedPropertyQueryCallback, 0, TestSpecialOperationsV8Internal::namedPropertyEnumeratorCallback);

    // Custom toString template
    functionTemplate->Set(v8AtomicString(isolate, "toString"), V8PerIsolateData::current()->toStringTemplate());
}

v8::Handle<v8::FunctionTemplate> V8TestSpecialOperations::domTemplate(v8::Isolate* isolate)
{
    V8PerIsolateData* data = V8PerIsolateData::from(isolate);
    v8::Local<v8::FunctionTemplate> result = data->existingDOMTemplate(const_cast<WrapperTypeInfo*>(&wrapperTypeInfo));
    if (!result.IsEmpty())
        return result;

    TRACE_EVENT_SCOPED_SAMPLING_STATE("Blink", "BuildDOMTemplate");
    result = v8::FunctionTemplate::New(isolate, V8ObjectConstructor::isValidConstructorMode);
    configureV8TestSpecialOperationsTemplate(result, isolate);
    data->setDOMTemplate(const_cast<WrapperTypeInfo*>(&wrapperTypeInfo), result);
    return result;
}

bool V8TestSpecialOperations::hasInstance(v8::Handle<v8::Value> v8Value, v8::Isolate* isolate)
{
    return V8PerIsolateData::from(isolate)->hasInstance(&wrapperTypeInfo, v8Value);
}

v8::Handle<v8::Object> V8TestSpecialOperations::findInstanceInPrototypeChain(v8::Handle<v8::Value> v8Value, v8::Isolate* isolate)
{
    return V8PerIsolateData::from(isolate)->findInstanceInPrototypeChain(&wrapperTypeInfo, v8Value);
}

TestSpecialOperations* V8TestSpecialOperations::toNativeWithTypeCheck(v8::Isolate* isolate, v8::Handle<v8::Value> value)
{
    return hasInstance(value, isolate) ? fromInternalPointer(v8::Handle<v8::Object>::Cast(value)->GetAlignedPointerFromInternalField(v8DOMWrapperObjectIndex)) : 0;
}

v8::Handle<v8::Object> V8TestSpecialOperations::createWrapper(PassRefPtr<TestSpecialOperations> impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    ASSERT(impl);
    ASSERT(!DOMDataStore::containsWrapper<V8TestSpecialOperations>(impl.get(), isolate));
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
    V8DOMWrapper::associateObjectWithWrapper<V8TestSpecialOperations>(impl, &wrapperTypeInfo, wrapper, isolate, WrapperConfiguration::Independent);
    return wrapper;
}

void V8TestSpecialOperations::derefObject(void* object)
{
    fromInternalPointer(object)->deref();
}

template<>
v8::Handle<v8::Value> toV8NoInline(TestSpecialOperations* impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    return toV8(impl, creationContext, isolate);
}

} // namespace WebCore
