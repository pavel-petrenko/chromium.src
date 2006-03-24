/*
 * Copyright (C) 2003, 2004, 2005, 2006 Apple Computer, Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "config.h"
#import "AccessibilityObjectCache.h"

#import "Document.h"
#import "RenderObject.h"
#import "VisiblePosition.h"
#import "WebCoreAXObject.h"
#import "WebCoreViewFactory.h"
#import <kxmlcore/Assertions.h>

// The simple Cocoa calls in this file don't throw exceptions.

namespace WebCore {

struct TextMarkerData  {
    AXID axID;
    Node* node;
    int offset;
    EAffinity affinity;
};

bool AccessibilityObjectCache::gAccessibilityEnabled = false;

AccessibilityObjectCache::~AccessibilityObjectCache()
{
    HashMap<RenderObject*, WebCoreAXObject*>::iterator end = m_objects.end();
    for (HashMap<RenderObject*, WebCoreAXObject*>::iterator it = m_objects.begin(); it != end; ++it) {
        WebCoreAXObject* obj = (*it).second;
        [obj detach];
        CFRelease(obj);
    }
}

WebCoreAXObject* AccessibilityObjectCache::get(RenderObject* renderer)
{
    WebCoreAXObject* obj = m_objects.get(renderer);
    if (obj)
        return obj;

    obj = [[WebCoreAXObject alloc] initWithRenderer:renderer];
    CFRetain(obj);
    m_objects.set(renderer, obj);
    [obj release];
    return obj;
}

void AccessibilityObjectCache::remove(RenderObject* renderer)
{
    HashMap<RenderObject*, WebCoreAXObject*>::iterator it = m_objects.find(renderer);
    if (it == m_objects.end())
        return;
    WebCoreAXObject* obj = (*it).second;
    [obj detach];
    CFRelease(obj);
    m_objects.remove(it);

    ASSERT(m_objects.size() >= m_idsInUse.size());
}

AXID AccessibilityObjectCache::getAXID(WebCoreAXObject* obj)
{
    // check for already-assigned ID
    AXID objID = [obj axObjectID];
    if (objID) {
        ASSERT(m_idsInUse.contains(objID));
        return objID;
    }

    // generate a new ID
    static AXID lastUsedID = 0;
    objID = lastUsedID;
    do
        ++objID;
    while (objID == 0 || objID == AXIDHashTraits::deletedValue() || m_idsInUse.contains(objID));
    m_idsInUse.add(objID);
    lastUsedID = objID;
    [obj setAXObjectID:objID];

    return objID;
}

void AccessibilityObjectCache::removeAXID(WebCoreAXObject* obj)
{
    AXID objID = [obj axObjectID];
    if (objID == 0)
        return;
    ASSERT(objID != AXIDHashTraits::deletedValue());
    ASSERT(m_idsInUse.contains(objID));
    [obj setAXObjectID:0];
    m_idsInUse.remove(objID);
}

WebCoreTextMarker* AccessibilityObjectCache::textMarkerForVisiblePosition(const VisiblePosition& visiblePos)
{
    Position deepPos = visiblePos.deepEquivalent();
    Node* domNode = deepPos.node();
    ASSERT(domNode);
    if (!domNode)
        return nil;
    
    // locate the renderer, which must exist for a visible dom node
    RenderObject* renderer = domNode->renderer();
    ASSERT(renderer);
    
    // find or create an accessibility object for this renderer
    WebCoreAXObject* obj = get(renderer);
    
    // create a text marker, adding an ID for the WebCoreAXObject if needed
    TextMarkerData textMarkerData;
    textMarkerData.axID = getAXID(obj);
    textMarkerData.node = domNode;
    textMarkerData.offset = deepPos.offset();
    textMarkerData.affinity = visiblePos.affinity();
    return [[WebCoreViewFactory sharedFactory] textMarkerWithBytes:&textMarkerData length:sizeof(textMarkerData)]; 
}

VisiblePosition AccessibilityObjectCache::visiblePositionForTextMarker(WebCoreTextMarker* textMarker)
{
    TextMarkerData textMarkerData;
    
    if (![[WebCoreViewFactory sharedFactory] getBytes:&textMarkerData fromTextMarker:textMarker length:sizeof(textMarkerData)])
        return VisiblePosition();

    // return empty position if the text marker is no longer valid
    if (!m_idsInUse.contains(textMarkerData.axID))
        return VisiblePosition();

    // return the position from the data we stored earlier
    return VisiblePosition(textMarkerData.node, textMarkerData.offset, textMarkerData.affinity);
}

void AccessibilityObjectCache::childrenChanged(RenderObject* renderer)
{
    WebCoreAXObject* obj = m_objects.get(renderer);
    if (obj)
        [obj childrenChanged];
}

void AccessibilityObjectCache::postNotificationToTopWebArea(RenderObject* renderer, const String& message)
{
    if (renderer)
        NSAccessibilityPostNotification(get(renderer->document()->topDocument()->renderer()), message);
}

void AccessibilityObjectCache::postNotification(RenderObject* renderer, const String& message)
{
    if (renderer)
        NSAccessibilityPostNotification(get(renderer), message);
}

void AccessibilityObjectCache::handleFocusedUIElementChanged()
{
    [[WebCoreViewFactory sharedFactory] accessibilityHandleFocusChanged];
}

}
