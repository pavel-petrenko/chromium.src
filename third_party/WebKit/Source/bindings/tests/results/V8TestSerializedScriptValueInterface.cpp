/*
    This file is part of the WebKit open source project.
    This file has been generated by generate-bindings.pl. DO NOT MODIFY!

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#include "config.h"
#if ENABLE(Condition1) || ENABLE(Condition2)
#include "V8TestSerializedScriptValueInterface.h"

#include "MessagePort.h"
#include "SerializedScriptValue.h"
#include "V8MessagePort.h"
#include "bindings/v8/BindingState.h"
#include "bindings/v8/ScriptController.h"
#include "bindings/v8/SerializedScriptValue.h"
#include "bindings/v8/V8Binding.h"
#include "bindings/v8/V8DOMWrapper.h"
#include "core/dom/ContextFeatures.h"
#include "core/page/Frame.h"
#include "core/page/RuntimeEnabledFeatures.h"
#include <wtf/UnusedParam.h>

#if ENABLE(BINDING_INTEGRITY)
#if defined(OS_WIN)
#pragma warning(disable: 4483)
extern "C" { extern void (*const __identifier("??_7TestSerializedScriptValueInterface@WebCore@@6B@")[])(); }
#else
extern "C" { extern void* _ZTVN7WebCore34TestSerializedScriptValueInterfaceE[]; }
#endif
#endif // ENABLE(BINDING_INTEGRITY)

namespace WebCore {

#if ENABLE(BINDING_INTEGRITY)
// This checks if a DOM object that is about to be wrapped is valid.
// Specifically, it checks that a vtable of the DOM object is equal to
// a vtable of an expected class.
// Due to a dangling pointer, the DOM object you are wrapping might be
// already freed or realloced. If freed, the check will fail because
// a free list pointer should be stored at the head of the DOM object.
// If realloced, the check will fail because the vtable of the DOM object
// differs from the expected vtable (unless the same class of DOM object
// is realloced on the slot).
inline void checkTypeOrDieTrying(TestSerializedScriptValueInterface* object)
{
    void* actualVTablePointer = *(reinterpret_cast<void**>(object));
#if defined(OS_WIN)
    void* expectedVTablePointer = reinterpret_cast<void*>(__identifier("??_7TestSerializedScriptValueInterface@WebCore@@6B@"));
#else
    void* expectedVTablePointer = &_ZTVN7WebCore34TestSerializedScriptValueInterfaceE[2];
#endif
    if (actualVTablePointer != expectedVTablePointer)
        CRASH();
}
#endif // ENABLE(BINDING_INTEGRITY)

#if defined(OS_WIN)
// In ScriptWrappable, the use of extern function prototypes inside templated static methods has an issue on windows.
// These prototypes do not pick up the surrounding namespace, so drop out of WebCore as a workaround.
} // namespace WebCore
using WebCore::ScriptWrappable;
using WebCore::V8TestSerializedScriptValueInterface;
using WebCore::TestSerializedScriptValueInterface;
#endif
void initializeScriptWrappableForInterface(TestSerializedScriptValueInterface* object)
{
    if (ScriptWrappable::wrapperCanBeStoredInObject(object))
        ScriptWrappable::setTypeInfoInObject(object, &V8TestSerializedScriptValueInterface::info);
}
#if defined(OS_WIN)
namespace WebCore {
#endif
WrapperTypeInfo V8TestSerializedScriptValueInterface::info = { V8TestSerializedScriptValueInterface::GetTemplate, V8TestSerializedScriptValueInterface::derefObject, 0, 0, 0, V8TestSerializedScriptValueInterface::installPerContextPrototypeProperties, 0, WrapperTypeObjectPrototype };

namespace TestSerializedScriptValueInterfaceV8Internal {

template <typename T> void V8_USE(T) { }

static v8::Handle<v8::Value> valueAttrGetter(v8::Local<v8::String> name, const v8::AccessorInfo& info)
{
    TestSerializedScriptValueInterface* imp = V8TestSerializedScriptValueInterface::toNative(info.Holder());
    return imp->value() ? imp->value()->deserialize() : v8::Handle<v8::Value>(v8Null(info.GetIsolate()));
}

static v8::Handle<v8::Value> valueAttrGetterCallback(v8::Local<v8::String> name, const v8::AccessorInfo& info)
{
    return TestSerializedScriptValueInterfaceV8Internal::valueAttrGetter(name, info);
}

static void valueAttrSetter(v8::Local<v8::String> name, v8::Local<v8::Value> value, const v8::AccessorInfo& info)
{
    TestSerializedScriptValueInterface* imp = V8TestSerializedScriptValueInterface::toNative(info.Holder());
    V8TRYCATCH_VOID(RefPtr<SerializedScriptValue>, v, SerializedScriptValue::create(value, info.GetIsolate()));
    imp->setValue(WTF::getPtr(v));
    return;
}

static void valueAttrSetterCallback(v8::Local<v8::String> name, v8::Local<v8::Value> value, const v8::AccessorInfo& info)
{
    TestSerializedScriptValueInterfaceV8Internal::valueAttrSetter(name, value, info);
}

static v8::Handle<v8::Value> readonlyValueAttrGetter(v8::Local<v8::String> name, const v8::AccessorInfo& info)
{
    TestSerializedScriptValueInterface* imp = V8TestSerializedScriptValueInterface::toNative(info.Holder());
    return imp->readonlyValue() ? imp->readonlyValue()->deserialize() : v8::Handle<v8::Value>(v8Null(info.GetIsolate()));
}

static v8::Handle<v8::Value> readonlyValueAttrGetterCallback(v8::Local<v8::String> name, const v8::AccessorInfo& info)
{
    return TestSerializedScriptValueInterfaceV8Internal::readonlyValueAttrGetter(name, info);
}

static v8::Handle<v8::Value> cachedValueAttrGetter(v8::Local<v8::String> name, const v8::AccessorInfo& info)
{
    v8::Handle<v8::String> propertyName = v8::String::NewSymbol("cachedValue");
    v8::Handle<v8::Value> value = info.Holder()->GetHiddenValue(propertyName);
    if (!value.IsEmpty())
        return value;
    TestSerializedScriptValueInterface* imp = V8TestSerializedScriptValueInterface::toNative(info.Holder());
    RefPtr<SerializedScriptValue> serialized = imp->cachedValue();
    value = serialized ? serialized->deserialize() : v8::Handle<v8::Value>(v8Null(info.GetIsolate()));
    info.Holder()->SetHiddenValue(propertyName, value);
    return value;
}

static v8::Handle<v8::Value> cachedValueAttrGetterCallback(v8::Local<v8::String> name, const v8::AccessorInfo& info)
{
    return TestSerializedScriptValueInterfaceV8Internal::cachedValueAttrGetter(name, info);
}

static void cachedValueAttrSetter(v8::Local<v8::String> name, v8::Local<v8::Value> value, const v8::AccessorInfo& info)
{
    TestSerializedScriptValueInterface* imp = V8TestSerializedScriptValueInterface::toNative(info.Holder());
    V8TRYCATCH_VOID(RefPtr<SerializedScriptValue>, v, SerializedScriptValue::create(value, info.GetIsolate()));
    imp->setCachedValue(WTF::getPtr(v));
    info.Holder()->DeleteHiddenValue(v8::String::NewSymbol("cachedValue")); // Invalidate the cached value.
    return;
}

static void cachedValueAttrSetterCallback(v8::Local<v8::String> name, v8::Local<v8::Value> value, const v8::AccessorInfo& info)
{
    TestSerializedScriptValueInterfaceV8Internal::cachedValueAttrSetter(name, value, info);
}

static v8::Handle<v8::Value> portsAttrGetter(v8::Local<v8::String> name, const v8::AccessorInfo& info)
{
    TestSerializedScriptValueInterface* imp = V8TestSerializedScriptValueInterface::toNative(info.Holder());
    MessagePortArray* ports = imp->ports();
    if (!ports)
        return v8::Array::New(0);
    MessagePortArray portsCopy(*ports);
    v8::Local<v8::Array> portArray = v8::Array::New(portsCopy.size());
    for (size_t i = 0; i < portsCopy.size(); ++i)
        portArray->Set(v8Integer(i, info.GetIsolate()), toV8Fast(portsCopy[i].get(), info, imp));
    return portArray;
}

static v8::Handle<v8::Value> portsAttrGetterCallback(v8::Local<v8::String> name, const v8::AccessorInfo& info)
{
    return TestSerializedScriptValueInterfaceV8Internal::portsAttrGetter(name, info);
}

static v8::Handle<v8::Value> cachedReadonlyValueAttrGetter(v8::Local<v8::String> name, const v8::AccessorInfo& info)
{
    v8::Handle<v8::String> propertyName = v8::String::NewSymbol("cachedReadonlyValue");
    v8::Handle<v8::Value> value = info.Holder()->GetHiddenValue(propertyName);
    if (!value.IsEmpty())
        return value;
    TestSerializedScriptValueInterface* imp = V8TestSerializedScriptValueInterface::toNative(info.Holder());
    RefPtr<SerializedScriptValue> serialized = imp->cachedReadonlyValue();
    value = serialized ? serialized->deserialize() : v8::Handle<v8::Value>(v8Null(info.GetIsolate()));
    info.Holder()->SetHiddenValue(propertyName, value);
    return value;
}

static v8::Handle<v8::Value> cachedReadonlyValueAttrGetterCallback(v8::Local<v8::String> name, const v8::AccessorInfo& info)
{
    return TestSerializedScriptValueInterfaceV8Internal::cachedReadonlyValueAttrGetter(name, info);
}

} // namespace TestSerializedScriptValueInterfaceV8Internal

static const V8DOMConfiguration::BatchedAttribute V8TestSerializedScriptValueInterfaceAttrs[] = {
    // Attribute 'value' (Type: 'attribute' ExtAttr: '')
    {"value", TestSerializedScriptValueInterfaceV8Internal::valueAttrGetterCallback, TestSerializedScriptValueInterfaceV8Internal::valueAttrSetterCallback, 0, 0, 0 /* no data */, static_cast<v8::AccessControl>(v8::DEFAULT), static_cast<v8::PropertyAttribute>(v8::None), 0 /* on instance */},
    // Attribute 'readonlyValue' (Type: 'readonly attribute' ExtAttr: '')
    {"readonlyValue", TestSerializedScriptValueInterfaceV8Internal::readonlyValueAttrGetterCallback, 0, 0, 0, 0 /* no data */, static_cast<v8::AccessControl>(v8::DEFAULT), static_cast<v8::PropertyAttribute>(v8::None), 0 /* on instance */},
    // Attribute 'cachedValue' (Type: 'attribute' ExtAttr: 'CachedAttribute')
    {"cachedValue", TestSerializedScriptValueInterfaceV8Internal::cachedValueAttrGetterCallback, TestSerializedScriptValueInterfaceV8Internal::cachedValueAttrSetterCallback, 0, 0, 0 /* no data */, static_cast<v8::AccessControl>(v8::DEFAULT), static_cast<v8::PropertyAttribute>(v8::None), 0 /* on instance */},
    // Attribute 'ports' (Type: 'readonly attribute' ExtAttr: '')
    {"ports", TestSerializedScriptValueInterfaceV8Internal::portsAttrGetterCallback, 0, 0, 0, 0 /* no data */, static_cast<v8::AccessControl>(v8::DEFAULT), static_cast<v8::PropertyAttribute>(v8::None), 0 /* on instance */},
    // Attribute 'cachedReadonlyValue' (Type: 'readonly attribute' ExtAttr: 'CachedAttribute')
    {"cachedReadonlyValue", TestSerializedScriptValueInterfaceV8Internal::cachedReadonlyValueAttrGetterCallback, 0, 0, 0, 0 /* no data */, static_cast<v8::AccessControl>(v8::DEFAULT), static_cast<v8::PropertyAttribute>(v8::None), 0 /* on instance */},
};

static v8::Persistent<v8::FunctionTemplate> ConfigureV8TestSerializedScriptValueInterfaceTemplate(v8::Persistent<v8::FunctionTemplate> desc, v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    desc->ReadOnlyPrototype();

    v8::Local<v8::Signature> defaultSignature;
    defaultSignature = V8DOMConfiguration::configureTemplate(desc, "TestSerializedScriptValueInterface", v8::Persistent<v8::FunctionTemplate>(), V8TestSerializedScriptValueInterface::internalFieldCount,
        V8TestSerializedScriptValueInterfaceAttrs, WTF_ARRAY_LENGTH(V8TestSerializedScriptValueInterfaceAttrs),
        0, 0, isolate, currentWorldType);
    UNUSED_PARAM(defaultSignature); // In some cases, it will not be used.

    // Custom toString template
    desc->Set(v8::String::NewSymbol("toString"), V8PerIsolateData::current()->toStringTemplate());
    return desc;
}

v8::Persistent<v8::FunctionTemplate> V8TestSerializedScriptValueInterface::GetTemplate(v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    V8PerIsolateData* data = V8PerIsolateData::from(isolate);
    V8PerIsolateData::TemplateMap::iterator result = data->templateMap(currentWorldType).find(&info);
    if (result != data->templateMap(currentWorldType).end())
        return result->value;

    v8::HandleScope handleScope;
    v8::Persistent<v8::FunctionTemplate> templ =
        ConfigureV8TestSerializedScriptValueInterfaceTemplate(data->rawTemplate(&info, currentWorldType), isolate, currentWorldType);
    data->templateMap(currentWorldType).add(&info, templ);
    return templ;
}

bool V8TestSerializedScriptValueInterface::HasInstance(v8::Handle<v8::Value> value, v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    return V8PerIsolateData::from(isolate)->hasInstance(&info, value, currentWorldType);
}

bool V8TestSerializedScriptValueInterface::HasInstanceInAnyWorld(v8::Handle<v8::Value> value, v8::Isolate* isolate)
{
    return V8PerIsolateData::from(isolate)->hasInstance(&info, value, MainWorld)
        || V8PerIsolateData::from(isolate)->hasInstance(&info, value, IsolatedWorld)
        || V8PerIsolateData::from(isolate)->hasInstance(&info, value, WorkerWorld);
}


v8::Handle<v8::Object> V8TestSerializedScriptValueInterface::createWrapper(PassRefPtr<TestSerializedScriptValueInterface> impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    ASSERT(impl.get());
    ASSERT(DOMDataStore::getWrapper(impl.get(), isolate).IsEmpty());

#if ENABLE(BINDING_INTEGRITY)
    checkTypeOrDieTrying(impl.get());
#endif

    v8::Handle<v8::Object> wrapper = V8DOMWrapper::createWrapper(creationContext, &info, impl.get(), isolate);
    if (UNLIKELY(wrapper.IsEmpty()))
        return wrapper;

    installPerContextProperties(wrapper, impl.get(), isolate);
    V8DOMWrapper::associateObjectWithWrapper(impl, &info, wrapper, isolate, hasDependentLifetime ? WrapperConfiguration::Dependent : WrapperConfiguration::Independent);
    return wrapper;
}
void V8TestSerializedScriptValueInterface::derefObject(void* object)
{
    static_cast<TestSerializedScriptValueInterface*>(object)->deref();
}

} // namespace WebCore

#endif // ENABLE(Condition1) || ENABLE(Condition2)
