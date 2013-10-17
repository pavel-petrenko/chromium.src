/*
 * Copyright (C) 2003, 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
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

#ifndef AXObjectCache_h
#define AXObjectCache_h

#include "core/accessibility/AXObject.h"
#include "platform/Timer.h"
#include "wtf/Forward.h"
#include "wtf/HashMap.h"
#include "wtf/HashSet.h"
#include "wtf/RefPtr.h"

namespace WebCore {

class Document;
class HTMLAreaElement;
class Node;
class Page;
class RenderObject;
class ScrollView;
class VisiblePosition;
class Widget;

struct TextMarkerData {
    AXID axID;
    Node* node;
    int offset;
    EAffinity affinity;
};

class AXComputedObjectAttributeCache {
public:
    static PassOwnPtr<AXComputedObjectAttributeCache> create() { return adoptPtr(new AXComputedObjectAttributeCache()); }

    AXObjectInclusion getIgnored(AXID) const;
    void setIgnored(AXID, AXObjectInclusion);

    void clear();

private:
    AXComputedObjectAttributeCache() { }

    struct CachedAXObjectAttributes {
        CachedAXObjectAttributes() : ignored(DefaultBehavior) { }

        AXObjectInclusion ignored;
    };

    HashMap<AXID, CachedAXObjectAttributes> m_idMapping;
};

enum PostType { PostSynchronously, PostAsynchronously };

class AXObjectCache {
    WTF_MAKE_NONCOPYABLE(AXObjectCache); WTF_MAKE_FAST_ALLOCATED;
public:
    explicit AXObjectCache(const Document*);
    ~AXObjectCache();

    static AXObject* focusedUIElementForPage(const Page*);

    // Returns the root object for the entire document.
    AXObject* rootObject();

    // For AX objects with elements that back them.
    AXObject* getOrCreate(RenderObject*);
    AXObject* getOrCreate(Widget*);
    AXObject* getOrCreate(Node*);

    // used for objects without backing elements
    AXObject* getOrCreate(AccessibilityRole);

    // will only return the AXObject if it already exists
    AXObject* get(RenderObject*);
    AXObject* get(Widget*);
    AXObject* get(Node*);

    void remove(RenderObject*);
    void remove(Node*);
    void remove(Widget*);
    void remove(AXID);

    void detachWrapper(AXObject*);
    void attachWrapper(AXObject*);
    void childrenChanged(Node*);
    void childrenChanged(RenderObject*);
    void childrenChanged(AXObject*);
    void checkedStateChanged(Node*);
    void selectedChildrenChanged(Node*);
    void selectedChildrenChanged(RenderObject*);
    void selectionChanged(Node*);
    // Called by a node when text or a text equivalent (e.g. alt) attribute is changed.
    void textChanged(Node*);
    void textChanged(RenderObject*);
    // Called when a node has just been attached, so we can make sure we have the right subclass of AXObject.
    void updateCacheAfterNodeIsAttached(Node*);

    void handleActiveDescendantChanged(Node*);
    void handleAriaRoleChanged(Node*);
    void handleFocusedUIElementChanged(Node* oldFocusedNode, Node* newFocusedNode);
    void handleScrolledToAnchor(const Node* anchorNode);
    void handleAriaExpandedChange(Node*);
    void handleScrollbarUpdate(ScrollView*);

    void handleAttributeChanged(const QualifiedName& attrName, Element*);
    void recomputeIsIgnored(RenderObject* renderer);

#if HAVE(ACCESSIBILITY)
    static void enableAccessibility() { gAccessibilityEnabled = true; }
    static bool accessibilityEnabled() { return gAccessibilityEnabled; }
#else
    static void enableAccessibility() { }
    static bool accessibilityEnabled() { return false; }
#endif

    void removeAXID(AXObject*);
    bool isIDinUse(AXID id) const { return m_idsInUse.contains(id); }

    Element* rootAXEditableElement(Node*);
    const Element* rootAXEditableElement(const Node*);
    bool nodeIsTextControl(const Node*);

    AXID platformGenerateAXID() const;
    AXObject* objectFromAXID(AXID id) const { return m_objects.get(id); }

    // Text marker utilities.
    void textMarkerDataForVisiblePosition(TextMarkerData&, const VisiblePosition&);
    VisiblePosition visiblePositionForTextMarkerData(TextMarkerData&);

    enum AXNotification {
        AXActiveDescendantChanged,
        AXAlert,
        AXAriaAttributeChanged,
        AXAutocorrectionOccured,
        AXBlur,
        AXCheckedStateChanged,
        AXChildrenChanged,
        AXFocusedUIElementChanged,
        AXHide,
        AXInvalidStatusChanged,
        AXLayoutComplete,
        AXLiveRegionChanged,
        AXLoadComplete,
        AXLocationChanged,
        AXMenuListItemSelected,
        AXMenuListValueChanged,
        AXRowCollapsed,
        AXRowCountChanged,
        AXRowExpanded,
        AXScrolledToAnchor,
        AXSelectedChildrenChanged,
        AXSelectedTextChanged,
        AXShow,
        AXTextChanged,
        AXTextInserted,
        AXTextRemoved,
        AXValueChanged
    };

    void postNotification(RenderObject*, AXNotification, bool postToElement, PostType = PostAsynchronously);
    void postNotification(Node*, AXNotification, bool postToElement, PostType = PostAsynchronously);
    void postNotification(AXObject*, Document*, AXNotification, bool postToElement, PostType = PostAsynchronously);

    bool nodeHasRole(Node*, const AtomicString& role);

    void startCachingComputedObjectAttributesUntilTreeMutates();
    void stopCachingComputedObjectAttributes();

    AXComputedObjectAttributeCache* computedObjectAttributeCache() { return m_computedObjectAttributeCache.get(); }

protected:
    void postPlatformNotification(AXObject*, AXNotification);
    void textChanged(AXObject*);
    void labelChanged(Element*);

    // This is a weak reference cache for knowing if Nodes used by TextMarkers are valid.
    void setNodeInUse(Node* n) { m_textMarkerNodes.add(n); }
    void removeNodeForUse(Node* n) { m_textMarkerNodes.remove(n); }
    bool isNodeInUse(Node* n) { return m_textMarkerNodes.contains(n); }

private:
    Document* m_document;
    HashMap<AXID, RefPtr<AXObject> > m_objects;
    HashMap<RenderObject*, AXID> m_renderObjectMapping;
    HashMap<Widget*, AXID> m_widgetObjectMapping;
    HashMap<Node*, AXID> m_nodeObjectMapping;
    HashSet<Node*> m_textMarkerNodes;
    OwnPtr<AXComputedObjectAttributeCache> m_computedObjectAttributeCache;
    static bool gAccessibilityEnabled;

    HashSet<AXID> m_idsInUse;

    Timer<AXObjectCache> m_notificationPostTimer;
    Vector<pair<RefPtr<AXObject>, AXNotification> > m_notificationsToPost;
    void notificationPostTimerFired(Timer<AXObjectCache>*);

    static AXObject* focusedImageMapUIElement(HTMLAreaElement*);

    AXID getAXID(AXObject*);
};

bool nodeHasRole(Node*, const String& role);
// This will let you know if aria-hidden was explicitly set to false.
bool isNodeAriaVisible(Node*);

#if !HAVE(ACCESSIBILITY)
inline AXObjectInclusion AXComputedObjectAttributeCache::getIgnored(AXID) const { return DefaultBehavior; }
inline void AXComputedObjectAttributeCache::setIgnored(AXID, AXObjectInclusion) { }
inline AXObjectCache::AXObjectCache(const Document* doc) : m_document(const_cast<Document*>(doc)), m_notificationPostTimer(this, 0) { }
inline AXObjectCache::~AXObjectCache() { }
inline AXObject* AXObjectCache::focusedUIElementForPage(const Page*) { return 0; }
inline AXObject* AXObjectCache::get(RenderObject*) { return 0; }
inline AXObject* AXObjectCache::get(Node*) { return 0; }
inline AXObject* AXObjectCache::get(Widget*) { return 0; }
inline AXObject* AXObjectCache::getOrCreate(AccessibilityRole) { return 0; }
inline AXObject* AXObjectCache::getOrCreate(RenderObject*) { return 0; }
inline AXObject* AXObjectCache::getOrCreate(Node*) { return 0; }
inline AXObject* AXObjectCache::getOrCreate(Widget*) { return 0; }
inline AXObject* AXObjectCache::rootObject() { return 0; }
inline Element* AXObjectCache::rootAXEditableElement(Node*) { return 0; }
inline bool nodeHasRole(Node*, const String&) { return false; }
inline void AXObjectCache::startCachingComputedObjectAttributesUntilTreeMutates() { }
inline void AXObjectCache::stopCachingComputedObjectAttributes() { }
inline bool isNodeAriaVisible(Node*) { return true; }
inline const Element* AXObjectCache::rootAXEditableElement(const Node*) { return 0; }
inline void AXObjectCache::attachWrapper(AXObject*) { }
inline void AXObjectCache::checkedStateChanged(Node*) { }
inline void AXObjectCache::childrenChanged(RenderObject*) { }
inline void AXObjectCache::childrenChanged(Node*) { }
inline void AXObjectCache::childrenChanged(AXObject*) { }
inline void AXObjectCache::textChanged(RenderObject*) { }
inline void AXObjectCache::textChanged(Node*) { }
inline void AXObjectCache::textChanged(AXObject*) { }
inline void AXObjectCache::updateCacheAfterNodeIsAttached(Node*) { }
inline void AXObjectCache::detachWrapper(AXObject*) { }
inline void AXObjectCache::handleActiveDescendantChanged(Node*) { }
inline void AXObjectCache::handleAriaExpandedChange(Node*) { }
inline void AXObjectCache::handleAriaRoleChanged(Node*) { }
inline void AXObjectCache::handleFocusedUIElementChanged(Node*, Node*) { }
inline void AXObjectCache::handleScrollbarUpdate(ScrollView*) { }
inline void AXObjectCache::handleAttributeChanged(const QualifiedName&, Element*) { }
inline void AXObjectCache::recomputeIsIgnored(RenderObject*) { }
inline void AXObjectCache::handleScrolledToAnchor(const Node*) { }
inline void AXObjectCache::nodeTextChangeNotification(Node*, AXTextChange, unsigned, const String&) { }
inline void AXObjectCache::nodeTextChangePlatformNotification(AXObject*, AXTextChange, unsigned, const String&) { }
inline void AXObjectCache::postNotification(AXObject*, Document*, AXNotification, bool, PostType) { }
inline void AXObjectCache::postNotification(RenderObject*, AXNotification, bool, PostType) { }
inline void AXObjectCache::postNotification(Node*, AXNotification, bool, PostType) { }
inline void AXObjectCache::postPlatformNotification(AXObject*, AXNotification) { }
inline void AXObjectCache::remove(AXID) { }
inline void AXObjectCache::remove(RenderObject*) { }
inline void AXObjectCache::remove(Node*) { }
inline void AXObjectCache::remove(Widget*) { }
inline void AXObjectCache::selectedChildrenChanged(RenderObject*) { }
inline void AXObjectCache::selectedChildrenChanged(Node*) { }
#endif

}

#endif
