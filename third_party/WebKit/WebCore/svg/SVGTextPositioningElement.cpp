/*
    Copyright (C) 2004, 2005, 2008 Nikolas Zimmermann <zimmermann@kde.org>
                  2004, 2005, 2006, 2007, 2008 Rob Buis <buis@kde.org>

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
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"

#if ENABLE(SVG)
#include "SVGTextPositioningElement.h"

#include "SVGLengthList.h"
#include "SVGNames.h"
#include "SVGNumberList.h"

namespace WebCore {

SVGTextPositioningElement::SVGTextPositioningElement(const QualifiedName& tagName, Document* doc)
    : SVGTextContentElement(tagName, doc)
    , m_x(SVGLengthList::create(SVGNames::xAttr))
    , m_y(SVGLengthList::create(SVGNames::yAttr))
    , m_dx(SVGLengthList::create(SVGNames::dxAttr))
    , m_dy(SVGLengthList::create(SVGNames::dyAttr))
    , m_rotate(SVGNumberList::create(SVGNames::rotateAttr))
{
}

SVGTextPositioningElement::~SVGTextPositioningElement()
{
}

ANIMATED_PROPERTY_DEFINITIONS_REFCOUNTED(SVGTextPositioningElement, SVGLengthList, X, x, SVGNames::xAttr)
ANIMATED_PROPERTY_DEFINITIONS_REFCOUNTED(SVGTextPositioningElement, SVGLengthList, Y, y, SVGNames::yAttr)
ANIMATED_PROPERTY_DEFINITIONS_REFCOUNTED(SVGTextPositioningElement, SVGLengthList, Dx, dx, SVGNames::dxAttr)
ANIMATED_PROPERTY_DEFINITIONS_REFCOUNTED(SVGTextPositioningElement, SVGLengthList, Dy, dy, SVGNames::dyAttr)
ANIMATED_PROPERTY_DEFINITIONS_REFCOUNTED(SVGTextPositioningElement, SVGNumberList, Rotate, rotate, SVGNames::rotateAttr)

void SVGTextPositioningElement::parseMappedAttribute(MappedAttribute* attr)
{
    if (attr->name() == SVGNames::xAttr)
        xBaseValue()->parse(attr->value(), LengthModeWidth);
    else if (attr->name() == SVGNames::yAttr)
        yBaseValue()->parse(attr->value(), LengthModeHeight);
    else if (attr->name() == SVGNames::dxAttr)
        dxBaseValue()->parse(attr->value(), LengthModeWidth);
    else if (attr->name() == SVGNames::dyAttr)
        dyBaseValue()->parse(attr->value(), LengthModeHeight);
    else if (attr->name() == SVGNames::rotateAttr)
        rotateBaseValue()->parse(attr->value());
    else
        SVGTextContentElement::parseMappedAttribute(attr);
}

bool SVGTextPositioningElement::isKnownAttribute(const QualifiedName& attrName)
{
    return (attrName.matches(SVGNames::xAttr) ||
            attrName.matches(SVGNames::yAttr) ||
            attrName.matches(SVGNames::dxAttr) ||
            attrName.matches(SVGNames::dyAttr) ||
            attrName.matches(SVGNames::rotateAttr) ||
            SVGTextContentElement::isKnownAttribute(attrName));
}

}

#endif // ENABLE(SVG)
