// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file has been auto-generated by code_generator_v8.py. DO NOT MODIFY!

#include "config.h"
#include "V8TestInterface2.h"

#include "RuntimeEnabledFeatures.h"
#include "V8Interface1.h"
#include "V8Interface2.h"
#include "V8TestInterfaceEmpty.h"
#include "bindings/v8/ExceptionState.h"
#include "bindings/v8/V8DOMConfiguration.h"
#include "bindings/v8/V8GCController.h"
#include "bindings/v8/V8HiddenValue.h"
#include "bindings/v8/V8ObjectConstructor.h"
#include "core/dom/ContextFeatures.h"
#include "core/dom/Document.h"
#include "core/dom/Element.h"
#include "core/frame/DOMWindow.h"
#include "platform/TraceEvent.h"
#include "wtf/GetPtr.h"
#include "wtf/RefPtr.h"

namespace WebCore {

static void initializeScriptWrappableForInterface(TestInterface2* object)
{
    if (ScriptWrappable::wrapperCanBeStoredInObject(object))
        ScriptWrappable::setTypeInfoInObject(object, &V8TestInterface2::wrapperTypeInfo);
    else
        ASSERT_NOT_REACHED();
}

} // namespace WebCore

void webCoreInitializeScriptWrappableForInterface(WebCore::TestInterface2* object)
{
    WebCore::initializeScriptWrappableForInterface(object);
}

namespace WebCore {
const WrapperTypeInfo V8TestInterface2::wrapperTypeInfo = { gin::kEmbedderBlink, V8TestInterface2::domTemplate, V8TestInterface2::derefObject, 0, 0, V8TestInterface2::visitDOMWrapper, V8TestInterface2::installPerContextEnabledMethods, 0, WrapperTypeObjectPrototype, false };

namespace TestInterface2V8Internal {

template <typename T> void V8_USE(T) { }

static void itemMethod(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    ExceptionState exceptionState(ExceptionState::ExecutionContext, "item", "TestInterface2", info.Holder(), info.GetIsolate());
    if (UNLIKELY(info.Length() < 1)) {
        throwArityTypeError(exceptionState, 1, info.Length());
        return;
    }
    TestInterface2* impl = V8TestInterface2::toNative(info.Holder());
    V8TRYCATCH_EXCEPTION_VOID(unsigned, index, toUInt32(info[0], exceptionState), exceptionState);
    RefPtr<TestInterfaceEmpty> result = impl->item(index, exceptionState);
    if (exceptionState.throwIfNeeded())
        return;
    v8SetReturnValue(info, result.release());
}

static void itemMethodCallback(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMMethod");
    TestInterface2V8Internal::itemMethod(info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "V8Execution");
}

static void setItemMethod(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    ExceptionState exceptionState(ExceptionState::ExecutionContext, "setItem", "TestInterface2", info.Holder(), info.GetIsolate());
    if (UNLIKELY(info.Length() < 2)) {
        throwArityTypeError(exceptionState, 2, info.Length());
        return;
    }
    TestInterface2* impl = V8TestInterface2::toNative(info.Holder());
    V8TRYCATCH_EXCEPTION_VOID(unsigned, index, toUInt32(info[0], exceptionState), exceptionState);
    V8TRYCATCH_FOR_V8STRINGRESOURCE_VOID(V8StringResource<>, value, info[1]);
    String result = impl->setItem(index, value, exceptionState);
    if (exceptionState.throwIfNeeded())
        return;
    v8SetReturnValueString(info, result, info.GetIsolate());
}

static void setItemMethodCallback(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMMethod");
    TestInterface2V8Internal::setItemMethod(info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "V8Execution");
}

static void deleteItemMethod(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    ExceptionState exceptionState(ExceptionState::ExecutionContext, "deleteItem", "TestInterface2", info.Holder(), info.GetIsolate());
    if (UNLIKELY(info.Length() < 1)) {
        throwArityTypeError(exceptionState, 1, info.Length());
        return;
    }
    TestInterface2* impl = V8TestInterface2::toNative(info.Holder());
    V8TRYCATCH_EXCEPTION_VOID(unsigned, index, toUInt32(info[0], exceptionState), exceptionState);
    bool result = impl->deleteItem(index, exceptionState);
    if (exceptionState.throwIfNeeded())
        return;
    v8SetReturnValueBool(info, result);
}

static void deleteItemMethodCallback(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMMethod");
    TestInterface2V8Internal::deleteItemMethod(info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "V8Execution");
}

static void namedItemMethod(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    ExceptionState exceptionState(ExceptionState::ExecutionContext, "namedItem", "TestInterface2", info.Holder(), info.GetIsolate());
    if (UNLIKELY(info.Length() < 1)) {
        throwArityTypeError(exceptionState, 1, info.Length());
        return;
    }
    TestInterface2* impl = V8TestInterface2::toNative(info.Holder());
    V8TRYCATCH_FOR_V8STRINGRESOURCE_VOID(V8StringResource<>, name, info[0]);
    RefPtr<TestInterfaceEmpty> result = impl->namedItem(name, exceptionState);
    if (exceptionState.throwIfNeeded())
        return;
    v8SetReturnValue(info, result.release());
}

static void namedItemMethodCallback(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMMethod");
    TestInterface2V8Internal::namedItemMethod(info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "V8Execution");
}

static void setNamedItemMethod(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    ExceptionState exceptionState(ExceptionState::ExecutionContext, "setNamedItem", "TestInterface2", info.Holder(), info.GetIsolate());
    if (UNLIKELY(info.Length() < 2)) {
        throwArityTypeError(exceptionState, 2, info.Length());
        return;
    }
    TestInterface2* impl = V8TestInterface2::toNative(info.Holder());
    V8TRYCATCH_FOR_V8STRINGRESOURCE_VOID(V8StringResource<>, name, info[0]);
    V8TRYCATCH_FOR_V8STRINGRESOURCE_VOID(V8StringResource<>, value, info[1]);
    String result = impl->setNamedItem(name, value, exceptionState);
    if (exceptionState.throwIfNeeded())
        return;
    v8SetReturnValueString(info, result, info.GetIsolate());
}

static void setNamedItemMethodCallback(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMMethod");
    TestInterface2V8Internal::setNamedItemMethod(info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "V8Execution");
}

static void deleteNamedItemMethod(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    ExceptionState exceptionState(ExceptionState::ExecutionContext, "deleteNamedItem", "TestInterface2", info.Holder(), info.GetIsolate());
    if (UNLIKELY(info.Length() < 1)) {
        throwArityTypeError(exceptionState, 1, info.Length());
        return;
    }
    TestInterface2* impl = V8TestInterface2::toNative(info.Holder());
    V8TRYCATCH_FOR_V8STRINGRESOURCE_VOID(V8StringResource<>, name, info[0]);
    bool result = impl->deleteNamedItem(name, exceptionState);
    if (exceptionState.throwIfNeeded())
        return;
    v8SetReturnValueBool(info, result);
}

static void deleteNamedItemMethodCallback(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMMethod");
    TestInterface2V8Internal::deleteNamedItemMethod(info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "V8Execution");
}

static void constructor(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    v8::Isolate* isolate = info.GetIsolate();
    RefPtr<TestInterface2> impl = TestInterface2::create();

    v8::Handle<v8::Object> wrapper = wrap(impl.get(), info.Holder(), isolate);
    v8SetReturnValue(info, wrapper);
}

static void indexedPropertyGetter(uint32_t index, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TestInterface2* impl = V8TestInterface2::toNative(info.Holder());
    ExceptionState exceptionState(ExceptionState::IndexedGetterContext, "TestInterface2", info.Holder(), info.GetIsolate());
    RefPtr<TestInterfaceEmpty> result = impl->item(index, exceptionState);
    if (exceptionState.throwIfNeeded())
        return;
    if (!result)
        return;
    v8SetReturnValueFast(info, WTF::getPtr(result.release()), impl);
}

static void indexedPropertyGetterCallback(uint32_t index, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMIndexedProperty");
    TestInterface2V8Internal::indexedPropertyGetter(index, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "V8Execution");
}

static void indexedPropertySetter(uint32_t index, v8::Local<v8::Value> v8Value, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TestInterface2* impl = V8TestInterface2::toNative(info.Holder());
    V8TRYCATCH_FOR_V8STRINGRESOURCE_VOID(V8StringResource<>, propertyValue, v8Value);
    ExceptionState exceptionState(ExceptionState::IndexedSetterContext, "TestInterface2", info.Holder(), info.GetIsolate());
    bool result = impl->setItem(index, propertyValue, exceptionState);
    if (exceptionState.throwIfNeeded())
        return;
    if (!result)
        return;
    v8SetReturnValue(info, v8Value);
}

static void indexedPropertySetterCallback(uint32_t index, v8::Local<v8::Value> v8Value, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMIndexedProperty");
    TestInterface2V8Internal::indexedPropertySetter(index, v8Value, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "V8Execution");
}

static void indexedPropertyDeleter(uint32_t index, const v8::PropertyCallbackInfo<v8::Boolean>& info)
{
    TestInterface2* impl = V8TestInterface2::toNative(info.Holder());
    ExceptionState exceptionState(ExceptionState::IndexedDeletionContext, "TestInterface2", info.Holder(), info.GetIsolate());
    DeleteResult result = impl->deleteItem(index, exceptionState);
    if (exceptionState.throwIfNeeded())
        return;
    if (result != DeleteUnknownProperty)
        return v8SetReturnValueBool(info, result == DeleteSuccess);
}

static void indexedPropertyDeleterCallback(uint32_t index, const v8::PropertyCallbackInfo<v8::Boolean>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMIndexedProperty");
    TestInterface2V8Internal::indexedPropertyDeleter(index, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "V8Execution");
}

static void namedPropertyGetter(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    if (info.Holder()->HasRealNamedProperty(name))
        return;
    if (!info.Holder()->GetRealNamedPropertyInPrototypeChain(name).IsEmpty())
        return;

    TestInterface2* impl = V8TestInterface2::toNative(info.Holder());
    AtomicString propertyName = toCoreAtomicString(name);
    v8::String::Utf8Value namedProperty(name);
    ExceptionState exceptionState(ExceptionState::GetterContext, *namedProperty, "TestInterface2", info.Holder(), info.GetIsolate());
    RefPtr<TestInterfaceEmpty> result = impl->namedItem(propertyName, exceptionState);
    if (exceptionState.throwIfNeeded())
        return;
    if (!result)
        return;
    v8SetReturnValueFast(info, WTF::getPtr(result.release()), impl);
}

static void namedPropertyGetterCallback(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMNamedProperty");
    TestInterface2V8Internal::namedPropertyGetter(name, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "V8Execution");
}

static void namedPropertySetter(v8::Local<v8::String> name, v8::Local<v8::Value> v8Value, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    if (info.Holder()->HasRealNamedProperty(name))
        return;
    if (!info.Holder()->GetRealNamedPropertyInPrototypeChain(name).IsEmpty())
        return;

    TestInterface2* impl = V8TestInterface2::toNative(info.Holder());
    V8TRYCATCH_FOR_V8STRINGRESOURCE_VOID(V8StringResource<>, propertyName, name);
    V8TRYCATCH_FOR_V8STRINGRESOURCE_VOID(V8StringResource<>, propertyValue, v8Value);
    v8::String::Utf8Value namedProperty(name);
    ExceptionState exceptionState(ExceptionState::SetterContext, *namedProperty, "TestInterface2", info.Holder(), info.GetIsolate());
    bool result = impl->setNamedItem(propertyName, propertyValue, exceptionState);
    if (exceptionState.throwIfNeeded())
        return;
    if (!result)
        return;
    v8SetReturnValue(info, v8Value);
}

static void namedPropertySetterCallback(v8::Local<v8::String> name, v8::Local<v8::Value> v8Value, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMNamedProperty");
    TestInterface2V8Internal::namedPropertySetter(name, v8Value, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "V8Execution");
}

static void namedPropertyQuery(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Integer>& info)
{
    TestInterface2* impl = V8TestInterface2::toNative(info.Holder());
    AtomicString propertyName = toCoreAtomicString(name);
    v8::String::Utf8Value namedProperty(name);
    ExceptionState exceptionState(ExceptionState::GetterContext, *namedProperty, "TestInterface2", info.Holder(), info.GetIsolate());
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
    TestInterface2V8Internal::namedPropertyQuery(name, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "V8Execution");
}

static void namedPropertyDeleter(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Boolean>& info)
{
    TestInterface2* impl = V8TestInterface2::toNative(info.Holder());
    AtomicString propertyName = toCoreAtomicString(name);
    v8::String::Utf8Value namedProperty(name);
    ExceptionState exceptionState(ExceptionState::DeletionContext, *namedProperty, "TestInterface2", info.Holder(), info.GetIsolate());
    DeleteResult result = impl->deleteNamedItem(propertyName, exceptionState);
    if (exceptionState.throwIfNeeded())
        return;
    if (result != DeleteUnknownProperty)
        return v8SetReturnValueBool(info, result == DeleteSuccess);
}

static void namedPropertyDeleterCallback(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Boolean>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMNamedProperty");
    TestInterface2V8Internal::namedPropertyDeleter(name, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "V8Execution");
}

static void namedPropertyEnumerator(const v8::PropertyCallbackInfo<v8::Array>& info)
{
    TestInterface2* impl = V8TestInterface2::toNative(info.Holder());
    v8::Isolate* isolate = info.GetIsolate();
    Vector<String> names;
    ExceptionState exceptionState(ExceptionState::EnumerationContext, "TestInterface2", info.Holder(), isolate);
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
    TestInterface2V8Internal::namedPropertyEnumerator(info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "V8Execution");
}

} // namespace TestInterface2V8Internal

void V8TestInterface2::visitDOMWrapper(void* object, const v8::Persistent<v8::Object>& wrapper, v8::Isolate* isolate)
{
    TestInterface2* impl = fromInternalPointer(object);
    // The ownerNode() method may return a reference or a pointer.
    if (Node* owner = WTF::getPtr(impl->ownerNode())) {
        Node* root = V8GCController::opaqueRootForGC(owner, isolate);
        isolate->SetReferenceFromGroup(v8::UniqueId(reinterpret_cast<intptr_t>(root)), wrapper);
        return;
    }
    setObjectGroup(object, wrapper, isolate);
}

static const V8DOMConfiguration::MethodConfiguration V8TestInterface2Methods[] = {
    {"item", TestInterface2V8Internal::itemMethodCallback, 0, 1},
    {"setItem", TestInterface2V8Internal::setItemMethodCallback, 0, 2},
    {"deleteItem", TestInterface2V8Internal::deleteItemMethodCallback, 0, 1},
    {"namedItem", TestInterface2V8Internal::namedItemMethodCallback, 0, 1},
    {"setNamedItem", TestInterface2V8Internal::setNamedItemMethodCallback, 0, 2},
    {"deleteNamedItem", TestInterface2V8Internal::deleteNamedItemMethodCallback, 0, 1},
};

void V8TestInterface2::constructorCallback(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SCOPED_SAMPLING_STATE("Blink", "DOMConstructor");
    if (!info.IsConstructCall()) {
        throwTypeError(ExceptionMessages::constructorNotCallableAsFunction("TestInterface2"), info.GetIsolate());
        return;
    }

    if (ConstructorMode::current() == ConstructorMode::WrapExistingObject) {
        v8SetReturnValue(info, info.Holder());
        return;
    }

    TestInterface2V8Internal::constructor(info);
}

static void configureV8TestInterface2Template(v8::Handle<v8::FunctionTemplate> functionTemplate, v8::Isolate* isolate)
{
    functionTemplate->ReadOnlyPrototype();

    v8::Local<v8::Signature> defaultSignature;
    defaultSignature = V8DOMConfiguration::installDOMClassTemplate(functionTemplate, "TestInterface2", v8::Local<v8::FunctionTemplate>(), V8TestInterface2::internalFieldCount,
        0, 0,
        0, 0,
        V8TestInterface2Methods, WTF_ARRAY_LENGTH(V8TestInterface2Methods),
        isolate);
    functionTemplate->SetCallHandler(V8TestInterface2::constructorCallback);
    functionTemplate->SetLength(0);
    v8::Local<v8::ObjectTemplate> ALLOW_UNUSED instanceTemplate = functionTemplate->InstanceTemplate();
    v8::Local<v8::ObjectTemplate> ALLOW_UNUSED prototypeTemplate = functionTemplate->PrototypeTemplate();
    functionTemplate->InstanceTemplate()->SetIndexedPropertyHandler(TestInterface2V8Internal::indexedPropertyGetterCallback, TestInterface2V8Internal::indexedPropertySetterCallback, 0, TestInterface2V8Internal::indexedPropertyDeleterCallback, indexedPropertyEnumerator<TestInterface2>);
    functionTemplate->InstanceTemplate()->SetNamedPropertyHandler(TestInterface2V8Internal::namedPropertyGetterCallback, TestInterface2V8Internal::namedPropertySetterCallback, TestInterface2V8Internal::namedPropertyQueryCallback, TestInterface2V8Internal::namedPropertyDeleterCallback, TestInterface2V8Internal::namedPropertyEnumeratorCallback);

    // Custom toString template
    functionTemplate->Set(v8AtomicString(isolate, "toString"), V8PerIsolateData::current()->toStringTemplate());
}

v8::Handle<v8::FunctionTemplate> V8TestInterface2::domTemplate(v8::Isolate* isolate)
{
    V8PerIsolateData* data = V8PerIsolateData::from(isolate);
    v8::Local<v8::FunctionTemplate> result = data->existingDOMTemplate(const_cast<WrapperTypeInfo*>(&wrapperTypeInfo));
    if (!result.IsEmpty())
        return result;

    TRACE_EVENT_SCOPED_SAMPLING_STATE("Blink", "BuildDOMTemplate");
    result = v8::FunctionTemplate::New(isolate, V8ObjectConstructor::isValidConstructorMode);
    configureV8TestInterface2Template(result, isolate);
    data->setDOMTemplate(const_cast<WrapperTypeInfo*>(&wrapperTypeInfo), result);
    return result;
}

bool V8TestInterface2::hasInstance(v8::Handle<v8::Value> v8Value, v8::Isolate* isolate)
{
    return V8PerIsolateData::from(isolate)->hasInstance(&wrapperTypeInfo, v8Value);
}

v8::Handle<v8::Object> V8TestInterface2::findInstanceInPrototypeChain(v8::Handle<v8::Value> v8Value, v8::Isolate* isolate)
{
    return V8PerIsolateData::from(isolate)->findInstanceInPrototypeChain(&wrapperTypeInfo, v8Value);
}

TestInterface2* V8TestInterface2::toNativeWithTypeCheck(v8::Isolate* isolate, v8::Handle<v8::Value> value)
{
    return hasInstance(value, isolate) ? fromInternalPointer(v8::Handle<v8::Object>::Cast(value)->GetAlignedPointerFromInternalField(v8DOMWrapperObjectIndex)) : 0;
}

v8::Handle<v8::Object> wrap(TestInterface2* impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    ASSERT(impl);
    if (impl->isInterface1())
        return wrap(toInterface1(impl), creationContext, isolate);
    if (impl->isInterface2())
        return wrap(toInterface2(impl), creationContext, isolate);
    v8::Handle<v8::Object> wrapper = V8TestInterface2::createWrapper(impl, creationContext, isolate);
    return wrapper;
}

v8::Handle<v8::Object> V8TestInterface2::createWrapper(PassRefPtr<TestInterface2> impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    ASSERT(impl);
    ASSERT(!DOMDataStore::containsWrapper<V8TestInterface2>(impl.get(), isolate));
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
    V8DOMWrapper::associateObjectWithWrapper<V8TestInterface2>(impl, &wrapperTypeInfo, wrapper, isolate, WrapperConfiguration::Dependent);
    return wrapper;
}

void V8TestInterface2::derefObject(void* object)
{
    fromInternalPointer(object)->deref();
}

template<>
v8::Handle<v8::Value> toV8NoInline(TestInterface2* impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    return toV8(impl, creationContext, isolate);
}

} // namespace WebCore
