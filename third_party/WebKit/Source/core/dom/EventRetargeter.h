/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
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

#ifndef EventRetargeter_h
#define EventRetargeter_h

#include "core/dom/ContainerNode.h"
#include "core/dom/EventContext.h"
#include "core/dom/shadow/ShadowRoot.h"
#include <wtf/HashMap.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>

#if ENABLE(SVG)
#include "SVGNames.h"
#include "core/svg/SVGElementInstance.h"
#include "core/svg/SVGUseElement.h"
#endif

namespace WebCore {

class EventTarget;
class FocusEvent;
class MouseEvent;
class Node;
class TouchEvent;
class TreeScope;

enum EventDispatchBehavior {
    RetargetEvent,
    StayInsideShadowDOM
};

class EventRetargeter {
public:
    static void calculateEventPath(Node*, Event*);
    static void adjustForMouseEvent(Node*, MouseEvent&);
    static void adjustForFocusEvent(Node*, FocusEvent&);
    typedef Vector<RefPtr<TouchList> > EventPathTouchLists;
    static void adjustForTouchEvent(Node*, TouchEvent&);
    static EventTarget* eventTargetRespectingTargetRules(Node* referenceNode);

private:
    typedef Vector<RefPtr<Node> > AdjustedNodes;
    typedef HashMap<TreeScope*, Node*> RelatedNodeMap;
    enum EventWithRelatedTargetDispatchBehavior {
        StopAtBoundaryIfNeeded,
        DoesNotStopAtBoundary
    };
    static void adjustForRelatedTarget(const Node*, EventTarget* relatedTarget, EventPath&);
    static void calculateAdjustedNodes(const Node*, const Node* relatedNode, EventWithRelatedTargetDispatchBehavior, EventPath&, AdjustedNodes&);
    static void buildRelatedNodeMap(const Node*, RelatedNodeMap&);
    static Node* findRelatedNode(TreeScope*, RelatedNodeMap&);
    static void adjustTouchList(const Node*, const TouchList*, const EventPath&, EventPathTouchLists&);
};

inline EventTarget* EventRetargeter::eventTargetRespectingTargetRules(Node* referenceNode)
{
    ASSERT(referenceNode);

    if (referenceNode->isPseudoElement())
        return referenceNode->parentNode();

#if ENABLE(SVG)
    if (!referenceNode->isSVGElement() || !referenceNode->isInShadowTree())
        return referenceNode;

    // Spec: The event handling for the non-exposed tree works as if the referenced element had been textually included
    // as a deeply cloned child of the 'use' element, except that events are dispatched to the SVGElementInstance objects
    Element* shadowHostElement = toShadowRoot(referenceNode->treeScope()->rootNode())->host();
    // At this time, SVG nodes are not supported in non-<use> shadow trees.
    if (!shadowHostElement || !shadowHostElement->hasTagName(SVGNames::useTag))
        return referenceNode;
    SVGUseElement* useElement = static_cast<SVGUseElement*>(shadowHostElement);
    if (SVGElementInstance* instance = useElement->instanceForShadowTreeElement(referenceNode))
        return instance;
#endif

    return referenceNode;
}

}

#endif // EventRetargeter_h
