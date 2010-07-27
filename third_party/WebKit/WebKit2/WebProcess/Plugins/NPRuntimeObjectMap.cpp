/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "NPRuntimeObjectMap.h"

#include "JSNPObject.h"
#include "NPJSObject.h"
#include "NPRuntimeUtilities.h"
#include "NotImplemented.h"
#include "PluginView.h"
#include <WebCore/Frame.h>

using namespace JSC;
using namespace WebCore;

namespace WebKit {


NPRuntimeObjectMap::NPRuntimeObjectMap(PluginView* pluginView)
    : m_pluginView(pluginView)
{
}

NPObject* NPRuntimeObjectMap::getOrCreateNPObject(JSObject* jsObject)
{
    // First, check if we already know about this object.
    if (NPJSObject* npJSObject = m_objects.get(jsObject)) {
        retainNPObject(npJSObject);
        return npJSObject;
    }

    NPJSObject* npJSObject = NPJSObject::create(this, jsObject);
    m_objects.set(jsObject, npJSObject);

    return npJSObject;
}

void NPRuntimeObjectMap::npJSObjectDestroyed(NPJSObject* npJSObject)
{
    // Remove the object from the map.
    ASSERT(m_objects.contains(npJSObject->jsObject()));
    m_objects.remove(npJSObject->jsObject());
}

JSObject* NPRuntimeObjectMap::getOrCreateJSObject(ExecState* exec, JSGlobalObject* globalObject, NPObject* npObject)
{
    // FIXME: Check if we already have a wrapper for this NPObject!
    return new (exec) JSNPObject(exec, globalObject, this, npObject);
}

void NPRuntimeObjectMap::jsNPObjectDestroyed(JSNPObject* jsNPObject)
{
    // FIXME: Implement.
}

JSValue NPRuntimeObjectMap::convertNPVariantToValue(JSC::ExecState* exec, const NPVariant& variant)
{
    switch (variant.type) {
    case NPVariantType_Void:
        return jsUndefined();

    case NPVariantType_Null:
        return jsNull();

    case NPVariantType_Bool:
        return jsBoolean(variant.value.boolValue);

    case NPVariantType_Int32:
        return jsNumber(exec, variant.value.intValue);

    case NPVariantType_Double:
        return jsNumber(exec, variant.value.doubleValue);

    case NPVariantType_String:
        return jsString(exec, String::fromUTF8WithLatin1Fallback(variant.value.stringValue.UTF8Characters, 
                                                                 variant.value.stringValue.UTF8Length));
    case NPVariantType_Object:
    default:
        notImplemented();
        break;
    }
    
    return jsUndefined();
}

void NPRuntimeObjectMap::invalidate()
{
    Vector<NPJSObject*> npJSObjects;
    copyValuesToVector(m_objects, npJSObjects);

    // Deallocate all the object wrappers so we won't leak any JavaScript objects.
    for (size_t i = 0; i < npJSObjects.size(); ++i)
        deallocateNPObject(npJSObjects[i]);
    
    // We shouldn't have any objects left now.
    ASSERT(m_objects.isEmpty());
}

ExecState* NPRuntimeObjectMap::globalExec() const
{
    Frame* frame = m_pluginView->frame();
    if (!frame)
        return 0;
    
    return frame->script()->globalObject(pluginWorld())->globalExec();
}

} // namespace WebKit
