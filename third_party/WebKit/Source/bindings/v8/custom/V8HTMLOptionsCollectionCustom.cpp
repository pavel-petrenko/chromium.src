/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
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

#include "config.h"
#include "V8HTMLOptionsCollection.h"

#include "core/dom/ExceptionCode.h"
#include "core/dom/NamedNodesCollection.h"
#include "core/html/HTMLOptionElement.h"
#include "core/html/HTMLOptionsCollection.h"

#include "V8HTMLOptionElement.h"
#include "V8Node.h"
#include "V8NodeList.h"
#include "bindings/v8/V8Binding.h"
#include "bindings/v8/V8Collection.h"
#include "bindings/v8/custom/V8HTMLSelectElementCustom.h"

namespace WebCore {

template<typename HolderContainer>
static v8::Handle<v8::Value> getNamedItems(HTMLOptionsCollection* collection, const AtomicString& name, const HolderContainer& holder)
{
    Vector<RefPtr<Node> > namedItems;
    collection->namedItems(name, namedItems);

    if (!namedItems.size())
        return v8Undefined();

    if (namedItems.size() == 1)
        return toV8Fast(namedItems.at(0).release(), holder, collection);

    return toV8Fast(NamedNodesCollection::create(namedItems), holder, collection);
}

v8::Handle<v8::Value> V8HTMLOptionsCollection::namedItemMethodCustom(const v8::Arguments& args)
{
    HTMLOptionsCollection* imp = V8HTMLOptionsCollection::toNative(args.Holder());
    v8::Handle<v8::Value> result = getNamedItems(imp, toWebCoreString(args[0]), args);

    if (result.IsEmpty())
        return v8Null(args.GetIsolate());

    return result;
}

v8::Handle<v8::Value> V8HTMLOptionsCollection::removeMethodCustom(const v8::Arguments& args)
{
    HTMLOptionsCollection* imp = V8HTMLOptionsCollection::toNative(args.Holder());
    HTMLSelectElement* base = toHTMLSelectElement(imp->ownerNode());
    return removeElement(base, args);
}

v8::Handle<v8::Value> V8HTMLOptionsCollection::addMethodCustom(const v8::Arguments& args)
{
    if (!V8HTMLOptionElement::HasInstance(args[0], args.GetIsolate(), worldType(args.GetIsolate())))
        return setDOMException(TYPE_MISMATCH_ERR, args.GetIsolate());
    HTMLOptionsCollection* imp = V8HTMLOptionsCollection::toNative(args.Holder());
    HTMLOptionElement* option = V8HTMLOptionElement::toNative(v8::Handle<v8::Object>(v8::Handle<v8::Object>::Cast(args[0])));

    ExceptionCode ec = 0;
    if (args.Length() < 2)
        imp->add(option, ec);
    else {
        bool ok;
        V8TRYCATCH(int, index, toInt32(args[1], ok));
        if (!ok)
            ec = TYPE_MISMATCH_ERR;
        else
            imp->add(option, index, ec);
    }

    if (ec)
        return setDOMException(ec, args.GetIsolate());

    return v8::Undefined();
}

void V8HTMLOptionsCollection::lengthAttrSetterCustom(v8::Local<v8::String> name, v8::Local<v8::Value> value, const v8::AccessorInfo& info)
{
    HTMLOptionsCollection* imp = V8HTMLOptionsCollection::toNative(info.Holder());
    double v = value->NumberValue();
    unsigned newLength = 0;
    ExceptionCode ec = 0;
    if (!std::isnan(v) && !std::isinf(v)) {
        if (v < 0.0)
            ec = INDEX_SIZE_ERR;
        else if (v > static_cast<double>(UINT_MAX))
            newLength = UINT_MAX;
        else
            newLength = static_cast<unsigned>(v);
    }
    if (!ec)
        imp->setLength(newLength, ec);

    setDOMException(ec, info.GetIsolate());
}

v8::Handle<v8::Value> V8HTMLOptionsCollection::indexedPropertySetter(uint32_t index, v8::Local<v8::Value> value, const v8::AccessorInfo& info)
{
    HTMLOptionsCollection* collection = V8HTMLOptionsCollection::toNative(info.Holder());
    HTMLSelectElement* base = toHTMLSelectElement(collection->ownerNode());
    return toOptionsCollectionSetter(index, value, base, info.GetIsolate());
}

} // namespace WebCore
