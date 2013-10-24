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
#include "V8SupportTestInterface.h"

#include "RuntimeEnabledFeatures.h"
#include "V8Node.h"
#include "V8TestObject.h"
#include "bindings/tests/idls/testing/SupportTestPartialInterface.h"
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

static void initializeScriptWrappableForInterface(SupportTestInterface* object)
{
    if (ScriptWrappable::wrapperCanBeStoredInObject(object))
        ScriptWrappable::setTypeInfoInObject(object, &V8SupportTestInterface::wrapperTypeInfo);
    else
        ASSERT_NOT_REACHED();
}

} // namespace WebCore

// In ScriptWrappable::init, the use of a local function declaration has an issue on Windows:
// the local declaration does not pick up the surrounding namespace. Therefore, we provide this function
// in the global namespace.
// (More info on the MSVC bug here: http://connect.microsoft.com/VisualStudio/feedback/details/664619/the-namespace-of-local-function-declarations-in-c)
void webCoreInitializeScriptWrappableForInterface(WebCore::SupportTestInterface* object)
{
    WebCore::initializeScriptWrappableForInterface(object);
}

namespace WebCore {
WrapperTypeInfo V8SupportTestInterface::wrapperTypeInfo = { V8SupportTestInterface::GetTemplate, V8SupportTestInterface::derefObject, 0, 0, 0, V8SupportTestInterface::installPerContextEnabledPrototypeProperties, 0, WrapperTypeObjectPrototype };

namespace SupportTestInterfaceV8Internal {

template <typename T> void V8_USE(T) { }

#if ENABLE(Condition11) || ENABLE(Condition12)
static void supplementalStaticReadOnlyAttrAttributeGetter(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    v8SetReturnValueInt(info, SupportTestPartialInterface::supplementalStaticReadOnlyAttr());
}
#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)
static void supplementalStaticReadOnlyAttrAttributeGetterCallback(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMGetter");
    SupportTestInterfaceV8Internal::supplementalStaticReadOnlyAttrAttributeGetter(name, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}
#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)
static void supplementalStaticAttrAttributeGetter(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    v8SetReturnValueString(info, SupportTestPartialInterface::supplementalStaticAttr(), info.GetIsolate());
}
#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)
static void supplementalStaticAttrAttributeGetterCallback(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMGetter");
    SupportTestInterfaceV8Internal::supplementalStaticAttrAttributeGetter(name, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}
#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)
static void supplementalStaticAttrAttributeSetter(v8::Local<v8::String> name, v8::Local<v8::Value> jsValue, const v8::PropertyCallbackInfo<void>& info)
{
    V8TRYCATCH_FOR_V8STRINGRESOURCE_VOID(V8StringResource<>, cppValue, jsValue);
    SupportTestPartialInterface::setSupplementalStaticAttr(cppValue);
}
#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)
static void supplementalStaticAttrAttributeSetterCallback(v8::Local<v8::String> name, v8::Local<v8::Value> jsValue, const v8::PropertyCallbackInfo<void>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMSetter");
    SupportTestInterfaceV8Internal::supplementalStaticAttrAttributeSetter(name, jsValue, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}
#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)
static void supplementalStr1AttributeGetter(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    SupportTestInterface* imp = V8SupportTestInterface::toNative(info.Holder());
    v8SetReturnValueString(info, SupportTestPartialInterface::supplementalStr1(imp), info.GetIsolate());
}
#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)
static void supplementalStr1AttributeGetterCallback(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMGetter");
    SupportTestInterfaceV8Internal::supplementalStr1AttributeGetter(name, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}
#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)
static void supplementalStr2AttributeGetter(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    SupportTestInterface* imp = V8SupportTestInterface::toNative(info.Holder());
    v8SetReturnValueString(info, SupportTestPartialInterface::supplementalStr2(imp), info.GetIsolate());
}
#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)
static void supplementalStr2AttributeGetterCallback(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMGetter");
    SupportTestInterfaceV8Internal::supplementalStr2AttributeGetter(name, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}
#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)
static void supplementalStr2AttributeSetter(v8::Local<v8::String> name, v8::Local<v8::Value> jsValue, const v8::PropertyCallbackInfo<void>& info)
{
    SupportTestInterface* imp = V8SupportTestInterface::toNative(info.Holder());
    V8TRYCATCH_FOR_V8STRINGRESOURCE_VOID(V8StringResource<>, cppValue, jsValue);
    SupportTestPartialInterface::setSupplementalStr2(imp, cppValue);
}
#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)
static void supplementalStr2AttributeSetterCallback(v8::Local<v8::String> name, v8::Local<v8::Value> jsValue, const v8::PropertyCallbackInfo<void>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMSetter");
    SupportTestInterfaceV8Internal::supplementalStr2AttributeSetter(name, jsValue, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}
#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)
static void supplementalStr3AttributeGetterCallback(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMGetter");
    V8SupportTestInterface::supplementalStr3AttributeGetterCustom(name, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}
#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)
static void supplementalStr3AttributeSetterCallback(v8::Local<v8::String> name, v8::Local<v8::Value> jsValue, const v8::PropertyCallbackInfo<void>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMSetter");
    V8SupportTestInterface::supplementalStr3AttributeSetterCustom(name, jsValue, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}
#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)
static void supplementalNodeAttributeGetter(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    SupportTestInterface* imp = V8SupportTestInterface::toNative(info.Holder());
    v8SetReturnValueFast(info, SupportTestPartialInterface::supplementalNode(imp), imp);
}
#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)
static void supplementalNodeAttributeGetterCallback(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMGetter");
    SupportTestInterfaceV8Internal::supplementalNodeAttributeGetter(name, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}
#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)
static void supplementalNodeAttributeSetter(v8::Local<v8::String> name, v8::Local<v8::Value> jsValue, const v8::PropertyCallbackInfo<void>& info)
{
    SupportTestInterface* imp = V8SupportTestInterface::toNative(info.Holder());
    V8TRYCATCH_VOID(Node*, cppValue, V8Node::HasInstance(jsValue, info.GetIsolate(), worldType(info.GetIsolate())) ? V8Node::toNative(v8::Handle<v8::Object>::Cast(jsValue)) : 0);
    SupportTestPartialInterface::setSupplementalNode(imp, WTF::getPtr(cppValue));
}
#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)
static void supplementalNodeAttributeSetterCallback(v8::Local<v8::String> name, v8::Local<v8::Value> jsValue, const v8::PropertyCallbackInfo<void>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMSetter");
    SupportTestInterfaceV8Internal::supplementalNodeAttributeSetter(name, jsValue, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}
#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)
static void Node13AttributeGetter(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    SupportTestInterface* imp = V8SupportTestInterface::toNative(info.Holder());
    v8SetReturnValueFast(info, SupportTestPartialInterface::node13(imp), imp);
}
#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)
static void Node13AttributeGetterCallback(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMGetter");
    SupportTestInterfaceV8Internal::Node13AttributeGetter(name, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}
#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)
static void Node13AttributeSetter(v8::Local<v8::String> name, v8::Local<v8::Value> jsValue, const v8::PropertyCallbackInfo<void>& info)
{
    SupportTestInterface* imp = V8SupportTestInterface::toNative(info.Holder());
    V8TRYCATCH_VOID(Node*, cppValue, V8Node::HasInstance(jsValue, info.GetIsolate(), worldType(info.GetIsolate())) ? V8Node::toNative(v8::Handle<v8::Object>::Cast(jsValue)) : 0);
    SupportTestPartialInterface::setNode13(imp, WTF::getPtr(cppValue));
}
#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)
static void Node13AttributeSetterCallback(v8::Local<v8::String> name, v8::Local<v8::Value> jsValue, const v8::PropertyCallbackInfo<void>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMSetter");
    SupportTestInterfaceV8Internal::Node13AttributeSetter(name, jsValue, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}
#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)
static void Node14AttributeGetter(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    SupportTestInterface* imp = V8SupportTestInterface::toNative(info.Holder());
    v8SetReturnValueFast(info, SupportTestPartialInterface::node14(imp), imp);
}
#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)
static void Node14AttributeGetterCallback(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMGetter");
    SupportTestInterfaceV8Internal::Node14AttributeGetter(name, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}
#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)
static void Node14AttributeSetter(v8::Local<v8::String> name, v8::Local<v8::Value> jsValue, const v8::PropertyCallbackInfo<void>& info)
{
    SupportTestInterface* imp = V8SupportTestInterface::toNative(info.Holder());
    V8TRYCATCH_VOID(Node*, cppValue, V8Node::HasInstance(jsValue, info.GetIsolate(), worldType(info.GetIsolate())) ? V8Node::toNative(v8::Handle<v8::Object>::Cast(jsValue)) : 0);
    SupportTestPartialInterface::setNode14(imp, WTF::getPtr(cppValue));
}
#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)
static void Node14AttributeSetterCallback(v8::Local<v8::String> name, v8::Local<v8::Value> jsValue, const v8::PropertyCallbackInfo<void>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMSetter");
    SupportTestInterfaceV8Internal::Node14AttributeSetter(name, jsValue, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}
#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)

static void supplementalMethod1Method(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    SupportTestInterface* imp = V8SupportTestInterface::toNative(args.Holder());
    SupportTestPartialInterface::supplementalMethod1(imp);

    return;
}

#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)

static void supplementalMethod1MethodCallback(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMMethod");
    SupportTestInterfaceV8Internal::supplementalMethod1Method(args);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)

static void supplementalMethod2Method(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if (UNLIKELY(args.Length() < 2)) {
        throwTypeError(ExceptionMessages::failedToExecute("supplementalMethod2", "SupportTestInterface", ExceptionMessages::notEnoughArguments(2, args.Length())), args.GetIsolate());
        return;
    }
    SupportTestInterface* imp = V8SupportTestInterface::toNative(args.Holder());
    ExceptionState es(args.GetIsolate());
    V8TRYCATCH_FOR_V8STRINGRESOURCE_VOID(V8StringResource<>, strArg, args[0]);
    V8TRYCATCH_VOID(TestObj*, objArg, V8TestObject::HasInstance(args[1], args.GetIsolate(), worldType(args.GetIsolate())) ? V8TestObject::toNative(v8::Handle<v8::Object>::Cast(args[1])) : 0);
    ExecutionContext* scriptContext = getExecutionContext();
    RefPtr<TestObj> result = SupportTestPartialInterface::supplementalMethod2(scriptContext, imp, strArg, objArg, es);
    if (es.throwIfNeeded())
        return;
    v8SetReturnValue(args, result.release(), args.Holder());
    return;
}

#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)

static void supplementalMethod2MethodCallback(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMMethod");
    SupportTestInterfaceV8Internal::supplementalMethod2Method(args);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)

static void supplementalMethod3MethodCallback(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMMethod");
    V8SupportTestInterface::supplementalMethod3MethodCustom(args);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)

static void supplementalMethod4Method(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    SupportTestPartialInterface::supplementalMethod4();

    return;
}

#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)

static void supplementalMethod4MethodCallback(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMMethod");
    SupportTestInterfaceV8Internal::supplementalMethod4Method(args);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

#endif // ENABLE(Condition11) || ENABLE(Condition12)

} // namespace SupportTestInterfaceV8Internal

static const V8DOMConfiguration::AttributeConfiguration V8SupportTestInterfaceAttributes[] = {
#if ENABLE(Condition11) || ENABLE(Condition12)
    {"supplementalStr1", SupportTestInterfaceV8Internal::supplementalStr1AttributeGetterCallback, 0, 0, 0, 0, static_cast<v8::AccessControl>(v8::DEFAULT), static_cast<v8::PropertyAttribute>(v8::None), 0 /* on instance */},
#endif // ENABLE(Condition11) || ENABLE(Condition12)
#if ENABLE(Condition11) || ENABLE(Condition12)
    {"supplementalStr2", SupportTestInterfaceV8Internal::supplementalStr2AttributeGetterCallback, SupportTestInterfaceV8Internal::supplementalStr2AttributeSetterCallback, 0, 0, 0, static_cast<v8::AccessControl>(v8::DEFAULT), static_cast<v8::PropertyAttribute>(v8::None), 0 /* on instance */},
#endif // ENABLE(Condition11) || ENABLE(Condition12)
#if ENABLE(Condition11) || ENABLE(Condition12)
    {"supplementalStr3", SupportTestInterfaceV8Internal::supplementalStr3AttributeGetterCallback, SupportTestInterfaceV8Internal::supplementalStr3AttributeSetterCallback, 0, 0, 0, static_cast<v8::AccessControl>(v8::DEFAULT), static_cast<v8::PropertyAttribute>(v8::None), 0 /* on instance */},
#endif // ENABLE(Condition11) || ENABLE(Condition12)
#if ENABLE(Condition11) || ENABLE(Condition12)
    {"supplementalNode", SupportTestInterfaceV8Internal::supplementalNodeAttributeGetterCallback, SupportTestInterfaceV8Internal::supplementalNodeAttributeSetterCallback, 0, 0, 0, static_cast<v8::AccessControl>(v8::DEFAULT), static_cast<v8::PropertyAttribute>(v8::None), 0 /* on instance */},
#endif // ENABLE(Condition11) || ENABLE(Condition12)
};

static const V8DOMConfiguration::MethodConfiguration V8SupportTestInterfaceMethods[] = {
#if ENABLE(Condition11) || ENABLE(Condition12)
    {"supplementalMethod1", SupportTestInterfaceV8Internal::supplementalMethod1MethodCallback, 0, 0},
#endif // ENABLE(Condition11) || ENABLE(Condition12)
#if ENABLE(Condition11) || ENABLE(Condition12)
    {"supplementalMethod3", SupportTestInterfaceV8Internal::supplementalMethod3MethodCallback, 0, 0},
#endif // ENABLE(Condition11) || ENABLE(Condition12)
};

static v8::Handle<v8::FunctionTemplate> ConfigureV8SupportTestInterfaceTemplate(v8::Handle<v8::FunctionTemplate> desc, v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    desc->ReadOnlyPrototype();

    v8::Local<v8::Signature> defaultSignature;
    defaultSignature = V8DOMConfiguration::installDOMClassTemplate(desc, "SupportTestInterface", v8::Local<v8::FunctionTemplate>(), V8SupportTestInterface::internalFieldCount,
        V8SupportTestInterfaceAttributes, WTF_ARRAY_LENGTH(V8SupportTestInterfaceAttributes),
        V8SupportTestInterfaceMethods, WTF_ARRAY_LENGTH(V8SupportTestInterfaceMethods), isolate, currentWorldType);
    UNUSED_PARAM(defaultSignature);
    v8::Local<v8::ObjectTemplate> instance = desc->InstanceTemplate();
    v8::Local<v8::ObjectTemplate> proto = desc->PrototypeTemplate();
    UNUSED_PARAM(instance);
    UNUSED_PARAM(proto);
#if ENABLE(Condition11) || ENABLE(Condition12)
    if (RuntimeEnabledFeatures::featureName13Enabled()) {
        static const V8DOMConfiguration::AttributeConfiguration attributeConfiguration =\
        {"Node13", SupportTestInterfaceV8Internal::Node13AttributeGetterCallback, SupportTestInterfaceV8Internal::Node13AttributeSetterCallback, 0, 0, 0, static_cast<v8::AccessControl>(v8::DEFAULT), static_cast<v8::PropertyAttribute>(v8::None), 0 /* on instance */};
        V8DOMConfiguration::installAttribute(instance, proto, attributeConfiguration, isolate, currentWorldType);
    }
#endif // ENABLE(Condition11) || ENABLE(Condition12)
    static const V8DOMConfiguration::ConstantConfiguration V8SupportTestInterfaceConstants[] = {
        {"SUPPLEMENTALCONSTANT1", 1},
        {"SUPPLEMENTALCONSTANT2", 2},
    };
    V8DOMConfiguration::installConstants(desc, proto, V8SupportTestInterfaceConstants, WTF_ARRAY_LENGTH(V8SupportTestInterfaceConstants), isolate);
    COMPILE_ASSERT(1 == SupportTestPartialInterface::SUPPLEMENTALCONSTANT1, TheValueOfSupportTestInterface_SUPPLEMENTALCONSTANT1DoesntMatchWithImplementation);
    COMPILE_ASSERT(2 == SupportTestPartialInterface::CONST_IMPL, TheValueOfSupportTestInterface_CONST_IMPLDoesntMatchWithImplementation);
#if ENABLE(Condition11) || ENABLE(Condition12)

    // Custom Signature 'supplementalMethod2'
    const int supplementalMethod2Argc = 2;
    v8::Handle<v8::FunctionTemplate> supplementalMethod2Argv[supplementalMethod2Argc] = { v8::Handle<v8::FunctionTemplate>(), V8PerIsolateData::from(isolate)->rawTemplate(&V8TestObject::wrapperTypeInfo, currentWorldType) };
    v8::Handle<v8::Signature> supplementalMethod2Signature = v8::Signature::New(desc, supplementalMethod2Argc, supplementalMethod2Argv);
    proto->Set(v8::String::NewSymbol("supplementalMethod2"), v8::FunctionTemplate::New(SupportTestInterfaceV8Internal::supplementalMethod2MethodCallback, v8Undefined(), supplementalMethod2Signature, 2));
#endif // ENABLE(Condition11) || ENABLE(Condition12)
#if ENABLE(Condition11) || ENABLE(Condition12)
    desc->Set(v8::String::NewSymbol("supplementalMethod4"), v8::FunctionTemplate::New(SupportTestInterfaceV8Internal::supplementalMethod4MethodCallback, v8Undefined(), v8::Local<v8::Signature>(), 0));
#endif // ENABLE(Condition11) || ENABLE(Condition12)
#if ENABLE(Condition11) || ENABLE(Condition12)
    desc->SetNativeDataProperty(v8::String::NewSymbol("supplementalStaticReadOnlyAttr"), SupportTestInterfaceV8Internal::supplementalStaticReadOnlyAttrAttributeGetterCallback, 0, v8::External::New(0), static_cast<v8::PropertyAttribute>(v8::None), v8::Handle<v8::AccessorSignature>(), static_cast<v8::AccessControl>(v8::DEFAULT));
#endif // ENABLE(Condition11) || ENABLE(Condition12)
#if ENABLE(Condition11) || ENABLE(Condition12)
    desc->SetNativeDataProperty(v8::String::NewSymbol("supplementalStaticAttr"), SupportTestInterfaceV8Internal::supplementalStaticAttrAttributeGetterCallback, SupportTestInterfaceV8Internal::supplementalStaticAttrAttributeSetterCallback, v8::External::New(0), static_cast<v8::PropertyAttribute>(v8::None), v8::Handle<v8::AccessorSignature>(), static_cast<v8::AccessControl>(v8::DEFAULT));
#endif // ENABLE(Condition11) || ENABLE(Condition12)

    // Custom toString template
    desc->Set(v8::String::NewSymbol("toString"), V8PerIsolateData::current()->toStringTemplate());
    return desc;
}

v8::Handle<v8::FunctionTemplate> V8SupportTestInterface::GetTemplate(v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    V8PerIsolateData* data = V8PerIsolateData::from(isolate);
    V8PerIsolateData::TemplateMap::iterator result = data->templateMap(currentWorldType).find(&wrapperTypeInfo);
    if (result != data->templateMap(currentWorldType).end())
        return result->value.newLocal(isolate);

    TRACE_EVENT_SCOPED_SAMPLING_STATE("Blink", "BuildDOMTemplate");
    v8::HandleScope handleScope(isolate);
    v8::Handle<v8::FunctionTemplate> templ =
        ConfigureV8SupportTestInterfaceTemplate(data->rawTemplate(&wrapperTypeInfo, currentWorldType), isolate, currentWorldType);
    data->templateMap(currentWorldType).add(&wrapperTypeInfo, UnsafePersistent<v8::FunctionTemplate>(isolate, templ));
    return handleScope.Close(templ);
}

bool V8SupportTestInterface::HasInstance(v8::Handle<v8::Value> jsValue, v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    return V8PerIsolateData::from(isolate)->hasInstance(&wrapperTypeInfo, jsValue, currentWorldType);
}

bool V8SupportTestInterface::HasInstanceInAnyWorld(v8::Handle<v8::Value> jsValue, v8::Isolate* isolate)
{
    return V8PerIsolateData::from(isolate)->hasInstance(&wrapperTypeInfo, jsValue, MainWorld)
        || V8PerIsolateData::from(isolate)->hasInstance(&wrapperTypeInfo, jsValue, IsolatedWorld)
        || V8PerIsolateData::from(isolate)->hasInstance(&wrapperTypeInfo, jsValue, WorkerWorld);
}

void V8SupportTestInterface::installPerContextEnabledProperties(v8::Handle<v8::Object> instance, SupportTestInterface* impl, v8::Isolate* isolate)
{
    v8::Local<v8::Object> proto = v8::Local<v8::Object>::Cast(instance->GetPrototype());
    if (ContextFeatures::featureName14Enabled(impl->document())) {
        static const V8DOMConfiguration::AttributeConfiguration attributeConfiguration =\
        {"Node14", SupportTestInterfaceV8Internal::Node14AttributeGetterCallback, SupportTestInterfaceV8Internal::Node14AttributeSetterCallback, 0, 0, 0, static_cast<v8::AccessControl>(v8::DEFAULT), static_cast<v8::PropertyAttribute>(v8::None), 0 /* on instance */};
        V8DOMConfiguration::installAttribute(instance, proto, attributeConfiguration, isolate);
    }
}

v8::Handle<v8::Object> V8SupportTestInterface::createWrapper(PassRefPtr<SupportTestInterface> impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    ASSERT(impl);
    ASSERT(!DOMDataStore::containsWrapper<V8SupportTestInterface>(impl.get(), isolate));
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
    V8DOMWrapper::associateObjectWithWrapper<V8SupportTestInterface>(impl, &wrapperTypeInfo, wrapper, isolate, WrapperConfiguration::Independent);
    return wrapper;
}

void V8SupportTestInterface::derefObject(void* object)
{
    fromInternalPointer(object)->deref();
}

} // namespace WebCore
