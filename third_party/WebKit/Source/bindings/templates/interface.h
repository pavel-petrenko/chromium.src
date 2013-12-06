{# http://www.chromium.org/blink/coding-style#TOC-License #}
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

{# FIXME: Rename to Python when switch #}
// This file has been auto-generated by code_generator_v8.pm. DO NOT MODIFY!

#ifndef {{v8_class}}_h
#define {{v8_class}}_h

{% filter conditional(conditional_string) %}
{% for filename in header_includes %}
#include "{{filename}}"
{% endfor %}

namespace WebCore {

class {{v8_class}} {
public:
    static bool hasInstance(v8::Handle<v8::Value>, v8::Isolate*, WrapperWorldType);
    static bool hasInstanceInAnyWorld(v8::Handle<v8::Value>, v8::Isolate*);
    static v8::Handle<v8::FunctionTemplate> GetTemplate(v8::Isolate*, WrapperWorldType);
    static {{cpp_class}}* toNative(v8::Handle<v8::Object> object)
    {
        return fromInternalPointer(object->GetAlignedPointerFromInternalField(v8DOMWrapperObjectIndex));
    }
    static void derefObject(void*);
    static const WrapperTypeInfo wrapperTypeInfo;
    {% if has_visit_dom_wrapper %}
    static void visitDOMWrapper(void*, const v8::Persistent<v8::Object>&, v8::Isolate*);
    {% endif %}
    {% if is_active_dom_object %}
    static ActiveDOMObject* toActiveDOMObject(v8::Handle<v8::Object>);
    {% endif %}
    {% for method in methods if method.is_custom %}
    {% filter conditional(method.conditional_string) %}
    static void {{method.name}}MethodCustom(const v8::FunctionCallbackInfo<v8::Value>&);
    {% endfilter %}
    {% endfor %}
    {% if has_constructor %}
    static void constructorCallback(const v8::FunctionCallbackInfo<v8::Value>&);
    {% endif %}
    {% for attribute in attributes %}
    {% if attribute.has_custom_getter %}{# FIXME: and not attribute.implemented_by #}
    {% filter conditional(attribute.conditional_string) %}
    static void {{attribute.name}}AttributeGetterCustom(const v8::PropertyCallbackInfo<v8::Value>&);
    {% endfilter %}
    {% endif %}
    {% if attribute.has_custom_setter %}{# FIXME: and not attribute.implemented_by #}
    {% filter conditional(attribute.conditional_string) %}
    static void {{attribute.name}}AttributeSetterCustom(v8::Local<v8::Value>, const v8::PropertyCallbackInfo<void>&);
    {% endfilter %}
    {% endif %}
    {% endfor %}
    {% if has_custom_legacy_call %}
    static void legacyCallCustom(const v8::FunctionCallbackInfo<v8::Value>&);
    {% endif %}
    static const int internalFieldCount = v8DefaultWrapperInternalFieldCount + 0;
    static inline void* toInternalPointer({{cpp_class}}* impl)
    {
        return impl;
    }

    static inline {{cpp_class}}* fromInternalPointer(void* object)
    {
        return static_cast<{{cpp_class}}*>(object);
    }
    static void installPerContextEnabledProperties(v8::Handle<v8::Object>, {{cpp_class}}*, v8::Isolate*){% if has_per_context_enabled_attributes %};
    {% else %} { }
    {% endif %}
    static void installPerContextEnabledMethods(v8::Handle<v8::Object>, v8::Isolate*){% if has_per_context_enabled_attributes %};
    {% else %} { }
    {% endif %}

private:
    {% if not has_custom_to_v8 %}
    friend v8::Handle<v8::Object> wrap({{cpp_class}}*, v8::Handle<v8::Object> creationContext, v8::Isolate*);
    static v8::Handle<v8::Object> createWrapper(PassRefPtr<{{cpp_class}}>, v8::Handle<v8::Object> creationContext, v8::Isolate*);
    {% endif %}
};

template<>
class WrapperTypeTraits<{{cpp_class}} > {
public:
    static const WrapperTypeInfo* wrapperTypeInfo() { return &{{v8_class}}::wrapperTypeInfo; }
};

{% if has_custom_to_v8 %}
class {{cpp_class}};
v8::Handle<v8::Value> toV8({{cpp_class}}*, v8::Handle<v8::Object> creationContext, v8::Isolate*);

template<class CallbackInfo>
inline void v8SetReturnValue(const CallbackInfo& callbackInfo, {{cpp_class}}* impl)
{
    v8SetReturnValue(callbackInfo, toV8(impl, callbackInfo.Holder(), callbackInfo.GetIsolate()));
}

template<class CallbackInfo>
inline void v8SetReturnValueForMainWorld(const CallbackInfo& callbackInfo, {{cpp_class}}* impl)
{
     v8SetReturnValue(callbackInfo, toV8(impl, callbackInfo.Holder(), callbackInfo.GetIsolate()));
}

template<class CallbackInfo, class Wrappable>
inline void v8SetReturnValueFast(const CallbackInfo& callbackInfo, {{cpp_class}}* impl, Wrappable*)
{
     v8SetReturnValue(callbackInfo, toV8(impl, callbackInfo.Holder(), callbackInfo.GetIsolate()));
}
{% else %}{# has_custom_to_v8 #}
{% if has_custom_wrap or special_wrap_for %}
v8::Handle<v8::Object> wrap({{cpp_class}}* impl, v8::Handle<v8::Object> creationContext, v8::Isolate*);
{% else %}
inline v8::Handle<v8::Object> wrap({{cpp_class}}* impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    ASSERT(impl);
    ASSERT(!DOMDataStore::containsWrapper<{{v8_class}}>(impl, isolate));
    return {{v8_class}}::createWrapper(impl, creationContext, isolate);
}
{% endif %}

inline v8::Handle<v8::Value> toV8({{cpp_class}}* impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    if (UNLIKELY(!impl))
        return v8::Null(isolate);
    v8::Handle<v8::Value> wrapper = DOMDataStore::getWrapper<{{v8_class}}>(impl, isolate);
    if (!wrapper.IsEmpty())
        return wrapper;
    return wrap(impl, creationContext, isolate);
}

template<typename CallbackInfo>
inline void v8SetReturnValue(const CallbackInfo& callbackInfo, {{cpp_class}}* impl)
{
    if (UNLIKELY(!impl)) {
        v8SetReturnValueNull(callbackInfo);
        return;
    }
    if (DOMDataStore::setReturnValueFromWrapper<{{v8_class}}>(callbackInfo.GetReturnValue(), impl))
        return;
    v8::Handle<v8::Object> wrapper = wrap(impl, callbackInfo.Holder(), callbackInfo.GetIsolate());
    v8SetReturnValue(callbackInfo, wrapper);
}

template<typename CallbackInfo>
inline void v8SetReturnValueForMainWorld(const CallbackInfo& callbackInfo, {{cpp_class}}* impl)
{
    ASSERT(worldType(callbackInfo.GetIsolate()) == MainWorld);
    if (UNLIKELY(!impl)) {
        v8SetReturnValueNull(callbackInfo);
        return;
    }
    if (DOMDataStore::setReturnValueFromWrapperForMainWorld<{{v8_class}}>(callbackInfo.GetReturnValue(), impl))
        return;
    v8::Handle<v8::Value> wrapper = wrap(impl, callbackInfo.Holder(), callbackInfo.GetIsolate());
    v8SetReturnValue(callbackInfo, wrapper);
}

template<class CallbackInfo, class Wrappable>
inline void v8SetReturnValueFast(const CallbackInfo& callbackInfo, {{cpp_class}}* impl, Wrappable* wrappable)
{
    if (UNLIKELY(!impl)) {
        v8SetReturnValueNull(callbackInfo);
        return;
    }
    if (DOMDataStore::setReturnValueFromWrapperFast<{{v8_class}}>(callbackInfo.GetReturnValue(), impl, callbackInfo.Holder(), wrappable))
        return;
    v8::Handle<v8::Object> wrapper = wrap(impl, callbackInfo.Holder(), callbackInfo.GetIsolate());
    v8SetReturnValue(callbackInfo, wrapper);
}
{% endif %}{# has_custom_to_v8 #}

inline v8::Handle<v8::Value> toV8(PassRefPtr<{{cpp_class}} > impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    return toV8(impl.get(), creationContext, isolate);
}

template<class CallbackInfo>
inline void v8SetReturnValue(const CallbackInfo& callbackInfo, PassRefPtr<{{cpp_class}} > impl)
{
    v8SetReturnValue(callbackInfo, impl.get());
}

template<class CallbackInfo>
inline void v8SetReturnValueForMainWorld(const CallbackInfo& callbackInfo, PassRefPtr<{{cpp_class}} > impl)
{
    v8SetReturnValueForMainWorld(callbackInfo, impl.get());
}

template<class CallbackInfo, class Wrappable>
inline void v8SetReturnValueFast(const CallbackInfo& callbackInfo, PassRefPtr<{{cpp_class}} > impl, Wrappable* wrappable)
{
    v8SetReturnValueFast(callbackInfo, impl.get(), wrappable);
}

}
{% endfilter %}
#endif // {{v8_class}}_h
