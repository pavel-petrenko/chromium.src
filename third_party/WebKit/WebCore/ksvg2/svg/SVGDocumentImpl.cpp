/*
    Copyright (C) 2004, 2005 Nikolas Zimmermann <wildfox@kde.org>
                  2004, 2005 Rob Buis <buis@kde.org>

    This file is part of the KDE project

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

#include <assert.h>

#include <qdict.h>

#include <kurl.h>
#include <klocale.h>

#include <kdom/Helper.h>
#include <kdom/Shared.h>
#include <kdom/Namespace.h>
//#include <kdom/DocumentType.h>
//#include <kdom/events/Event.h>
#include <kdom/core/domattrs.h>
#include <kdom/cache/KDOMLoader.h>
#include <kdom/core/CDFInterface.h>
#include <kdom/cache/KDOMCachedObject.h>
#include <kdom/css/CSSStyleSheetImpl.h>
#include <kdom/events/MouseEventImpl.h>
#include <kdom/css/StyleSheetListImpl.h>
#include <kdom/events/KeyboardEventImpl.h>
#include <kdom/core/ProcessingInstructionImpl.h>

#include <kcanvas/KCanvas.h>
#include <kcanvas/KCanvasItem.h>
#include <kcanvas/KCanvasView.h>

#include "ksvg.h"
//#include "Ecma.h"
#include <ksvg2/KSVGView.h>
#include "SVGEventImpl.h"
#include "SVGElementImpl.h"
#include "SVGRenderStyle.h"
#include "SVGDocumentImpl.h"
#include "SVGZoomEventImpl.h"
#include "KSVGTimeScheduler.h"
#include "SVGSVGElementImpl.h"
#include "SVGCSSStyleSelector.h"
#include "SVGCSSStyleSheetImpl.h"
#include "SVGAnimatedStringImpl.h"
#include "SVGDOMImplementationImpl.h"
#include "SVGAElementImpl.h"
#include "SVGGElementImpl.h"
#include "SVGUseElementImpl.h"
#include "SVGSVGElementImpl.h"
#include "SVGSetElementImpl.h"
#include "SVGDescElementImpl.h"
#include "SVGRectElementImpl.h"
#include "SVGDefsElementImpl.h"
#include "SVGStopElementImpl.h"
#include "SVGPathElementImpl.h"
#include "SVGLineElementImpl.h"
#include "SVGViewElementImpl.h"
#include "SVGTextElementImpl.h"
#include "SVGTSpanElementImpl.h"
#include "SVGImageElementImpl.h"
#include "SVGTitleElementImpl.h"
#include "SVGFilterElementImpl.h"
#include "SVGFEImageElementImpl.h"
#include "SVGFEBlendElementImpl.h"
#include "SVGFEFloodElementImpl.h"
#include "SVGFEOffsetElementImpl.h"
#include "SVGFEMergeElementImpl.h"
#include "SVGFEMergeNodeElementImpl.h"
#include "SVGFETurbulenceElementImpl.h"
#include "SVGFEFuncRElementImpl.h"
#include "SVGFEFuncGElementImpl.h"
#include "SVGFEFuncBElementImpl.h"
#include "SVGFEFuncAElementImpl.h"
#include "SVGStyleElementImpl.h"
#include "SVGSwitchElementImpl.h"
#include "SVGScriptElementImpl.h"
#include "SVGCircleElementImpl.h"
#include "SVGSymbolElementImpl.h"
#include "SVGMarkerElementImpl.h"
#include "SVGEllipseElementImpl.h"
#include "SVGAnimateElementImpl.h"
#include "SVGPolygonElementImpl.h"
#include "SVGPatternElementImpl.h"
#include "SVGPolylineElementImpl.h"
#include "SVGClipPathElementImpl.h"
#include "SVGAnimateColorElementImpl.h"
#include "SVGFECompositeElementImpl.h"
#include "SVGFEColorMatrixElementImpl.h"
#include "SVGFEGaussianBlurElementImpl.h"
#include "SVGLinearGradientElementImpl.h"
#include "SVGRadialGradientElementImpl.h"
#include "SVGAnimateTransformElementImpl.h"
#include "SVGFEComponentTransferElementImpl.h"

#include "svgtags.c"
#include "svgtags.h"
#include "svgattrs.c"
#include "svgattrs.h"

using namespace KSVG;

SVGDocumentImpl::SVGDocumentImpl(SVGDOMImplementationImpl *i, KDOM::KDOMView *view) : KDOM::DocumentImpl(i, view, ID_LAST_SVGTAG + 1, ATTR_LAST_SVGATTR + 1), KDOM::CachedObjectClient()
{
    setPaintDevice(svgView()); // Assign our KSVGView as document paint device

    m_namespaceMap->names.insert(0, new KDOM::DOMStringImpl(KDOM::NS_SVG.unicode(), KDOM::NS_SVG.length()));
    m_namespaceMap->count++;

    m_canvasView = 0;
    m_lastTarget = 0;

    m_scriptsIt = 0;
    m_cachedScript = 0;

    m_timeScheduler = new TimeScheduler(this);
}

SVGDocumentImpl::~SVGDocumentImpl()
{
    // Fire UNLOAD_EVENT upon destruction...
    if(KDOM::DocumentImpl::hasListenerType(KDOM::UNLOAD_EVENT))
    {
        SVGEventImpl *event = static_cast<SVGEventImpl *>(createEvent(KDOM::DOMString("SVGEvents").handle()));
        event->ref();

        event->initEvent(KDOM::DOMString("unload").handle(), false, false);
        dispatchRecursiveEvent(event, lastChild());

        event->deref();
    }

    delete m_scriptsIt;
    delete m_timeScheduler;
}

KDOM::DOMStringImpl *SVGDocumentImpl::title() const
{
    if(rootElement())
    {
        for(NodeImpl *child = rootElement()->firstChild(); child != 0; child = child->nextSibling())
            if(child->id() == ID_TITLE)
                return static_cast<SVGTitleElementImpl *>(child)->title();
    }

    return 0;
}

KDOM::DOMStringImpl *SVGDocumentImpl::referrer() const
{
    // TODO
    return 0;
}

KDOM::DOMStringImpl *SVGDocumentImpl::domain() const
{
    // TODO
    return 0;
}

KDOM::DOMStringImpl *SVGDocumentImpl::URL() const
{
    return new KDOM::DOMStringImpl(m_url.prettyURL());
}

SVGElementImpl *SVGDocumentImpl::createSVGElement(KDOM::DOMStringImpl *prefix, KDOM::DOMStringImpl *localName)
{
    SVGElementImpl *element = 0;
    KDOM::DOMString pref(prefix);
    if(!prefix || prefix->length() == 0)
        pref = KDOM::DOMString();
    QString local = KDOM::DOMString(localName).string();
    KDOM::NodeImpl::Id id = implementation()->cdfInterface()->getTagID(local.ascii(), local.length());
    switch(id)
    {
        case ID_SVG:
        {
            element = new SVGSVGElementImpl(docPtr(), id, prefix);
            break;
        }
        case ID_STYLE:
        {
            element = new SVGStyleElementImpl(docPtr(), id, prefix);
            break;
        }
        case ID_SCRIPT:
        {
            element = new SVGScriptElementImpl(docPtr(), id, prefix);
            break;
        }
        case ID_RECT:
        {
            element = new SVGRectElementImpl(docPtr(), id, prefix);
            break;
        }
        case ID_CIRCLE:
        {
            element = new SVGCircleElementImpl(docPtr(), id, prefix);
            break;
        }
        case ID_ELLIPSE:
        {
            element = new SVGEllipseElementImpl(docPtr(), id, prefix);
            break;
        }
        case ID_POLYLINE:
        {
            element = new SVGPolylineElementImpl(docPtr(), id, prefix);
            break;
        }
        case ID_POLYGON:
        {
            element = new SVGPolygonElementImpl(docPtr(), id, prefix);
            break;
        }
        case ID_G:
        {
            element = new SVGGElementImpl(docPtr(), id, prefix);
            break;
        }
        case ID_SWITCH:
        {
            element = new SVGSwitchElementImpl(docPtr(), id, prefix);
            break;
        }
        case ID_DEFS:
        {
            element = new SVGDefsElementImpl(docPtr(), id, prefix);
            break;
        }
        case ID_STOP:
        {
            element = new SVGStopElementImpl(docPtr(), id, prefix);
            break;
        }
        case ID_PATH:
        {
            element = new SVGPathElementImpl(docPtr(), id, prefix);
            break;
        }
        case ID_IMAGE:
        {
            element = new SVGImageElementImpl(docPtr(), id, prefix);
            break;
        }
        case ID_CLIPPATH:
        {
            element = new SVGClipPathElementImpl(docPtr(), id, prefix);
            break;
        }
        case ID_A:
        {
            element = new SVGAElementImpl(docPtr(), id, prefix);
            break;
        }
        case ID_LINE:
        {
            element = new SVGLineElementImpl(docPtr(), id, prefix);
            break;
        }
        case ID_LINEARGRADIENT:
        {
            element = new SVGLinearGradientElementImpl(docPtr(), id, prefix);
            break;
        }
        case ID_RADIALGRADIENT:
        {
            element = new SVGRadialGradientElementImpl(docPtr(), id, prefix);
            break;
        }
        case ID_TITLE:
        {
            element = new SVGTitleElementImpl(docPtr(), id, prefix);
            break;
        }
        case ID_DESC:
        {
            element = new SVGDescElementImpl(docPtr(), id, prefix);
            break;
        }
        case ID_SYMBOL:
        {
            element = new SVGSymbolElementImpl(docPtr(), id, prefix);
            break;
        }
        case ID_USE:
        {
            element = new SVGUseElementImpl(docPtr(), id, prefix);
            break;
        }
        case ID_PATTERN:
        {
            element = new SVGPatternElementImpl(docPtr(), id, prefix);
            break;
        }
        case ID_ANIMATECOLOR:
        {
            element = new SVGAnimateColorElementImpl(docPtr(), id, prefix);
            break;
        }
        case ID_ANIMATETRANSFORM:
        {
            element = new SVGAnimateTransformElementImpl(docPtr(), id, prefix);
            break;
        }
        case ID_SET:
        {
            element = new SVGSetElementImpl(docPtr(), id, prefix);
            break;
        }
        case ID_ANIMATE:
        {
            element = new SVGAnimateElementImpl(docPtr(), id, prefix);
            break;
        }
        case ID_MARKER:
        {
            element = new SVGMarkerElementImpl(docPtr(), id, prefix);
            break;
        }
        case ID_VIEW:
        {
            element = new SVGViewElementImpl(docPtr(), id, prefix);
            break;
        }
        case ID_FILTER:
        {
            element = new SVGFilterElementImpl(docPtr(), id, prefix);
            break;
        }
        case ID_FEGAUSSIANBLUR:
        {
            element = new SVGFEGaussianBlurElementImpl(docPtr(), id, prefix);
            break;
        }
        case ID_FEFLOOD:
        {
            element = new SVGFEFloodElementImpl(docPtr(), id, prefix);
            break;
        }
        case ID_FEBLEND:
        {
            element = new SVGFEBlendElementImpl(docPtr(), id, prefix);
            break;
        }
        case ID_FEOFFSET:
        {
            element = new SVGFEOffsetElementImpl(docPtr(), id, prefix);
            break;
        }
        case ID_FECOMPOSITE:
        {
            element = new SVGFECompositeElementImpl(docPtr(), id, prefix);
            break;
        }
        case ID_FECOLORMATRIX:
        {
            element = new SVGFEColorMatrixElementImpl(docPtr(), id, prefix);
            break;
        }
        case ID_FEIMAGE:
        {
            element = new SVGFEImageElementImpl(docPtr(), id, prefix);
            break;
        }
        case ID_FEMERGE:
        {
            element = new SVGFEMergeElementImpl(docPtr(), id, prefix);
            break;
        }
        case ID_FEMERGENODE:
        {
            element = new SVGFEMergeNodeElementImpl(docPtr(), id, prefix);
            break;
        }
        case ID_FECOMPONENTTRANSFER:
        {
            element = new SVGFEComponentTransferElementImpl(docPtr(), id, prefix);
            break;
        }
        case ID_FEFUNCR:
        {
            element = new SVGFEFuncRElementImpl(docPtr(), id, prefix);
            break;
        }
        case ID_FEFUNCG:
        {
            element = new SVGFEFuncGElementImpl(docPtr(), id, prefix);
            break;
        }
        case ID_FEFUNCB:
        {
            element = new SVGFEFuncRElementImpl(docPtr(), id, prefix);
            break;
        }
        case ID_FEFUNCA:
        {
            element = new SVGFEFuncAElementImpl(docPtr(), id, prefix);
            break;
        }
        case ID_FETURBULENCE:
        {
            element = new SVGFETurbulenceElementImpl(docPtr(), id, prefix);
            break;
        }
        case ID_TEXT:
        {
            element = new SVGTextElementImpl(docPtr(), id, prefix);
            break;
        }
        case ID_TSPAN:
        {
            element = new SVGTSpanElementImpl(docPtr(), id, prefix);
            break;
        }
        default:
        {
            return 0;
        }
    };

    return element;
}

KDOM::ElementImpl *SVGDocumentImpl::createElement(KDOM::DOMStringImpl *tagName)
{
    SVGElementImpl *elem = createSVGElement(0, tagName);
    if(!elem)
        return KDOM::DocumentImpl::createElement(tagName);

    return elem;
}

KDOM::ElementImpl *SVGDocumentImpl::createElementNS(KDOM::DOMStringImpl *namespaceURIImpl, KDOM::DOMStringImpl *qualifiedName)
{
    KDOM::ElementImpl *elem = NULL;
    if(namespaceURIImpl)
        namespaceURIImpl->ref();
    if(qualifiedName)
        qualifiedName->ref();
    KDOM::DOMStringImpl *prefix = 0, *localName = 0;
    KDOM::Helper::SplitPrefixLocalName(qualifiedName, prefix, localName);

    if(prefix)
        prefix->ref();
    if(localName)
        localName->ref();

    KDOM::DOMString namespaceURI(namespaceURIImpl);
    if(!((!prefix || prefix->length() == 0) || KDOM::DOMString(prefix) == "svg") &&
        (namespaceURI == KDOM::NS_SVG || namespaceURI.isEmpty()))
        elem = KDOM::DocumentImpl::createElementNS(namespaceURIImpl, qualifiedName);
    else {
        int dummy;
        KDOM::Helper::CheckQualifiedName(qualifiedName, namespaceURIImpl, dummy, false, false);

        elem = createSVGElement(prefix, localName);
        if(!elem)
            elem = KDOM::DocumentImpl::createElementNS(namespaceURIImpl, qualifiedName);
    }

    if(prefix)
        prefix->deref();
    if(localName)
        localName->deref();
    if(namespaceURIImpl)
        namespaceURIImpl->deref();
    if(qualifiedName)
        qualifiedName->deref();

    return elem;
}

SVGSVGElementImpl *SVGDocumentImpl::rootElement() const
{
    KDOM::ElementImpl *elem = documentElement();
    if(elem && elem->id() == ID_SVG)
        return static_cast<SVGSVGElementImpl *>(elem);

    return 0;
}

KDOM::EventImpl *SVGDocumentImpl::createEvent(KDOM::DOMStringImpl *eventTypeImpl)
{
    KDOM::DOMString eventType(eventTypeImpl);
    if(eventType == "SVGEvents")
        return new SVGEventImpl();
    else if(eventType == "SVGZoomEvents")
        return new SVGZoomEventImpl();

    return DocumentEventImpl::createEvent(eventTypeImpl);
}

void SVGDocumentImpl::notifyFinished(KDOM::CachedObject *finishedObj)
{
    // This is called when a script has finished loading that was requested from
    // executeScripts().  We execute the script, and then call executeScripts()
    // again to continue iterating through the list of scripts in the document
    if(finishedObj == m_cachedScript)
    {
        KDOM::DOMString scriptSource = m_cachedScript->script();
        
        m_cachedScript->deref(this);
        m_cachedScript = 0;
        
        SVGScriptElementImpl::executeScript(this, scriptSource.string());
        executeScripts(true);
    }
}

KSVGView *SVGDocumentImpl::svgView() const
{
    return static_cast<KSVGView *>(m_view);
}

#if 0
KDOM::Ecma *SVGDocumentImpl::ecmaEngine() const
{
    if(m_ecmaEngine) // No need to initialize anymore...
        return m_ecmaEngine;

    m_ecmaEngine = new Ecma(const_cast<SVGDocumentImpl *>(this));
    m_ecmaEngine->setup(implementation()->cdfInterface());

    return m_ecmaEngine;
}
#endif

void SVGDocumentImpl::finishedParsing()
{
    addScripts(rootElement());

    m_scriptsIt = new QPtrListIterator<SVGScriptElementImpl>(m_scripts);
    executeScripts(false);
}

void SVGDocumentImpl::dispatchRecursiveEvent(KDOM::EventImpl *event, KDOM::NodeImpl *obj)
{
    // Iterate the tree, backwards, and dispatch the event to every child
    for(KDOM::NodeImpl *n = obj; n != 0; n = n->previousSibling())
    {
        if(n->hasChildNodes())
        {
            // Dispatch to all children
            dispatchRecursiveEvent(event, n->lastChild());

            // Dispatch, locally
            n->dispatchEvent(event);
        }
        else
            n->dispatchEvent(event);
    }
}

void SVGDocumentImpl::dispatchZoomEvent(float prevScale, float newScale)
{
    // dispatch zoom event
    SVGZoomEventImpl *event = static_cast<SVGZoomEventImpl *>(createEvent(KDOM::DOMString("SVGZoomEvents").handle()));
    event->ref();

    event->initEvent(KDOM::DOMString("zoom").handle(), true, false);
    event->setPreviousScale(prevScale);
    event->setNewScale(newScale);
    rootElement()->dispatchEvent(event);

    event->deref();
}

void SVGDocumentImpl::dispatchScrollEvent()
{
    // dispatch zoom event
    SVGEventImpl *event = static_cast<SVGEventImpl *>(createEvent(KDOM::DOMString("SVGEvents").handle()));
    event->ref();

    event->initEvent(KDOM::DOMString("scroll").handle(), true, false);
    rootElement()->dispatchEvent(event);

    event->deref();
}

bool SVGDocumentImpl::dispatchKeyEvent(KDOM::EventTargetImpl *target, QKeyEvent *key, bool keypress)
{
    // dispatch key event
    KDOM::KeyboardEventImpl *keyEventImpl = static_cast<KDOM::KeyboardEventImpl *>(createEvent(KDOM::DOMString("KeyboardEvents").handle()));
    keyEventImpl->ref();

    keyEventImpl->initKeyboardEvent(key);
    target->dispatchEvent(keyEventImpl);

    bool r = /*keyEventImpl->defaultHandled() ||*/ keyEventImpl->defaultPrevented();
    keyEventImpl->deref();
    return r;
}

KDOM::DOMStringImpl *SVGDocumentImpl::defaultNS() const
{
    return new KDOM::DOMStringImpl(KDOM::NS_SVG.string());
}

void SVGDocumentImpl::recalcStyleSelector()
{
    if(!attached())
        return;

    assert(m_pendingStylesheets == 0);

    QString sheetUsed; // Empty sheet

    QPtrList<KDOM::StyleSheetImpl> oldStyleSheets = m_styleSheets->styleSheets;
    m_styleSheets->styleSheets.clear();

    for(int i = 0; i < 2; i++)
    {
        m_availableSheets.clear();
#ifndef APPLE_COMPILE_HACK
        m_availableSheets << i18n("Basic Page Style");
#endif

        for(KDOM::NodeImpl *n = this; n != 0; n = n->traverseNextNode())
        {
            KDOM::StyleSheetImpl *sheet = 0;

            if(n->nodeType() == KDOM::PROCESSING_INSTRUCTION_NODE)
            {
                // Processing instruction (XML documents only)
                KDOM::ProcessingInstructionImpl *pi = static_cast<KDOM::ProcessingInstructionImpl *>(n);
                sheet = pi->sheet();
                if(!sheet && !pi->localHref()->isEmpty())
                {
                    // Processing instruction with reference to an element in this document
                    // - e.g. <?xml-stylesheet href="#mystyle">, with the element
                    // <foo id="mystyle">heading { color: red; }</foo> at some location in the document
                    KDOM::ElementImpl *elem = getElementById(pi->localHref());
                    if(elem)
                    {
                        KDOM::DOMString sheetText("");
                        for(KDOM::NodeImpl *c = elem->firstChild(); c != 0; c = c->nextSibling())
                        {
                            if(c->nodeType() == KDOM::TEXT_NODE ||
                               c->nodeType() == KDOM::CDATA_SECTION_NODE)
                            {
                                sheetText += KDOM::DOMString(c->nodeValue());
                            }
                        }

                        KDOM::CSSStyleSheetImpl *cssSheet = createCSSStyleSheet(this, 0);
                        cssSheet->parseString(sheetText.handle());
                        pi->setStyleSheet(cssSheet);

                        sheet = cssSheet;
                    }
                }
            }
            else
            {
                QString title;

                if(n && n->id() == ID_STYLE)
                {
                    // <STYLE> element
                    SVGStyleElementImpl *s = static_cast<SVGStyleElementImpl*>(n);
                    if(!s->isLoading())
                    {
                        sheet = s->sheet();
                        if(sheet)
                            title = KDOM::DOMString(s->getAttribute(ATTR_TITLE)).string();
                    }
                }

                if(!title.isEmpty() && sheetUsed.isEmpty())
                    sheetUsed = title;

                if(!title.isEmpty())
                {
                    if(title != sheetUsed)
                        sheet = 0; // don't use it

                    title = title.replace('&', QString::fromLatin1("&&"));

                    if(!m_availableSheets.contains(title))
                        m_availableSheets.append(title);
                }
            }

            if(sheet)
            {
                sheet->ref();
                m_styleSheets->styleSheets.append(sheet);
            }
        }

        // we're done if we don't select an alternative sheet or we found the sheet we selected
        if(sheetUsed.isEmpty() || m_availableSheets.contains(sheetUsed))
            break;
    }

    // De-reference all the stylesheets in the old list
    QPtrListIterator<KDOM::StyleSheetImpl> it(oldStyleSheets);
    for(;it.current(); ++it)
        it.current()->deref();

    QString userSheet = m_userSheet;
    if(m_view && m_view->mediaType() == QString::fromLatin1("print"))
        userSheet += m_printSheet;

    // Create a new style selector
    delete m_styleSelector;
    m_styleSelector = createStyleSelector(userSheet);
}

KDOM::CSSStyleSelector *SVGDocumentImpl::createStyleSelector(const QString &usersheet)
{
    return new SVGCSSStyleSelector(this, usersheet, m_styleSheets, m_url, false);
}

KCanvas *SVGDocumentImpl::canvas() const
{
    return m_canvasView ? m_canvasView->canvas() : 0;
}

void SVGDocumentImpl::attach()
{
    if(!canvasView())
        return;

    assert(!m_styleSelector);
    m_styleSelector = createStyleSelector(m_userSheet);

    NodeBaseImpl::attach();
}

void SVGDocumentImpl::addScripts(KDOM::NodeImpl *n)
{
    if(!n)
        return;

    // Recursively go through the entire document tree, looking for html <script> tags.
    // For each of these that is found, add it to the m_scripts list from which they will be executed
    if(n->localId() == ID_SCRIPT)
        m_scripts.append(static_cast<SVGScriptElementImpl *>(n));

    NodeImpl *child;
    for(child = n->firstChild(); child != 0; child = child->nextSibling())
        addScripts(child);
}

void SVGDocumentImpl::executeScripts(bool needsStyleSelectorUpdate)
{
    // Iterate through all of the svg <script> tags in the document. For those that have a src attribute,
    // start loading the script and return (executeScripts() will be called again once the script is loaded
    // and continue where it left off). For scripts that don't have a src attribute, execute the code inside the tag
    SVGScriptElementImpl *script = 0;
    while((script = m_scriptsIt->current()))
    {
        KDOM::DOMString hrefAttr(script->href()->baseVal());
        QString charset; // TODO m_scriptsIt->current()->getAttribute(ATTR_CHARSET).string();

        if(!hrefAttr.isEmpty())
        {
            // we have a src attribute
            KURL fullUrl(documentKURI(), hrefAttr.string());
            m_cachedScript = docLoader()->requestScript(fullUrl, charset);
            ++(*m_scriptsIt);
            m_cachedScript->ref(this); // will call executeScripts() again if already cached
            return;
        }
        else
        {
            // no src attribute - execute from contents of tag
            SVGScriptElementImpl::executeScript(this, KDOM::DOMString(script->textContent()));
            ++(*m_scriptsIt);

            needsStyleSelectorUpdate = true;
        }
    }

    // Fire LOAD_EVENT after all scripts are evaluated
    if(KDOM::DocumentImpl::hasListenerType(KDOM::LOAD_EVENT) && !m_scriptsIt->current())
    {
        SVGEventImpl *event = static_cast<SVGEventImpl *>(createEvent(KDOM::DOMString("SVGEvents").handle()));
        event->ref();

        event->initEvent(KDOM::DOMString("load").handle(), false, false);
        dispatchRecursiveEvent(event, lastChild());

        event->deref();
    }

    // All scripts have finished executing, so calculate the
    // style for the document and close the last element
    if(canvas() && needsStyleSelectorUpdate)
        updateStyleSelector();

    // Start animations, as "load" scripts are executed.
    m_timeScheduler->startAnimations();
}

void SVGDocumentImpl::recalcStyle(StyleChange change)
{
    for(NodeImpl *n = firstChild(); n; n = n->nextSibling())
    {
        if(change >= Inherit || n->hasChangedChild() || n->changed())
            n->recalcStyle(change);
    }
}

KDOM::CSSStyleSheetImpl *SVGDocumentImpl::createCSSStyleSheet(KDOM::NodeImpl *parent, KDOM::DOMStringImpl *url) const
{
    SVGCSSStyleSheetImpl *sheet = new SVGCSSStyleSheetImpl(parent, url);
    sheet->ref();
    return sheet;
}

KDOM::CSSStyleSheetImpl *SVGDocumentImpl::createCSSStyleSheet(KDOM::CSSRuleImpl *ownerRule, KDOM::DOMStringImpl *url) const
{
    SVGCSSStyleSheetImpl *sheet = new SVGCSSStyleSheetImpl(ownerRule, url);
    sheet->ref();
    return sheet;
}

bool SVGDocumentImpl::prepareMouseEvent(bool, int x, int y, KDOM::MouseEventImpl *event)
{
    if(!canvas() || !event)
        return false;

    QPoint coordinate(x, y);
    SVGStyledElementImpl *current = 0;

    KCanvasItemList hits;
    canvas()->collisions(coordinate, hits);

    KCanvasItemList::ConstIterator it = hits.begin();
    KCanvasItemList::ConstIterator end = hits.end();

    // Check for mouseout/focusout events first..
    if(event->id() == KDOM::MOUSEMOVE_EVENT && hits.isEmpty())
    {
        if(m_lastTarget && m_lastTarget != current)
        {
            if(m_lastTarget->hasListenerType(KDOM::DOMFOCUSOUT_EVENT))
                dispatchUIEvent(m_lastTarget, "DOMFocusOut");

            if(m_lastTarget->hasListenerType(KDOM::MOUSEOUT_EVENT))
                dispatchMouseEvent(m_lastTarget, "mouseout");

            m_lastTarget = 0;
        }
    }

    for(; it != end; ++it)
    {
        current = static_cast<SVGStyledElementImpl *>((*it)->userData());

        if(current)
        {
            SVGRenderStyle *style = static_cast<SVGRenderStyle *>(current->renderStyle());
            if(!style || style->pointerEvents() == PE_NONE)
                return false;

            bool isStroked = (style->strokePaint() != 0);
            bool isVisible = (style->visibility() == KDOM::VS_VISIBLE);

            KCanvasItem *canvasItem = current->canvasItem();

            bool testFill = false;
            bool testStroke = false;
            switch(style->pointerEvents())
            {
                case PE_VISIBLE:
                {
                    testFill = isVisible;
                    testStroke = isVisible;
                    break;
                }
                case PE_VISIBLE_PAINTED:
                    testStroke = (isVisible && isStroked);
                case PE_VISIBLE_FILL:
                {
                    testFill = isVisible;
                    break;
                }
                case PE_VISIBLE_STROKE:
                {
                    testStroke = (isVisible && isStroked);
                    break;
                }
                case PE_PAINTED:
                    testStroke = isStroked;
                case PE_FILL:
                {
                    testFill = true;
                    break;
                }
                case PE_STROKE:
                {
                    testStroke = isStroked;
                    break;
                }
                case PE_ALL:
                default:
                {
                    testFill = true;
                    testStroke = true;
                }
            };
            
            if(testFill || testStroke)
            {
                if((testFill && canvasItem->fillContains(coordinate)) ||
                   (testStroke && canvasItem->strokeContains(coordinate)))
                {
                    if(event->id() == KDOM::MOUSEUP_EVENT)
                    {
                        if(current->hasListenerType(KDOM::CLICK_EVENT))
                            dispatchMouseEvent(current, "click");

                        if(current->hasListenerType(KDOM::DOMACTIVATE_EVENT))
                            dispatchUIEvent(current, "DOMActivate");

                        if(current->hasListenerType(KDOM::DOMFOCUSIN_EVENT))
                            dispatchUIEvent(current, "DOMFocusIn");    
                    }
                    else if(event->id() == KDOM::MOUSEMOVE_EVENT)
                    {
                        if(m_lastTarget && m_lastTarget != current)
                        {
                            if(m_lastTarget->hasListenerType(KDOM::DOMFOCUSOUT_EVENT))
                                dispatchUIEvent(m_lastTarget, "DOMFocusOut");

                            if(m_lastTarget->hasListenerType(KDOM::MOUSEOUT_EVENT))
                                dispatchMouseEvent(m_lastTarget, "mouseout");

                            m_lastTarget = 0;
                        }
                        
                        if(current->hasListenerType(KDOM::MOUSEOVER_EVENT))
                        {
                            if(m_lastTarget != current)
                                dispatchMouseEvent(current, "mouseover");
                        }
                    }

                    m_lastTarget = current;

                    event->setRelatedTarget(current);

                    // TODO : investigate hasListenerType
                    // in some cases default actions need
                    // to take place when there are no
                    // listeners, so dispatchEvent has to be
                    // called, that is the problem here...
                    //if(current->hasListenerType(event->id()))
                        current->dispatchEvent(event);

                    return true;
                }
            }
        }
    }
    
    return false;
}

TimeScheduler *SVGDocumentImpl::timeScheduler() const
{
    return m_timeScheduler;
}

void SVGDocumentImpl::dispatchUIEvent(KDOM::EventTargetImpl *target, const KDOM::DOMString &type)
{
    // Setup kdom 'UIEvent'...
    KDOM::UIEventImpl *event = static_cast<KDOM::UIEventImpl *>(createEvent(KDOM::DOMString("UIEvents").handle()));
    event->ref();

    event->initUIEvent(type.handle(), true, true, 0, 0);
    target->dispatchEvent(event);

    event->deref();
}

void SVGDocumentImpl::dispatchMouseEvent(KDOM::EventTargetImpl *target, const KDOM::DOMString &type)
{
    // Setup kdom 'MouseEvent'...
    KDOM::MouseEventImpl *event = static_cast<KDOM::MouseEventImpl *>(createEvent(KDOM::DOMString("MouseEvents").handle()));
    event->ref();

    event->initEvent(type.handle(), true, true);
    target->dispatchEvent(event);

    event->deref();
}

// vim:ts=4:noet
