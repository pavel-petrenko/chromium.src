/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2010 Antonio Gomes <tonikitoo@webkit.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef StaticHashSetNodeList_h
#define StaticHashSetNodeList_h

#include "core/dom/NodeList.h"
#include <wtf/ListHashSet.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class Node;

class StaticHashSetNodeList : public NodeList {
public:
    StaticHashSetNodeList();
    ~StaticHashSetNodeList();

    // Adopts the contents of the nodes ListHashSet.
    static PassRefPtr<StaticHashSetNodeList> adopt(const ListHashSet<RefPtr<Node> >& nodes)
    {
        return adopt(const_cast<ListHashSet<RefPtr<Node> >&>(nodes));
    }

    static PassRefPtr<StaticHashSetNodeList> adopt(ListHashSet<RefPtr<Node> >& nodes)
    {
        return adoptRef(new StaticHashSetNodeList(nodes));
    }

    virtual unsigned length() const OVERRIDE;
    virtual Node* item(unsigned index) const OVERRIDE;
    virtual Node* namedItem(const AtomicString&) const OVERRIDE;

private:
    explicit StaticHashSetNodeList(ListHashSet<RefPtr<Node> >& nodes);

    ListHashSet<RefPtr<Node> > m_nodes;
};

} // namespace WebCore

#endif // StaticHashSetNodeList_h
