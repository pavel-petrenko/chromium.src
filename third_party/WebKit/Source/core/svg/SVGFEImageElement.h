/*
 * Copyright (C) 2004, 2005, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
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

#ifndef SVGFEImageElement_h
#define SVGFEImageElement_h

#include "SVGNames.h"
#include "core/fetch/ImageResource.h"
#include "core/fetch/ResourcePtr.h"
#include "core/platform/graphics/ImageBuffer.h"
#include "core/svg/SVGAnimatedBoolean.h"
#include "core/svg/SVGAnimatedPreserveAspectRatio.h"
#include "core/svg/SVGExternalResourcesRequired.h"
#include "core/svg/SVGFilterPrimitiveStandardAttributes.h"
#include "core/svg/SVGURIReference.h"
#include "core/svg/graphics/filters/SVGFEImage.h"

namespace WebCore {

class SVGFEImageElement FINAL : public SVGFilterPrimitiveStandardAttributes,
                                public SVGURIReference,
                                public SVGExternalResourcesRequired,
                                public ImageResourceClient {
public:
    static PassRefPtr<SVGFEImageElement> create(const QualifiedName&, Document&);

    bool currentFrameHasSingleSecurityOrigin() const;

    virtual ~SVGFEImageElement();

private:
    SVGFEImageElement(const QualifiedName&, Document&);

    bool isSupportedAttribute(const QualifiedName&);
    virtual void parseAttribute(const QualifiedName&, const AtomicString&) OVERRIDE;
    virtual void svgAttributeChanged(const QualifiedName&);
    virtual void notifyFinished(Resource*);

    virtual void addSubresourceAttributeURLs(ListHashSet<KURL>&) const;
    virtual PassRefPtr<FilterEffect> build(SVGFilterBuilder*, Filter*);

    void clearResourceReferences();
    void fetchImageResource();

    virtual void buildPendingResource();
    virtual InsertionNotificationRequest insertedInto(ContainerNode*) OVERRIDE;
    virtual void removedFrom(ContainerNode*) OVERRIDE;

    BEGIN_DECLARE_ANIMATED_PROPERTIES(SVGFEImageElement)
        DECLARE_ANIMATED_PRESERVEASPECTRATIO(PreserveAspectRatio, preserveAspectRatio)
        DECLARE_ANIMATED_STRING(Href, href)
        DECLARE_ANIMATED_BOOLEAN(ExternalResourcesRequired, externalResourcesRequired)
    END_DECLARE_ANIMATED_PROPERTIES

    ResourcePtr<ImageResource> m_cachedImage;
};

inline SVGFEImageElement* toSVGFEImageElement(Node* node)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!node || node->hasTagName(SVGNames::feImageTag));
    return static_cast<SVGFEImageElement*>(node);
}

} // namespace WebCore

#endif
