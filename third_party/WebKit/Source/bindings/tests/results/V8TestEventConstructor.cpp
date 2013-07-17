/*
    This file is part of the Blink open source project.
    This file has been auto-generated by CodeGeneratorV8.pm. DO NOT MODIFY!

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
#include "V8TestEventConstructor.h"

#include "RuntimeEnabledFeatures.h"
#include "bindings/v8/Dictionary.h"
#include "bindings/v8/ScriptController.h"
#include "bindings/v8/V8Binding.h"
#include "bindings/v8/V8DOMConfiguration.h"
#include "bindings/v8/V8DOMWrapper.h"
#include "bindings/v8/V8ObjectConstructor.h"
#include "core/dom/ContextFeatures.h"
#include "core/dom/Document.h"
#include "core/page/Frame.h"
#include "core/platform/chromium/TraceEvent.h"
#include "wtf/UnusedParam.h"

namespace WebCore {

static void initializeScriptWrappableForInterface(TestEventConstructor* object)
{
    if (ScriptWrappable::wrapperCanBeStoredInObject(object))
        ScriptWrappable::setTypeInfoInObject(object, &V8TestEventConstructor::info);
    else
        ASSERT_NOT_REACHED();
}

} // namespace WebCore

// In ScriptWrappable::init, the use of a local function declaration has an issue on Windows:
// the local declaration does not pick up the surrounding namespace. Therefore, we provide this function
// in the global namespace.
// (More info on the MSVC bug here: http://connect.microsoft.com/VisualStudio/feedback/details/664619/the-namespace-of-local-function-declarations-in-c)
void webCoreInitializeScriptWrappableForInterface(WebCore::TestEventConstructor* object)
{
    WebCore::initializeScriptWrappableForInterface(object);
}

namespace WebCore {
WrapperTypeInfo V8TestEventConstructor::info = { V8TestEventConstructor::GetTemplate, V8TestEventConstructor::derefObject, 0, 0, 0, V8TestEventConstructor::installPerContextPrototypeProperties, 0, WrapperTypeObjectPrototype };

namespace TestEventConstructorV8Internal {

template <typename T> void V8_USE(T) { }

static void attr1AttrGetter(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TestEventConstructor* imp = V8TestEventConstructor::toNative(info.Holder());
    v8SetReturnValueString(info, imp->attr1(), info.GetIsolate(), NullStringAsEmpty);
    return;
}

static void attr1AttrGetterCallback(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMGetter");
    TestEventConstructorV8Internal::attr1AttrGetter(name, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void attr2AttrGetter(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TestEventConstructor* imp = V8TestEventConstructor::toNative(info.Holder());
    v8SetReturnValueString(info, imp->attr2(), info.GetIsolate(), NullStringAsEmpty);
    return;
}

static void attr2AttrGetterCallback(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMGetter");
    TestEventConstructorV8Internal::attr2AttrGetter(name, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void constructor(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if (args.Length() < 1) {
        throwNotEnoughArgumentsError(args.GetIsolate());
        return;
    }

    V8TRYCATCH_FOR_V8STRINGRESOURCE_VOID(V8StringResource<>, type, args[0]);
    TestEventConstructorInit eventInit;
    if (args.Length() >= 2) {
        V8TRYCATCH_VOID(Dictionary, options, Dictionary(args[1], args.GetIsolate()));
        if (!fillTestEventConstructorInit(eventInit, options))
            return;
    }

    RefPtr<TestEventConstructor> event = TestEventConstructor::create(type, eventInit);

    v8::Handle<v8::Object> wrapper = args.Holder();
    V8DOMWrapper::associateObjectWithWrapper<V8TestEventConstructor>(event.release(), &V8TestEventConstructor::info, wrapper, args.GetIsolate(), WrapperConfiguration::Dependent);
    v8SetReturnValue(args, wrapper);
}
} // namespace TestEventConstructorV8Internal

static const V8DOMConfiguration::BatchedAttribute V8TestEventConstructorAttrs[] = {
    // Attribute 'attr1'
    {"attr1", TestEventConstructorV8Internal::attr1AttrGetterCallback, 0, 0, 0, 0 /* no data */, static_cast<v8::AccessControl>(v8::DEFAULT), static_cast<v8::PropertyAttribute>(v8::None), 0 /* on instance */},
    // Attribute 'attr2'
    {"attr2", TestEventConstructorV8Internal::attr2AttrGetterCallback, 0, 0, 0, 0 /* no data */, static_cast<v8::AccessControl>(v8::DEFAULT), static_cast<v8::PropertyAttribute>(v8::None), 0 /* on instance */},
};

bool fillTestEventConstructorInit(TestEventConstructorInit& eventInit, const Dictionary& options)
{
    options.get("attr2", eventInit.attr2);
    return true;
}

void V8TestEventConstructor::constructorCallback(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    TRACE_EVENT_SCOPED_SAMPLING_STATE("Blink", "DOMConstructor");
    if (!args.IsConstructCall()) {
        throwTypeError("DOM object constructor cannot be called as a function.", args.GetIsolate());
        return;
    }

    if (ConstructorMode::current() == ConstructorMode::WrapExistingObject) {
        args.GetReturnValue().Set(args.Holder());
        return;
    }

    TestEventConstructorV8Internal::constructor(args);
}

static v8::Handle<v8::FunctionTemplate> ConfigureV8TestEventConstructorTemplate(v8::Handle<v8::FunctionTemplate> desc, v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    desc->ReadOnlyPrototype();

    v8::Local<v8::Signature> defaultSignature;
    defaultSignature = V8DOMConfiguration::configureTemplate(desc, "TestEventConstructor", v8::Local<v8::FunctionTemplate>(), V8TestEventConstructor::internalFieldCount,
        V8TestEventConstructorAttrs, WTF_ARRAY_LENGTH(V8TestEventConstructorAttrs),
        0, 0, isolate, currentWorldType);
    UNUSED_PARAM(defaultSignature); // In some cases, it will not be used.
    desc->SetCallHandler(V8TestEventConstructor::constructorCallback);
    desc->SetLength(1);

    // Custom toString template
    desc->Set(v8::String::NewSymbol("toString"), V8PerIsolateData::current()->toStringTemplate());
    return desc;
}

v8::Handle<v8::FunctionTemplate> V8TestEventConstructor::GetTemplate(v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    V8PerIsolateData* data = V8PerIsolateData::from(isolate);
    V8PerIsolateData::TemplateMap::iterator result = data->templateMap(currentWorldType).find(&info);
    if (result != data->templateMap(currentWorldType).end())
        return result->value.newLocal(isolate);

    TRACE_EVENT_SCOPED_SAMPLING_STATE("Blink", "BuildDOMTemplate");
    v8::HandleScope handleScope(isolate);
    v8::Handle<v8::FunctionTemplate> templ =
        ConfigureV8TestEventConstructorTemplate(data->rawTemplate(&info, currentWorldType), isolate, currentWorldType);
    data->templateMap(currentWorldType).add(&info, UnsafePersistent<v8::FunctionTemplate>(isolate, templ));
    return handleScope.Close(templ);
}

bool V8TestEventConstructor::HasInstance(v8::Handle<v8::Value> value, v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    return V8PerIsolateData::from(isolate)->hasInstance(&info, value, currentWorldType);
}

bool V8TestEventConstructor::HasInstanceInAnyWorld(v8::Handle<v8::Value> value, v8::Isolate* isolate)
{
    return V8PerIsolateData::from(isolate)->hasInstance(&info, value, MainWorld)
        || V8PerIsolateData::from(isolate)->hasInstance(&info, value, IsolatedWorld)
        || V8PerIsolateData::from(isolate)->hasInstance(&info, value, WorkerWorld);
}


v8::Handle<v8::Object> V8TestEventConstructor::createWrapper(PassRefPtr<TestEventConstructor> impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    ASSERT(impl.get());
    ASSERT(DOMDataStore::getWrapper<V8TestEventConstructor>(impl.get(), isolate).IsEmpty());
    if (ScriptWrappable::wrapperCanBeStoredInObject(impl.get())) {
        const WrapperTypeInfo* actualInfo = ScriptWrappable::getTypeInfoFromObject(impl.get());
        // Might be a XXXConstructor::info instead of an XXX::info. These will both have
        // the same object de-ref functions, though, so use that as the basis of the check.
        RELEASE_ASSERT_WITH_SECURITY_IMPLICATION(actualInfo->derefObjectFunction == info.derefObjectFunction);
    }


    v8::Handle<v8::Object> wrapper = V8DOMWrapper::createWrapper(creationContext, &info, toInternalPointer(impl.get()), isolate);
    if (UNLIKELY(wrapper.IsEmpty()))
        return wrapper;
    installPerContextProperties(wrapper, impl.get(), isolate);
    V8DOMWrapper::associateObjectWithWrapper<V8TestEventConstructor>(impl, &info, wrapper, isolate, WrapperConfiguration::Independent);
    return wrapper;
}
void V8TestEventConstructor::derefObject(void* object)
{
    fromInternalPointer(object)->deref();
}

} // namespace WebCore
