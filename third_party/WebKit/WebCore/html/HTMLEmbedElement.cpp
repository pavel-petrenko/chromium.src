/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Stefan Schimanski (1Stein@gmx.de)
 * Copyright (C) 2004, 2005, 2006, 2008, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
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
#include "HTMLEmbedElement.h"

#include "Attribute.h"
#include "CSSHelper.h"
#include "CSSPropertyNames.h"
#include "Frame.h"
#include "HTMLDocument.h"
#include "HTMLImageLoader.h"
#include "HTMLNames.h"
#include "HTMLObjectElement.h"
#include "RenderEmbeddedObject.h"
#include "RenderImage.h"
#include "RenderWidget.h"
#include "ScriptController.h"
#include "Settings.h"

namespace WebCore {

using namespace HTMLNames;

inline HTMLEmbedElement::HTMLEmbedElement(const QualifiedName& tagName, Document* document, bool createdByParser)
    : HTMLPlugInImageElement(tagName, document, createdByParser)
{
    ASSERT(hasTagName(embedTag));
}

PassRefPtr<HTMLEmbedElement> HTMLEmbedElement::create(const QualifiedName& tagName, Document* document, bool createdByParser)
{
    return adoptRef(new HTMLEmbedElement(tagName, document, createdByParser));
}

static inline RenderWidget* findWidgetRenderer(const Node* n) 
{
    if (!n->renderer())
        do
            n = n->parentNode();
        while (n && !n->hasTagName(objectTag));

    if (n && n->renderer() && n->renderer()->isWidget())
        return toRenderWidget(n->renderer());

    return 0;
}

RenderWidget* HTMLEmbedElement::renderWidgetForJSBindings() const
{
    document()->updateLayoutIgnorePendingStylesheets();
    return findWidgetRenderer(this);
}

bool HTMLEmbedElement::mapToEntry(const QualifiedName& attrName, MappedAttributeEntry& result) const
{
    if (attrName == hiddenAttr) {
        result = eUniversal;
        return false;
    }
        
    return HTMLPlugInImageElement::mapToEntry(attrName, result);
}

void HTMLEmbedElement::parseMappedAttribute(Attribute* attr)
{
    const AtomicString& value = attr->value();
  
    if (attr->name() == typeAttr) {
        m_serviceType = value.string().lower();
        size_t pos = m_serviceType.find(";");
        if (pos != notFound)
            m_serviceType = m_serviceType.left(pos);
        if (!isImageType() && m_imageLoader)
            m_imageLoader.clear();
    } else if (attr->name() == codeAttr)
        m_url = deprecatedParseURL(value.string());
    else if (attr->name() == srcAttr) {
        m_url = deprecatedParseURL(value.string());
        if (renderer() && isImageType()) {
            if (!m_imageLoader)
                m_imageLoader = adoptPtr(new HTMLImageLoader(this));
            m_imageLoader->updateFromElementIgnoringPreviousError();
        }
    } else if (attr->name() == hiddenAttr) {
        if (equalIgnoringCase(value.string(), "yes") || equalIgnoringCase(value.string(), "true")) {
            // FIXME: Not dynamic, since we add this but don't remove it, but it may be OK for now
            // that this rarely-used attribute won't work properly if you remove it.
            addCSSLength(attr, CSSPropertyWidth, "0");
            addCSSLength(attr, CSSPropertyHeight, "0");
        }
    } else if (attr->name() == nameAttr) {
        if (inDocument() && document()->isHTMLDocument()) {
            HTMLDocument* document = static_cast<HTMLDocument*>(this->document());
            document->removeNamedItem(m_name);
            document->addNamedItem(value);
        }
        m_name = value;
    } else
        HTMLPlugInImageElement::parseMappedAttribute(attr);
}

void HTMLEmbedElement::parametersForPlugin(Vector<String>& paramNames, Vector<String>& paramValues)
{
    NamedNodeMap* attributes = this->attributes(true);
    if (!attributes)
        return;

    for (unsigned i = 0; i < attributes->length(); ++i) {
        Attribute* it = attributes->attributeItem(i);
        paramNames.append(it->localName());
        paramValues.append(it->value());
    }
}

// FIXME: This should be unified with HTMLObjectElement::updateWidget and
// moved down into HTMLPluginImageElement.cpp
void HTMLEmbedElement::updateWidget(bool onlyCreateNonNetscapePlugins)
{
    ASSERT(!renderEmbeddedObject()->pluginCrashedOrWasMissing());
    // FIXME: We should ASSERT(needsWidgetUpdate()), but currently
    // FrameView::updateWidget() calls updateWidget(false) without checking if
    // the widget actually needs updating!
    setNeedsWidgetUpdate(false);

    if (m_url.isEmpty() && m_serviceType.isEmpty())
        return;

    // Note these pass m_url and m_serviceType to allow better code sharing with
    // <object> which modifies url and serviceType before calling these.
    if (!allowedToLoadFrameURL(m_url))
        return;
    if (onlyCreateNonNetscapePlugins && wouldLoadAsNetscapePlugin(m_url, m_serviceType))
        return;

    // FIXME: These should be joined into a PluginParameters class.
    Vector<String> paramNames;
    Vector<String> paramValues;
    parametersForPlugin(paramNames, paramValues);

    if (!dispatchBeforeLoadEvent(m_url))
        return;

    SubframeLoader* loader = document()->frame()->loader()->subframeLoader();
    // FIXME: beforeLoad could have detached the renderer!  Just like in the <object> case above.
    loader->requestObject(this, m_url, getAttribute(nameAttr), m_serviceType, paramNames, paramValues);
}

bool HTMLEmbedElement::rendererIsNeeded(RenderStyle* style)
{
    if (isImageType())
        return HTMLPlugInImageElement::rendererIsNeeded(style);

    Frame* frame = document()->frame();
    if (!frame)
        return false;

    // If my parent is an <object> and is not set to use fallback content, I
    // should be ignored and not get a renderer.
    Node* p = parentNode();
    if (p && p->hasTagName(objectTag)) {
        ASSERT(p->renderer());
        if (!static_cast<HTMLObjectElement*>(p)->useFallbackContent()) {
            ASSERT(!p->renderer()->isEmbeddedObject());
            return false;
        }
    }

#if ENABLE(DASHBOARD_SUPPORT)
    // Workaround for <rdar://problem/6642221>. 
    if (Settings* settings = frame->settings()) {
        if (settings->usesDashboardBackwardCompatibilityMode())
            return true;
    }
#endif

    return HTMLPlugInImageElement::rendererIsNeeded(style);
}

void HTMLEmbedElement::insertedIntoDocument()
{
    if (document()->isHTMLDocument())
        static_cast<HTMLDocument*>(document())->addNamedItem(m_name);

    String width = getAttribute(widthAttr);
    String height = getAttribute(heightAttr);
    if (!width.isEmpty() || !height.isEmpty()) {
        Node* n = parent();
        while (n && !n->hasTagName(objectTag))
            n = n->parent();
        if (n) {
            if (!width.isEmpty())
                static_cast<HTMLObjectElement*>(n)->setAttribute(widthAttr, width);
            if (!height.isEmpty())
                static_cast<HTMLObjectElement*>(n)->setAttribute(heightAttr, height);
        }
    }

    HTMLPlugInImageElement::insertedIntoDocument();
}

void HTMLEmbedElement::removedFromDocument()
{
    if (document()->isHTMLDocument())
        static_cast<HTMLDocument*>(document())->removeNamedItem(m_name);

    HTMLPlugInImageElement::removedFromDocument();
}

void HTMLEmbedElement::attributeChanged(Attribute* attr, bool preserveDecls)
{
    HTMLPlugInImageElement::attributeChanged(attr, preserveDecls);

    if ((attr->name() == widthAttr || attr->name() == heightAttr) && !attr->isEmpty()) {
        Node* n = parent();
        while (n && !n->hasTagName(objectTag))
            n = n->parent();
        if (n)
            static_cast<HTMLObjectElement*>(n)->setAttribute(attr->name(), attr->value());
    }
}

bool HTMLEmbedElement::isURLAttribute(Attribute* attr) const
{
    return attr->name() == srcAttr;
}

const QualifiedName& HTMLEmbedElement::imageSourceAttributeName() const
{
    return srcAttr;
}

void HTMLEmbedElement::addSubresourceAttributeURLs(ListHashSet<KURL>& urls) const
{
    HTMLPlugInImageElement::addSubresourceAttributeURLs(urls);

    addSubresourceURL(urls, document()->completeURL(getAttribute(srcAttr)));
}

}
