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
#include "V8TestTypedefs.h"

#include "RuntimeEnabledFeatures.h"
#include "V8SVGPoint.h"
#include "V8SerializedScriptValue.h"
#include "V8TestCallback.h"
#include "V8TestSubObj.h"
#include "bindings/v8/ExceptionMessages.h"
#include "bindings/v8/ExceptionState.h"
#include "bindings/v8/ScriptController.h"
#include "bindings/v8/SerializedScriptValue.h"
#include "bindings/v8/V8Binding.h"
#include "bindings/v8/V8DOMConfiguration.h"
#include "bindings/v8/V8DOMWrapper.h"
#include "bindings/v8/V8ObjectConstructor.h"
#include "core/dom/ContextFeatures.h"
#include "core/dom/Document.h"
#include "core/platform/chromium/TraceEvent.h"
#include "core/svg/properties/SVGPropertyTearOff.h"
#include "wtf/UnusedParam.h"

namespace WebCore {

static void initializeScriptWrappableForInterface(TestTypedefs* object)
{
    if (ScriptWrappable::wrapperCanBeStoredInObject(object))
        ScriptWrappable::setTypeInfoInObject(object, &V8TestTypedefs::info);
    else
        ASSERT_NOT_REACHED();
}

} // namespace WebCore

// In ScriptWrappable::init, the use of a local function declaration has an issue on Windows:
// the local declaration does not pick up the surrounding namespace. Therefore, we provide this function
// in the global namespace.
// (More info on the MSVC bug here: http://connect.microsoft.com/VisualStudio/feedback/details/664619/the-namespace-of-local-function-declarations-in-c)
void webCoreInitializeScriptWrappableForInterface(WebCore::TestTypedefs* object)
{
    WebCore::initializeScriptWrappableForInterface(object);
}

namespace WebCore {
WrapperTypeInfo V8TestTypedefs::info = { V8TestTypedefs::GetTemplate, V8TestTypedefs::derefObject, 0, 0, 0, V8TestTypedefs::installPerContextPrototypeProperties, 0, WrapperTypeObjectPrototype };

namespace TestTypedefsV8Internal {

template <typename T> void V8_USE(T) { }

static void unsignedLongLongAttrAttributeGetter(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TestTypedefs* imp = V8TestTypedefs::toNative(info.Holder());
    v8SetReturnValue(info, static_cast<double>(imp->unsignedLongLongAttr()));
}

static void unsignedLongLongAttrAttributeGetterCallback(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMGetter");
    TestTypedefsV8Internal::unsignedLongLongAttrAttributeGetter(name, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void unsignedLongLongAttrAttributeSetter(v8::Local<v8::String> name, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
    TestTypedefs* imp = V8TestTypedefs::toNative(info.Holder());
    V8TRYCATCH_VOID(unsigned long long, v, toUInt64(value));
    imp->setUnsignedLongLongAttr(v);
}

static void unsignedLongLongAttrAttributeSetterCallback(v8::Local<v8::String> name, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMSetter");
    TestTypedefsV8Internal::unsignedLongLongAttrAttributeSetter(name, value, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void immutableSerializedScriptValueAttributeGetter(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TestTypedefs* imp = V8TestTypedefs::toNative(info.Holder());
    v8SetReturnValue(info, imp->immutableSerializedScriptValue() ? imp->immutableSerializedScriptValue()->deserialize() : v8::Handle<v8::Value>(v8::Null(info.GetIsolate())));
}

static void immutableSerializedScriptValueAttributeGetterCallback(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMGetter");
    TestTypedefsV8Internal::immutableSerializedScriptValueAttributeGetter(name, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void immutableSerializedScriptValueAttributeSetter(v8::Local<v8::String> name, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
    TestTypedefs* imp = V8TestTypedefs::toNative(info.Holder());
    V8TRYCATCH_VOID(RefPtr<SerializedScriptValue>, v, SerializedScriptValue::create(value, info.GetIsolate()));
    imp->setImmutableSerializedScriptValue(WTF::getPtr(v));
}

static void immutableSerializedScriptValueAttributeSetterCallback(v8::Local<v8::String> name, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMSetter");
    TestTypedefsV8Internal::immutableSerializedScriptValueAttributeSetter(name, value, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void attrWithGetterExceptionAttributeGetter(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TestTypedefs* imp = V8TestTypedefs::toNative(info.Holder());
    ExceptionState es(info.GetIsolate());
    int value = imp->attrWithGetterException(es);
    if (UNLIKELY(es.throwIfNeeded()))
        return;
    v8SetReturnValueInt(info, value);
}

static void attrWithGetterExceptionAttributeGetterCallback(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMGetter");
    TestTypedefsV8Internal::attrWithGetterExceptionAttributeGetter(name, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void attrWithGetterExceptionAttributeSetter(v8::Local<v8::String> name, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
    TestTypedefs* imp = V8TestTypedefs::toNative(info.Holder());
    V8TRYCATCH_VOID(int, v, toInt32(value));
    imp->setAttrWithGetterException(v);
}

static void attrWithGetterExceptionAttributeSetterCallback(v8::Local<v8::String> name, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMSetter");
    TestTypedefsV8Internal::attrWithGetterExceptionAttributeSetter(name, value, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void attrWithSetterExceptionAttributeGetter(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TestTypedefs* imp = V8TestTypedefs::toNative(info.Holder());
    v8SetReturnValueInt(info, imp->attrWithSetterException());
}

static void attrWithSetterExceptionAttributeGetterCallback(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMGetter");
    TestTypedefsV8Internal::attrWithSetterExceptionAttributeGetter(name, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void attrWithSetterExceptionAttributeSetter(v8::Local<v8::String> name, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
    TestTypedefs* imp = V8TestTypedefs::toNative(info.Holder());
    V8TRYCATCH_VOID(int, v, toInt32(value));
    ExceptionState es(info.GetIsolate());
    imp->setAttrWithSetterException(v, es);
    es.throwIfNeeded();
}

static void attrWithSetterExceptionAttributeSetterCallback(v8::Local<v8::String> name, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMSetter");
    TestTypedefsV8Internal::attrWithSetterExceptionAttributeSetter(name, value, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void stringAttrWithGetterExceptionAttributeGetter(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TestTypedefs* imp = V8TestTypedefs::toNative(info.Holder());
    ExceptionState es(info.GetIsolate());
    String value = imp->stringAttrWithGetterException(es);
    if (UNLIKELY(es.throwIfNeeded()))
        return;
    v8SetReturnValueString(info, value, info.GetIsolate());
}

static void stringAttrWithGetterExceptionAttributeGetterCallback(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMGetter");
    TestTypedefsV8Internal::stringAttrWithGetterExceptionAttributeGetter(name, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void stringAttrWithGetterExceptionAttributeSetter(v8::Local<v8::String> name, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
    TestTypedefs* imp = V8TestTypedefs::toNative(info.Holder());
    V8TRYCATCH_FOR_V8STRINGRESOURCE_VOID(V8StringResource<>, v, value);
    imp->setStringAttrWithGetterException(v);
}

static void stringAttrWithGetterExceptionAttributeSetterCallback(v8::Local<v8::String> name, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMSetter");
    TestTypedefsV8Internal::stringAttrWithGetterExceptionAttributeSetter(name, value, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void stringAttrWithSetterExceptionAttributeGetter(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TestTypedefs* imp = V8TestTypedefs::toNative(info.Holder());
    v8SetReturnValueString(info, imp->stringAttrWithSetterException(), info.GetIsolate());
}

static void stringAttrWithSetterExceptionAttributeGetterCallback(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMGetter");
    TestTypedefsV8Internal::stringAttrWithSetterExceptionAttributeGetter(name, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void stringAttrWithSetterExceptionAttributeSetter(v8::Local<v8::String> name, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
    TestTypedefs* imp = V8TestTypedefs::toNative(info.Holder());
    V8TRYCATCH_FOR_V8STRINGRESOURCE_VOID(V8StringResource<>, v, value);
    ExceptionState es(info.GetIsolate());
    imp->setStringAttrWithSetterException(v, es);
    es.throwIfNeeded();
}

static void stringAttrWithSetterExceptionAttributeSetterCallback(v8::Local<v8::String> name, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMSetter");
    TestTypedefsV8Internal::stringAttrWithSetterExceptionAttributeSetter(name, value, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void TestTypedefsConstructorGetter(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    v8::Handle<v8::Value> data = info.Data();
    ASSERT(data->IsExternal());
    V8PerContextData* perContextData = V8PerContextData::from(info.Holder()->CreationContext());
    if (!perContextData)
        return;
    v8SetReturnValue(info, perContextData->constructorForType(WrapperTypeInfo::unwrap(data)));
}
static void TestTypedefsReplaceableAttributeSetter(v8::Local<v8::String> name, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
    info.This()->ForceSet(name, value);
}

static void TestTypedefsReplaceableAttributeSetterCallback(v8::Local<v8::String> name, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
    TestTypedefsV8Internal::TestTypedefsReplaceableAttributeSetter(name, value, info);
}

static void funcMethod(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    TestTypedefs* imp = V8TestTypedefs::toNative(args.Holder());
    if (UNLIKELY(args.Length() <= 0)) {
        imp->func();

        return;
    }
    V8TRYCATCH_VOID(Vector<int>, x, toNativeArray<int>(args[0], args.GetIsolate()));
    imp->func(x);

    return;
}

static void funcMethodCallback(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMMethod");
    TestTypedefsV8Internal::funcMethod(args);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void setShadowMethod(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if (UNLIKELY(args.Length() < 3)) {
        throwTypeError(ExceptionMessages::failedToExecute("setShadow", "TestTypedefs", ExceptionMessages::notEnoughArguments(3, args.Length())), args.GetIsolate());
        return;
    }
    TestTypedefs* imp = V8TestTypedefs::toNative(args.Holder());
    V8TRYCATCH_VOID(float, width, static_cast<float>(args[0]->NumberValue()));
    V8TRYCATCH_VOID(float, height, static_cast<float>(args[1]->NumberValue()));
    V8TRYCATCH_VOID(float, blur, static_cast<float>(args[2]->NumberValue()));
    if (UNLIKELY(args.Length() <= 3)) {
        imp->setShadow(width, height, blur);

        return;
    }
    V8TRYCATCH_FOR_V8STRINGRESOURCE_VOID(V8StringResource<>, color, args[3]);
    if (UNLIKELY(args.Length() <= 4)) {
        imp->setShadow(width, height, blur, color);

        return;
    }
    V8TRYCATCH_VOID(float, alpha, static_cast<float>(args[4]->NumberValue()));
    imp->setShadow(width, height, blur, color, alpha);

    return;
}

static void setShadowMethodCallback(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMMethod");
    TestTypedefsV8Internal::setShadowMethod(args);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void methodWithSequenceArgMethod(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if (UNLIKELY(args.Length() < 1)) {
        throwTypeError(ExceptionMessages::failedToExecute("methodWithSequenceArg", "TestTypedefs", ExceptionMessages::notEnoughArguments(1, args.Length())), args.GetIsolate());
        return;
    }
    TestTypedefs* imp = V8TestTypedefs::toNative(args.Holder());
    V8TRYCATCH_VOID(Vector<RefPtr<SerializedScriptValue> >, sequenceArg, (toRefPtrNativeArray<SerializedScriptValue, V8SerializedScriptValue>(args[0], args.GetIsolate())));
    v8SetReturnValue(args, static_cast<double>(imp->methodWithSequenceArg(sequenceArg)));
    return;
}

static void methodWithSequenceArgMethodCallback(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMMethod");
    TestTypedefsV8Internal::methodWithSequenceArgMethod(args);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void nullableArrayArgMethod(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if (UNLIKELY(args.Length() < 1)) {
        throwTypeError(ExceptionMessages::failedToExecute("nullableArrayArg", "TestTypedefs", ExceptionMessages::notEnoughArguments(1, args.Length())), args.GetIsolate());
        return;
    }
    TestTypedefs* imp = V8TestTypedefs::toNative(args.Holder());
    V8TRYCATCH_VOID(Vector<String>, arrayArg, toNativeArray<String>(args[0], args.GetIsolate()));
    imp->nullableArrayArg(arrayArg);

    return;
}

static void nullableArrayArgMethodCallback(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMMethod");
    TestTypedefsV8Internal::nullableArrayArgMethod(args);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void funcWithClampMethod(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if (UNLIKELY(args.Length() < 1)) {
        throwTypeError(ExceptionMessages::failedToExecute("funcWithClamp", "TestTypedefs", ExceptionMessages::notEnoughArguments(1, args.Length())), args.GetIsolate());
        return;
    }
    TestTypedefs* imp = V8TestTypedefs::toNative(args.Holder());
    unsigned long long arg1 = 0;
    V8TRYCATCH_VOID(double, arg1NativeValue, args[0]->NumberValue());
    if (!std::isnan(arg1NativeValue))
        arg1 = clampTo<unsigned long long>(arg1NativeValue);
    if (UNLIKELY(args.Length() <= 1)) {
        imp->funcWithClamp(arg1);

        return;
    }
    unsigned long long arg2 = 0;
    V8TRYCATCH_VOID(double, arg2NativeValue, args[1]->NumberValue());
    if (!std::isnan(arg2NativeValue))
        arg2 = clampTo<unsigned long long>(arg2NativeValue);
    imp->funcWithClamp(arg1, arg2);

    return;
}

static void funcWithClampMethodCallback(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMMethod");
    TestTypedefsV8Internal::funcWithClampMethod(args);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void immutablePointFunctionMethod(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    TestTypedefs* imp = V8TestTypedefs::toNative(args.Holder());
    v8SetReturnValue(args, WTF::getPtr(SVGPropertyTearOff<SVGPoint>::create(imp->immutablePointFunction())), args.Holder());
    return;
}

static void immutablePointFunctionMethodCallback(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMMethod");
    TestTypedefsV8Internal::immutablePointFunctionMethod(args);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void stringArrayFunctionMethod(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if (UNLIKELY(args.Length() < 1)) {
        throwTypeError(ExceptionMessages::failedToExecute("stringArrayFunction", "TestTypedefs", ExceptionMessages::notEnoughArguments(1, args.Length())), args.GetIsolate());
        return;
    }
    TestTypedefs* imp = V8TestTypedefs::toNative(args.Holder());
    ExceptionState es(args.GetIsolate());
    V8TRYCATCH_VOID(Vector<String>, values, toNativeArray<String>(args[0], args.GetIsolate()));
    Vector<String> result = imp->stringArrayFunction(values, es);
    if (es.throwIfNeeded())
        return;
    v8SetReturnValue(args, v8Array(result, args.GetIsolate()));
    return;
}

static void stringArrayFunctionMethodCallback(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMMethod");
    TestTypedefsV8Internal::stringArrayFunctionMethod(args);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void stringArrayFunction2Method(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if (UNLIKELY(args.Length() < 1)) {
        throwTypeError(ExceptionMessages::failedToExecute("stringArrayFunction2", "TestTypedefs", ExceptionMessages::notEnoughArguments(1, args.Length())), args.GetIsolate());
        return;
    }
    TestTypedefs* imp = V8TestTypedefs::toNative(args.Holder());
    ExceptionState es(args.GetIsolate());
    V8TRYCATCH_VOID(Vector<String>, values, toNativeArray<String>(args[0], args.GetIsolate()));
    Vector<String> result = imp->stringArrayFunction2(values, es);
    if (es.throwIfNeeded())
        return;
    v8SetReturnValue(args, v8Array(result, args.GetIsolate()));
    return;
}

static void stringArrayFunction2MethodCallback(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMMethod");
    TestTypedefsV8Internal::stringArrayFunction2Method(args);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void methodWithExceptionMethod(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    TestTypedefs* imp = V8TestTypedefs::toNative(args.Holder());
    ExceptionState es(args.GetIsolate());
    imp->methodWithException(es);
    if (es.throwIfNeeded())
        return;

    return;
}

static void methodWithExceptionMethodCallback(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMMethod");
    TestTypedefsV8Internal::methodWithExceptionMethod(args);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void constructor(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if (UNLIKELY(args.Length() < 2)) {
        throwTypeError(ExceptionMessages::failedToExecute("Constructor", "TestTypedefs", ExceptionMessages::notEnoughArguments(2, args.Length())), args.GetIsolate());
        return;
    }
    V8TRYCATCH_FOR_V8STRINGRESOURCE_VOID(V8StringResource<>, hello, args[0]);
    if (args.Length() <= 1 || !args[1]->IsFunction()) {
        throwTypeError(args.GetIsolate());
        return;
    }
    RefPtr<TestCallback> testCallback = V8TestCallback::create(args[1], getScriptExecutionContext());

    RefPtr<TestTypedefs> impl = TestTypedefs::create(hello, testCallback);
    v8::Handle<v8::Object> wrapper = args.Holder();

    V8DOMWrapper::associateObjectWithWrapper<V8TestTypedefs>(impl.release(), &V8TestTypedefs::info, wrapper, args.GetIsolate(), WrapperConfiguration::Dependent);
    args.GetReturnValue().Set(wrapper);
}

} // namespace TestTypedefsV8Internal

static const V8DOMConfiguration::AttributeConfiguration V8TestTypedefsAttributes[] = {
    {"unsignedLongLongAttr", TestTypedefsV8Internal::unsignedLongLongAttrAttributeGetterCallback, TestTypedefsV8Internal::unsignedLongLongAttrAttributeSetterCallback, 0, 0, 0, static_cast<v8::AccessControl>(v8::DEFAULT), static_cast<v8::PropertyAttribute>(v8::None), 0 /* on instance */},
    {"immutableSerializedScriptValue", TestTypedefsV8Internal::immutableSerializedScriptValueAttributeGetterCallback, TestTypedefsV8Internal::immutableSerializedScriptValueAttributeSetterCallback, 0, 0, 0, static_cast<v8::AccessControl>(v8::DEFAULT), static_cast<v8::PropertyAttribute>(v8::None), 0 /* on instance */},
    {"attrWithGetterException", TestTypedefsV8Internal::attrWithGetterExceptionAttributeGetterCallback, TestTypedefsV8Internal::attrWithGetterExceptionAttributeSetterCallback, 0, 0, 0, static_cast<v8::AccessControl>(v8::DEFAULT), static_cast<v8::PropertyAttribute>(v8::None), 0 /* on instance */},
    {"attrWithSetterException", TestTypedefsV8Internal::attrWithSetterExceptionAttributeGetterCallback, TestTypedefsV8Internal::attrWithSetterExceptionAttributeSetterCallback, 0, 0, 0, static_cast<v8::AccessControl>(v8::DEFAULT), static_cast<v8::PropertyAttribute>(v8::None), 0 /* on instance */},
    {"stringAttrWithGetterException", TestTypedefsV8Internal::stringAttrWithGetterExceptionAttributeGetterCallback, TestTypedefsV8Internal::stringAttrWithGetterExceptionAttributeSetterCallback, 0, 0, 0, static_cast<v8::AccessControl>(v8::DEFAULT), static_cast<v8::PropertyAttribute>(v8::None), 0 /* on instance */},
    {"stringAttrWithSetterException", TestTypedefsV8Internal::stringAttrWithSetterExceptionAttributeGetterCallback, TestTypedefsV8Internal::stringAttrWithSetterExceptionAttributeSetterCallback, 0, 0, 0, static_cast<v8::AccessControl>(v8::DEFAULT), static_cast<v8::PropertyAttribute>(v8::None), 0 /* on instance */},
};

static const V8DOMConfiguration::MethodConfiguration V8TestTypedefsMethods[] = {
    {"func", TestTypedefsV8Internal::funcMethodCallback, 0, 0},
    {"setShadow", TestTypedefsV8Internal::setShadowMethodCallback, 0, 3},
    {"methodWithSequenceArg", TestTypedefsV8Internal::methodWithSequenceArgMethodCallback, 0, 1},
    {"nullableArrayArg", TestTypedefsV8Internal::nullableArrayArgMethodCallback, 0, 1},
    {"funcWithClamp", TestTypedefsV8Internal::funcWithClampMethodCallback, 0, 1},
    {"immutablePointFunction", TestTypedefsV8Internal::immutablePointFunctionMethodCallback, 0, 0},
    {"stringArrayFunction", TestTypedefsV8Internal::stringArrayFunctionMethodCallback, 0, 1},
    {"stringArrayFunction2", TestTypedefsV8Internal::stringArrayFunction2MethodCallback, 0, 1},
    {"methodWithException", TestTypedefsV8Internal::methodWithExceptionMethodCallback, 0, 0},
};

void V8TestTypedefs::constructorCallback(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    TRACE_EVENT_SCOPED_SAMPLING_STATE("Blink", "DOMConstructor");
    if (!args.IsConstructCall()) {
        throwTypeError(ExceptionMessages::failedToConstruct("TestTypedefs", "Please use the 'new' operator, this DOM object constructor cannot be called as a function."), args.GetIsolate());
        return;
    }

    if (ConstructorMode::current() == ConstructorMode::WrapExistingObject) {
        args.GetReturnValue().Set(args.Holder());
        return;
    }

    TestTypedefsV8Internal::constructor(args);
}

static v8::Handle<v8::FunctionTemplate> ConfigureV8TestTypedefsTemplate(v8::Handle<v8::FunctionTemplate> desc, v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    desc->ReadOnlyPrototype();

    v8::Local<v8::Signature> defaultSignature;
    defaultSignature = V8DOMConfiguration::installDOMClassTemplate(desc, "TestTypedefs", v8::Local<v8::FunctionTemplate>(), V8TestTypedefs::internalFieldCount,
        V8TestTypedefsAttributes, WTF_ARRAY_LENGTH(V8TestTypedefsAttributes),
        V8TestTypedefsMethods, WTF_ARRAY_LENGTH(V8TestTypedefsMethods), isolate, currentWorldType);
    UNUSED_PARAM(defaultSignature);
    desc->SetCallHandler(V8TestTypedefs::constructorCallback);
    desc->SetLength(2);
    v8::Local<v8::ObjectTemplate> instance = desc->InstanceTemplate();
    v8::Local<v8::ObjectTemplate> proto = desc->PrototypeTemplate();
    UNUSED_PARAM(instance);
    UNUSED_PARAM(proto);
    desc->SetNativeDataProperty(v8::String::NewSymbol("TestSubObj"), TestTypedefsV8Internal::TestTypedefsConstructorGetter, 0, v8::External::New(&V8TestSubObj::info), static_cast<v8::PropertyAttribute>(v8::None | v8::DontEnum), v8::Handle<v8::AccessorSignature>(), static_cast<v8::AccessControl>(v8::DEFAULT));

    // Custom toString template
    desc->Set(v8::String::NewSymbol("toString"), V8PerIsolateData::current()->toStringTemplate());
    return desc;
}

v8::Handle<v8::FunctionTemplate> V8TestTypedefs::GetTemplate(v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    V8PerIsolateData* data = V8PerIsolateData::from(isolate);
    V8PerIsolateData::TemplateMap::iterator result = data->templateMap(currentWorldType).find(&info);
    if (result != data->templateMap(currentWorldType).end())
        return result->value.newLocal(isolate);

    TRACE_EVENT_SCOPED_SAMPLING_STATE("Blink", "BuildDOMTemplate");
    v8::HandleScope handleScope(isolate);
    v8::Handle<v8::FunctionTemplate> templ =
        ConfigureV8TestTypedefsTemplate(data->rawTemplate(&info, currentWorldType), isolate, currentWorldType);
    data->templateMap(currentWorldType).add(&info, UnsafePersistent<v8::FunctionTemplate>(isolate, templ));
    return handleScope.Close(templ);
}

bool V8TestTypedefs::HasInstance(v8::Handle<v8::Value> value, v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    return V8PerIsolateData::from(isolate)->hasInstance(&info, value, currentWorldType);
}

bool V8TestTypedefs::HasInstanceInAnyWorld(v8::Handle<v8::Value> value, v8::Isolate* isolate)
{
    return V8PerIsolateData::from(isolate)->hasInstance(&info, value, MainWorld)
        || V8PerIsolateData::from(isolate)->hasInstance(&info, value, IsolatedWorld)
        || V8PerIsolateData::from(isolate)->hasInstance(&info, value, WorkerWorld);
}

v8::Handle<v8::Object> V8TestTypedefs::createWrapper(PassRefPtr<TestTypedefs> impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    ASSERT(impl.get());
    ASSERT(!DOMDataStore::containsWrapper<V8TestTypedefs>(impl.get(), isolate));
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
    V8DOMWrapper::associateObjectWithWrapper<V8TestTypedefs>(impl, &info, wrapper, isolate, WrapperConfiguration::Independent);
    return wrapper;
}

void V8TestTypedefs::derefObject(void* object)
{
    fromInternalPointer(object)->deref();
}

} // namespace WebCore
