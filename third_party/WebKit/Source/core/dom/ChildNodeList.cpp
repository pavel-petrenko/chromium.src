/**
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2007, 2008 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "core/dom/ChildNodeList.h"

#include "core/dom/Element.h"
#include "core/dom/NodeRareData.h"

namespace WebCore {

ChildNodeList::ChildNodeList(PassRefPtr<Node> node)
    : LiveNodeList(node, ChildNodeListType, DoNotInvalidateOnAttributeChanges)
{
}

ChildNodeList::~ChildNodeList()
{
    ownerNode()->nodeLists()->removeChildNodeList(this);
}

bool ChildNodeList::nodeMatches(Element* testNode) const
{
    // This function will be called only by LiveNodeList::namedItem,
    // for an element that was located with getElementById.
    return testNode->parentNode() == rootNode();
}

} // namespace WebCore
