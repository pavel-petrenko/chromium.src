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
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include <glib-object.h>
#include "config.h"

#include <wtf/GetPtr.h>
#include <wtf/RefPtr.h>
#include "ExceptionCode.h"
#include "TestObj.h"
#include "WebKitDOMBinding.h"
#include "gobject/ConvertToUTF8String.h"
#include "webkit/WebKitDOMSerializedScriptValue.h"
#include "webkit/WebKitDOMSerializedScriptValuePrivate.h"
#include "webkit/WebKitDOMTestObj.h"
#include "webkit/WebKitDOMTestObjPrivate.h"
#include "webkitmarshal.h"
#include "webkitprivate.h"

namespace WebKit {
    
gpointer kit(WebCore::TestObj* obj)
{
    g_return_val_if_fail(obj, 0);

    if (gpointer ret = DOMObjectCache::get(obj))
        return ret;

    return DOMObjectCache::put(obj, WebKit::wrapTestObj(obj));
}
    
} // namespace WebKit //

void
webkit_dom_test_obj_void_method(WebKitDOMTestObj* self)
{
    g_return_if_fail (self);
    WebCore::TestObj * item = WebKit::core(self);
    item->voidMethod();
}

void
webkit_dom_test_obj_void_method_with_args(WebKitDOMTestObj* self, glong int_arg, gchar*  str_arg, WebKitDOMTestObj*  obj_arg)
{
    g_return_if_fail (self);
    WebCore::TestObj * item = WebKit::core(self);
    g_return_if_fail (str_arg);
    g_return_if_fail (obj_arg);
    WebCore::String _g_str_arg = WebCore::String::fromUTF8(str_arg);
    WebCore::TestObj * _g_obj_arg = WebKit::core(obj_arg);
    g_return_if_fail (_g_obj_arg);
    item->voidMethodWithArgs(int_arg, _g_str_arg, _g_obj_arg);
}

glong
webkit_dom_test_obj_int_method(WebKitDOMTestObj* self)
{
    g_return_val_if_fail (self, 0);
    WebCore::TestObj * item = WebKit::core(self);
    glong res = item->intMethod();
    return res;
}

glong
webkit_dom_test_obj_int_method_with_args(WebKitDOMTestObj* self, glong int_arg, gchar*  str_arg, WebKitDOMTestObj*  obj_arg)
{
    g_return_val_if_fail (self, 0);
    WebCore::TestObj * item = WebKit::core(self);
    g_return_val_if_fail (str_arg, 0);
    g_return_val_if_fail (obj_arg, 0);
    WebCore::String _g_str_arg = WebCore::String::fromUTF8(str_arg);
    WebCore::TestObj * _g_obj_arg = WebKit::core(obj_arg);
    g_return_val_if_fail (_g_obj_arg, 0);
    glong res = item->intMethodWithArgs(int_arg, _g_str_arg, _g_obj_arg);
    return res;
}

WebKitDOMTestObj* 
webkit_dom_test_obj_obj_method(WebKitDOMTestObj* self)
{
    g_return_val_if_fail (self, 0);
    WebCore::TestObj * item = WebKit::core(self);
    PassRefPtr<WebCore::TestObj> g_res = WTF::getPtr(item->objMethod());
    WebKitDOMTestObj*  res = static_cast<WebKitDOMTestObj* >(WebKit::kit(g_res.get()));
    return res;
}

WebKitDOMTestObj* 
webkit_dom_test_obj_obj_method_with_args(WebKitDOMTestObj* self, glong int_arg, gchar*  str_arg, WebKitDOMTestObj*  obj_arg)
{
    g_return_val_if_fail (self, 0);
    WebCore::TestObj * item = WebKit::core(self);
    g_return_val_if_fail (str_arg, 0);
    g_return_val_if_fail (obj_arg, 0);
    WebCore::String _g_str_arg = WebCore::String::fromUTF8(str_arg);
    WebCore::TestObj * _g_obj_arg = WebKit::core(obj_arg);
    g_return_val_if_fail (_g_obj_arg, 0);
    PassRefPtr<WebCore::TestObj> g_res = WTF::getPtr(item->objMethodWithArgs(int_arg, _g_str_arg, _g_obj_arg));
    WebKitDOMTestObj*  res = static_cast<WebKitDOMTestObj* >(WebKit::kit(g_res.get()));
    return res;
}

WebKitDOMTestObj* 
webkit_dom_test_obj_method_that_requires_all_args(WebKitDOMTestObj* self, gchar*  str_arg, WebKitDOMTestObj*  obj_arg)
{
    g_return_val_if_fail (self, 0);
    WebCore::TestObj * item = WebKit::core(self);
    g_return_val_if_fail (str_arg, 0);
    g_return_val_if_fail (obj_arg, 0);
    WebCore::String _g_str_arg = WebCore::String::fromUTF8(str_arg);
    WebCore::TestObj * _g_obj_arg = WebKit::core(obj_arg);
    g_return_val_if_fail (_g_obj_arg, 0);
    PassRefPtr<WebCore::TestObj> g_res = WTF::getPtr(item->methodThatRequiresAllArgs(_g_str_arg, _g_obj_arg));
    WebKitDOMTestObj*  res = static_cast<WebKitDOMTestObj* >(WebKit::kit(g_res.get()));
    return res;
}

WebKitDOMTestObj* 
webkit_dom_test_obj_method_that_requires_all_args_and_throws(WebKitDOMTestObj* self, gchar*  str_arg, WebKitDOMTestObj*  obj_arg, GError **error)
{
    g_return_val_if_fail (self, 0);
    WebCore::TestObj * item = WebKit::core(self);
    g_return_val_if_fail (str_arg, 0);
    g_return_val_if_fail (obj_arg, 0);
    WebCore::String _g_str_arg = WebCore::String::fromUTF8(str_arg);
    WebCore::TestObj * _g_obj_arg = WebKit::core(obj_arg);
    g_return_val_if_fail (_g_obj_arg, 0);
    WebCore::ExceptionCode ec = 0;
    PassRefPtr<WebCore::TestObj> g_res = WTF::getPtr(item->methodThatRequiresAllArgsAndThrows(_g_str_arg, _g_obj_arg, ec));
    if (ec) {
        WebCore::ExceptionCodeDescription ecdesc;
        WebCore::getExceptionCodeDescription(ec, ecdesc);
        g_set_error_literal(error, g_quark_from_string("WEBKIT_DOM"), ecdesc.code, ecdesc.name);
    }
    WebKitDOMTestObj*  res = static_cast<WebKitDOMTestObj* >(WebKit::kit(g_res.get()));
    return res;
}

void
webkit_dom_test_obj_serialized_value(WebKitDOMTestObj* self, WebKitDOMSerializedScriptValue*  serialized_arg)
{
    g_return_if_fail (self);
    WebCore::TestObj * item = WebKit::core(self);
    g_return_if_fail (serialized_arg);
    WebCore::SerializedScriptValue * _g_serialized_arg = WebKit::core(serialized_arg);
    g_return_if_fail (_g_serialized_arg);
    item->serializedValue(_g_serialized_arg);
}

void
webkit_dom_test_obj_method_with_exception(WebKitDOMTestObj* self, GError **error)
{
    g_return_if_fail (self);
    WebCore::TestObj * item = WebKit::core(self);
    WebCore::ExceptionCode ec = 0;
    item->methodWithException(ec);
    if (ec) {
        WebCore::ExceptionCodeDescription ecdesc;
        WebCore::getExceptionCodeDescription(ec, ecdesc);
        g_set_error_literal(error, g_quark_from_string("WEBKIT_DOM"), ecdesc.code, ecdesc.name);
    }
}


/* TODO: event function webkit_dom_test_obj_add_event_listener */


/* TODO: event function webkit_dom_test_obj_remove_event_listener */

void
webkit_dom_test_obj_with_dynamic_frame(WebKitDOMTestObj* self)
{
    g_return_if_fail (self);
    WebCore::TestObj * item = WebKit::core(self);
    item->withDynamicFrame();
}

void
webkit_dom_test_obj_with_dynamic_frame_and_arg(WebKitDOMTestObj* self, glong int_arg)
{
    g_return_if_fail (self);
    WebCore::TestObj * item = WebKit::core(self);
    item->withDynamicFrameAndArg(int_arg);
}

void
webkit_dom_test_obj_with_dynamic_frame_and_optional_arg(WebKitDOMTestObj* self, glong int_arg, glong optional_arg)
{
    g_return_if_fail (self);
    WebCore::TestObj * item = WebKit::core(self);
    item->withDynamicFrameAndOptionalArg(int_arg, optional_arg);
}

void
webkit_dom_test_obj_with_dynamic_frame_and_user_gesture(WebKitDOMTestObj* self, glong int_arg)
{
    g_return_if_fail (self);
    WebCore::TestObj * item = WebKit::core(self);
    item->withDynamicFrameAndUserGesture(int_arg);
}

void
webkit_dom_test_obj_with_dynamic_frame_and_user_gesture_asad(WebKitDOMTestObj* self, glong int_arg, glong optional_arg)
{
    g_return_if_fail (self);
    WebCore::TestObj * item = WebKit::core(self);
    item->withDynamicFrameAndUserGestureASAD(int_arg, optional_arg);
}

void
webkit_dom_test_obj_with_script_state_void(WebKitDOMTestObj* self)
{
    g_return_if_fail (self);
    WebCore::TestObj * item = WebKit::core(self);
    item->withScriptStateVoid();
}

WebKitDOMTestObj* 
webkit_dom_test_obj_with_script_state_obj(WebKitDOMTestObj* self)
{
    g_return_val_if_fail (self, 0);
    WebCore::TestObj * item = WebKit::core(self);
    PassRefPtr<WebCore::TestObj> g_res = WTF::getPtr(item->withScriptStateObj());
    WebKitDOMTestObj*  res = static_cast<WebKitDOMTestObj* >(WebKit::kit(g_res.get()));
    return res;
}

void
webkit_dom_test_obj_with_script_state_void_exception(WebKitDOMTestObj* self, GError **error)
{
    g_return_if_fail (self);
    WebCore::TestObj * item = WebKit::core(self);
    WebCore::ExceptionCode ec = 0;
    item->withScriptStateVoidException(ec);
    if (ec) {
        WebCore::ExceptionCodeDescription ecdesc;
        WebCore::getExceptionCodeDescription(ec, ecdesc);
        g_set_error_literal(error, g_quark_from_string("WEBKIT_DOM"), ecdesc.code, ecdesc.name);
    }
}

WebKitDOMTestObj* 
webkit_dom_test_obj_with_script_state_obj_exception(WebKitDOMTestObj* self, GError **error)
{
    g_return_val_if_fail (self, 0);
    WebCore::TestObj * item = WebKit::core(self);
    WebCore::ExceptionCode ec = 0;
    PassRefPtr<WebCore::TestObj> g_res = WTF::getPtr(item->withScriptStateObjException(ec));
    if (ec) {
        WebCore::ExceptionCodeDescription ecdesc;
        WebCore::getExceptionCodeDescription(ec, ecdesc);
        g_set_error_literal(error, g_quark_from_string("WEBKIT_DOM"), ecdesc.code, ecdesc.name);
    }
    WebKitDOMTestObj*  res = static_cast<WebKitDOMTestObj* >(WebKit::kit(g_res.get()));
    return res;
}

void
webkit_dom_test_obj_with_script_execution_context(WebKitDOMTestObj* self)
{
    g_return_if_fail (self);
    WebCore::TestObj * item = WebKit::core(self);
    item->withScriptExecutionContext();
}

void
webkit_dom_test_obj_method_with_optional_arg(WebKitDOMTestObj* self, glong opt)
{
    g_return_if_fail (self);
    WebCore::TestObj * item = WebKit::core(self);
    item->methodWithOptionalArg(opt);
}

void
webkit_dom_test_obj_method_with_non_optional_arg_and_optional_arg(WebKitDOMTestObj* self, glong non_opt, glong opt)
{
    g_return_if_fail (self);
    WebCore::TestObj * item = WebKit::core(self);
    item->methodWithNonOptionalArgAndOptionalArg(non_opt, opt);
}

void
webkit_dom_test_obj_method_with_non_optional_arg_and_two_optional_args(WebKitDOMTestObj* self, glong non_opt, glong opt1, glong opt2)
{
    g_return_if_fail (self);
    WebCore::TestObj * item = WebKit::core(self);
    item->methodWithNonOptionalArgAndTwoOptionalArgs(non_opt, opt1, opt2);
}

glong
webkit_dom_test_obj_get_read_only_int_attr(WebKitDOMTestObj* self)
{
    g_return_val_if_fail (self, 0);
    WebCore::TestObj * item = WebKit::core(self);
    glong res = item->readOnlyIntAttr();
    return res;
}

gchar* 
webkit_dom_test_obj_get_read_only_string_attr(WebKitDOMTestObj* self)
{
    g_return_val_if_fail (self, 0);
    WebCore::TestObj * item = WebKit::core(self);
    gchar*  res = convertToUTF8String(item->readOnlyStringAttr());
    return res;
}

WebKitDOMTestObj* 
webkit_dom_test_obj_get_read_only_test_obj_attr(WebKitDOMTestObj* self)
{
    g_return_val_if_fail (self, 0);
    WebCore::TestObj * item = WebKit::core(self);
    PassRefPtr<WebCore::TestObj> g_res = WTF::getPtr(item->readOnlyTestObjAttr());
    WebKitDOMTestObj*  res = static_cast<WebKitDOMTestObj* >(WebKit::kit(g_res.get()));
    return res;
}

glong
webkit_dom_test_obj_get_int_attr(WebKitDOMTestObj* self)
{
    g_return_val_if_fail (self, 0);
    WebCore::TestObj * item = WebKit::core(self);
    glong res = item->intAttr();
    return res;
}

void
webkit_dom_test_obj_set_int_attr(WebKitDOMTestObj* self, glong value)
{
    g_return_if_fail (self);
    WebCore::TestObj * item = WebKit::core(self);
    item->setIntAttr(value);
}

gint64
webkit_dom_test_obj_get_long_long_attr(WebKitDOMTestObj* self)
{
    g_return_val_if_fail (self, 0);
    WebCore::TestObj * item = WebKit::core(self);
    gint64 res = item->longLongAttr();
    return res;
}

void
webkit_dom_test_obj_set_long_long_attr(WebKitDOMTestObj* self, gint64 value)
{
    g_return_if_fail (self);
    WebCore::TestObj * item = WebKit::core(self);
    item->setLongLongAttr(value);
}

guint64
webkit_dom_test_obj_get_unsigned_long_long_attr(WebKitDOMTestObj* self)
{
    g_return_val_if_fail (self, 0);
    WebCore::TestObj * item = WebKit::core(self);
    guint64 res = item->unsignedLongLongAttr();
    return res;
}

void
webkit_dom_test_obj_set_unsigned_long_long_attr(WebKitDOMTestObj* self, guint64 value)
{
    g_return_if_fail (self);
    WebCore::TestObj * item = WebKit::core(self);
    item->setUnsignedLongLongAttr(value);
}

gchar* 
webkit_dom_test_obj_get_string_attr(WebKitDOMTestObj* self)
{
    g_return_val_if_fail (self, 0);
    WebCore::TestObj * item = WebKit::core(self);
    gchar*  res = convertToUTF8String(item->stringAttr());
    return res;
}

void
webkit_dom_test_obj_set_string_attr(WebKitDOMTestObj* self, gchar*  value)
{
    g_return_if_fail (self);
    WebCore::TestObj * item = WebKit::core(self);
    g_return_if_fail (value);
    WebCore::String _g_value = WebCore::String::fromUTF8(value);
    item->setStringAttr(_g_value);
}

WebKitDOMTestObj* 
webkit_dom_test_obj_get_test_obj_attr(WebKitDOMTestObj* self)
{
    g_return_val_if_fail (self, 0);
    WebCore::TestObj * item = WebKit::core(self);
    PassRefPtr<WebCore::TestObj> g_res = WTF::getPtr(item->testObjAttr());
    WebKitDOMTestObj*  res = static_cast<WebKitDOMTestObj* >(WebKit::kit(g_res.get()));
    return res;
}

void
webkit_dom_test_obj_set_test_obj_attr(WebKitDOMTestObj* self, WebKitDOMTestObj*  value)
{
    g_return_if_fail (self);
    WebCore::TestObj * item = WebKit::core(self);
    g_return_if_fail (value);
    WebCore::TestObj * _g_value = WebKit::core(value);
    g_return_if_fail (_g_value);
    item->setTestObjAttr(_g_value);
}

glong
webkit_dom_test_obj_get_attr_with_exception(WebKitDOMTestObj* self)
{
    g_return_val_if_fail (self, 0);
    WebCore::TestObj * item = WebKit::core(self);
    glong res = item->attrWithException();
    return res;
}

void
webkit_dom_test_obj_set_attr_with_exception(WebKitDOMTestObj* self, glong value)
{
    g_return_if_fail (self);
    WebCore::TestObj * item = WebKit::core(self);
    item->setAttrWithException(value);
}

glong
webkit_dom_test_obj_get_attr_with_setter_exception(WebKitDOMTestObj* self, GError **error)
{
    g_return_val_if_fail (self, 0);
    WebCore::TestObj * item = WebKit::core(self);
    WebCore::ExceptionCode ec = 0;
    glong res = item->attrWithSetterException(ec);
    if (ec) {
        WebCore::ExceptionCodeDescription ecdesc;
        WebCore::getExceptionCodeDescription(ec, ecdesc);
        g_set_error_literal(error, g_quark_from_string("WEBKIT_DOM"), ecdesc.code, ecdesc.name);
    }
    return res;
}

void
webkit_dom_test_obj_set_attr_with_setter_exception(WebKitDOMTestObj* self, glong value, GError **error)
{
    g_return_if_fail (self);
    WebCore::TestObj * item = WebKit::core(self);
    WebCore::ExceptionCode ec = 0;
    item->setAttrWithSetterException(value, ec);
    if (ec) {
        WebCore::ExceptionCodeDescription ecdesc;
        WebCore::getExceptionCodeDescription(ec, ecdesc);
        g_set_error_literal(error, g_quark_from_string("WEBKIT_DOM"), ecdesc.code, ecdesc.name);
    }
}

glong
webkit_dom_test_obj_get_attr_with_getter_exception(WebKitDOMTestObj* self)
{
    g_return_val_if_fail (self, 0);
    WebCore::TestObj * item = WebKit::core(self);
    glong res = item->attrWithGetterException();
    return res;
}

void
webkit_dom_test_obj_set_attr_with_getter_exception(WebKitDOMTestObj* self, glong value, GError **error)
{
    g_return_if_fail (self);
    WebCore::TestObj * item = WebKit::core(self);
    WebCore::ExceptionCode ec = 0;
    item->setAttrWithGetterException(value, ec);
    if (ec) {
        WebCore::ExceptionCodeDescription ecdesc;
        WebCore::getExceptionCodeDescription(ec, ecdesc);
        g_set_error_literal(error, g_quark_from_string("WEBKIT_DOM"), ecdesc.code, ecdesc.name);
    }
}

gchar* 
webkit_dom_test_obj_get_script_string_attr(WebKitDOMTestObj* self)
{
    g_return_val_if_fail (self, 0);
    WebCore::TestObj * item = WebKit::core(self);
    gchar*  res = convertToUTF8String(item->scriptStringAttr());
    return res;
}


G_DEFINE_TYPE(WebKitDOMTestObj, webkit_dom_test_obj, WEBKIT_TYPE_DOM_OBJECT)

namespace WebKit {

WebCore::TestObj* core(WebKitDOMTestObj* request)
{
    g_return_val_if_fail(request, 0);

    WebCore::TestObj* coreObject = static_cast<WebCore::TestObj*>(WEBKIT_DOM_OBJECT(request)->coreObject);
    g_return_val_if_fail(coreObject, 0);

    return coreObject;
}

} // namespace WebKit
enum {
    PROP_0,
    PROP_READ_ONLY_INT_ATTR,
    PROP_READ_ONLY_STRING_ATTR,
    PROP_READ_ONLY_TEST_OBJ_ATTR,
    PROP_INT_ATTR,
    PROP_LONG_LONG_ATTR,
    PROP_UNSIGNED_LONG_LONG_ATTR,
    PROP_STRING_ATTR,
    PROP_TEST_OBJ_ATTR,
    PROP_ATTR_WITH_EXCEPTION,
    PROP_ATTR_WITH_SETTER_EXCEPTION,
    PROP_ATTR_WITH_GETTER_EXCEPTION,
    PROP_CUSTOM_ATTR,
    PROP_SCRIPT_STRING_ATTR,
};


static void webkit_dom_test_obj_finalize(GObject* object)
{
    WebKitDOMObject* dom_object = WEBKIT_DOM_OBJECT(object);
    
    if (dom_object->coreObject) {
        WebCore::TestObj* coreObject = static_cast<WebCore::TestObj *>(dom_object->coreObject);

        WebKit::DOMObjectCache::forget(coreObject);
        coreObject->deref();

        dom_object->coreObject = NULL;
    }

    G_OBJECT_CLASS(webkit_dom_test_obj_parent_class)->finalize(object);
}

static void webkit_dom_test_obj_set_property(GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec)
{
    WebKitDOMTestObj* self = WEBKIT_DOM_TEST_OBJ(object);
    WebCore::TestObj* coreSelf = WebKit::core(self);
    switch (prop_id) {
    case PROP_INT_ATTR:
    {
        coreSelf->setIntAttr((g_value_get_long(value)));
        break;
    }
    case PROP_UNSIGNED_LONG_LONG_ATTR:
    {
        coreSelf->setUnsignedLongLongAttr((g_value_get_uint64(value)));
        break;
    }
    case PROP_STRING_ATTR:
    {
        coreSelf->setStringAttr(WebCore::String::fromUTF8(g_value_get_string(value)));
        break;
    }
    case PROP_ATTR_WITH_EXCEPTION:
    {
        coreSelf->setAttrWithException((g_value_get_long(value)));
        break;
    }
    case PROP_ATTR_WITH_SETTER_EXCEPTION:
    {
        WebCore::ExceptionCode ec = 0;
        coreSelf->setAttrWithSetterException((g_value_get_long(value)), ec);
        break;
    }
    case PROP_ATTR_WITH_GETTER_EXCEPTION:
    {
        WebCore::ExceptionCode ec = 0;
        coreSelf->setAttrWithGetterException((g_value_get_long(value)), ec);
        break;
    }
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}


static void webkit_dom_test_obj_get_property(GObject* object, guint prop_id, GValue* value, GParamSpec* pspec)
{
    WebKitDOMTestObj* self = WEBKIT_DOM_TEST_OBJ(object);
    WebCore::TestObj* coreSelf = WebKit::core(self);
    switch (prop_id) {
    case PROP_READ_ONLY_INT_ATTR:
    {
        g_value_set_long(value, coreSelf->readOnlyIntAttr());
        break;
    }
    case PROP_READ_ONLY_STRING_ATTR:
    {
        g_value_take_string(value, convertToUTF8String(coreSelf->readOnlyStringAttr()));
        break;
    }
    case PROP_READ_ONLY_TEST_OBJ_ATTR:
    {
        RefPtr<WebCore::TestObj> ptr = coreSelf->readOnlyTestObjAttr();
        g_value_set_object(value, WebKit::kit(ptr.get()));
        break;
    }
    case PROP_INT_ATTR:
    {
        g_value_set_long(value, coreSelf->intAttr());
        break;
    }
    case PROP_LONG_LONG_ATTR:
    {
        g_value_set_int64(value, coreSelf->longLongAttr());
        break;
    }
    case PROP_UNSIGNED_LONG_LONG_ATTR:
    {
        g_value_set_uint64(value, coreSelf->unsignedLongLongAttr());
        break;
    }
    case PROP_STRING_ATTR:
    {
        g_value_take_string(value, convertToUTF8String(coreSelf->stringAttr()));
        break;
    }
    case PROP_TEST_OBJ_ATTR:
    {
        RefPtr<WebCore::TestObj> ptr = coreSelf->testObjAttr();
        g_value_set_object(value, WebKit::kit(ptr.get()));
        break;
    }
    case PROP_ATTR_WITH_EXCEPTION:
    {
        g_value_set_long(value, coreSelf->attrWithException());
        break;
    }
    case PROP_ATTR_WITH_SETTER_EXCEPTION:
    {
        WebCore::ExceptionCode ec = 0;
        g_value_set_long(value, coreSelf->attrWithSetterException(ec));
        break;
    }
    case PROP_ATTR_WITH_GETTER_EXCEPTION:
    {
        g_value_set_long(value, coreSelf->attrWithGetterException());
        break;
    }
    case PROP_SCRIPT_STRING_ATTR:
    {
        g_value_take_string(value, convertToUTF8String(coreSelf->scriptStringAttr()));
        break;
    }
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}


static void webkit_dom_test_obj_class_init(WebKitDOMTestObjClass* requestClass)
{
    GObjectClass *gobjectClass = G_OBJECT_CLASS(requestClass);
    gobjectClass->finalize = webkit_dom_test_obj_finalize;
    gobjectClass->set_property = webkit_dom_test_obj_set_property;
    gobjectClass->get_property = webkit_dom_test_obj_get_property;

    g_object_class_install_property(gobjectClass,
                                    PROP_READ_ONLY_INT_ATTR,
                                    g_param_spec_long("read-only-int-attr", /* name */
                                                           "test_obj_read-only-int-attr", /* short description */
                                                           "read-only  glong TestObj.read-only-int-attr", /* longer - could do with some extra doc stuff here */
                                                           G_MINLONG, /* min */
G_MAXLONG, /* max */
0, /* default */
                                                           WEBKIT_PARAM_READABLE));
    g_object_class_install_property(gobjectClass,
                                    PROP_READ_ONLY_STRING_ATTR,
                                    g_param_spec_string("read-only-string-attr", /* name */
                                                           "test_obj_read-only-string-attr", /* short description */
                                                           "read-only  gchar*  TestObj.read-only-string-attr", /* longer - could do with some extra doc stuff here */
                                                           "", /* default */
                                                           WEBKIT_PARAM_READABLE));
    g_object_class_install_property(gobjectClass,
                                    PROP_READ_ONLY_TEST_OBJ_ATTR,
                                    g_param_spec_object("read-only-test-obj-attr", /* name */
                                                           "test_obj_read-only-test-obj-attr", /* short description */
                                                           "read-only  WebKitDOMTestObj*  TestObj.read-only-test-obj-attr", /* longer - could do with some extra doc stuff here */
                                                           WEBKIT_TYPE_DOM_TEST_OBJ, /* gobject type */
                                                           WEBKIT_PARAM_READABLE));
    g_object_class_install_property(gobjectClass,
                                    PROP_INT_ATTR,
                                    g_param_spec_long("int-attr", /* name */
                                                           "test_obj_int-attr", /* short description */
                                                           "read-write  glong TestObj.int-attr", /* longer - could do with some extra doc stuff here */
                                                           G_MINLONG, /* min */
G_MAXLONG, /* max */
0, /* default */
                                                           WEBKIT_PARAM_READWRITE));
    g_object_class_install_property(gobjectClass,
                                    PROP_LONG_LONG_ATTR,
                                    g_param_spec_int64("long-long-attr", /* name */
                                                           "test_obj_long-long-attr", /* short description */
                                                           "read-write  gint64 TestObj.long-long-attr", /* longer - could do with some extra doc stuff here */
                                                           G_MININT64, /* min */
G_MAXINT64, /* max */
0, /* default */
                                                           WEBKIT_PARAM_READWRITE));
    g_object_class_install_property(gobjectClass,
                                    PROP_UNSIGNED_LONG_LONG_ATTR,
                                    g_param_spec_uint64("unsigned-long-long-attr", /* name */
                                                           "test_obj_unsigned-long-long-attr", /* short description */
                                                           "read-write  guint64 TestObj.unsigned-long-long-attr", /* longer - could do with some extra doc stuff here */
                                                           0, /* min */
G_MAXUINT64, /* min */
0, /* default */
                                                           WEBKIT_PARAM_READWRITE));
    g_object_class_install_property(gobjectClass,
                                    PROP_STRING_ATTR,
                                    g_param_spec_string("string-attr", /* name */
                                                           "test_obj_string-attr", /* short description */
                                                           "read-write  gchar*  TestObj.string-attr", /* longer - could do with some extra doc stuff here */
                                                           "", /* default */
                                                           WEBKIT_PARAM_READWRITE));
    g_object_class_install_property(gobjectClass,
                                    PROP_TEST_OBJ_ATTR,
                                    g_param_spec_object("test-obj-attr", /* name */
                                                           "test_obj_test-obj-attr", /* short description */
                                                           "read-write  WebKitDOMTestObj*  TestObj.test-obj-attr", /* longer - could do with some extra doc stuff here */
                                                           WEBKIT_TYPE_DOM_TEST_OBJ, /* gobject type */
                                                           WEBKIT_PARAM_READWRITE));
    g_object_class_install_property(gobjectClass,
                                    PROP_ATTR_WITH_EXCEPTION,
                                    g_param_spec_long("attr-with-exception", /* name */
                                                           "test_obj_attr-with-exception", /* short description */
                                                           "read-write  glong TestObj.attr-with-exception", /* longer - could do with some extra doc stuff here */
                                                           G_MINLONG, /* min */
G_MAXLONG, /* max */
0, /* default */
                                                           WEBKIT_PARAM_READWRITE));
    g_object_class_install_property(gobjectClass,
                                    PROP_ATTR_WITH_SETTER_EXCEPTION,
                                    g_param_spec_long("attr-with-setter-exception", /* name */
                                                           "test_obj_attr-with-setter-exception", /* short description */
                                                           "read-write  glong TestObj.attr-with-setter-exception", /* longer - could do with some extra doc stuff here */
                                                           G_MINLONG, /* min */
G_MAXLONG, /* max */
0, /* default */
                                                           WEBKIT_PARAM_READWRITE));
    g_object_class_install_property(gobjectClass,
                                    PROP_ATTR_WITH_GETTER_EXCEPTION,
                                    g_param_spec_long("attr-with-getter-exception", /* name */
                                                           "test_obj_attr-with-getter-exception", /* short description */
                                                           "read-write  glong TestObj.attr-with-getter-exception", /* longer - could do with some extra doc stuff here */
                                                           G_MINLONG, /* min */
G_MAXLONG, /* max */
0, /* default */
                                                           WEBKIT_PARAM_READWRITE));
    g_object_class_install_property(gobjectClass,
                                    PROP_SCRIPT_STRING_ATTR,
                                    g_param_spec_string("script-string-attr", /* name */
                                                           "test_obj_script-string-attr", /* short description */
                                                           "read-only  gchar*  TestObj.script-string-attr", /* longer - could do with some extra doc stuff here */
                                                           "", /* default */
                                                           WEBKIT_PARAM_READABLE));


}

static void webkit_dom_test_obj_init(WebKitDOMTestObj* request)
{
}

namespace WebKit {
WebKitDOMTestObj* wrapTestObj(WebCore::TestObj* coreObject)
{
    g_return_val_if_fail(coreObject, 0);
    
    WebKitDOMTestObj* wrapper = WEBKIT_DOM_TEST_OBJ(g_object_new(WEBKIT_TYPE_DOM_TEST_OBJ, NULL));
    g_return_val_if_fail(wrapper, 0);

    /* We call ref() rather than using a C++ smart pointer because we can't store a C++ object
     * in a C-allocated GObject structure.  See the finalize() code for the
     * matching deref().
     */

    coreObject->ref();
    WEBKIT_DOM_OBJECT(wrapper)->coreObject = coreObject;


    return wrapper;
}
} // namespace WebKit
