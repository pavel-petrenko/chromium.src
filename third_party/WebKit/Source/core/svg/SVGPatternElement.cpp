/*
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007 Rob Buis <buis@kde.org>
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
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

#include "core/svg/SVGPatternElement.h"

#include "XLinkNames.h"
#include "core/rendering/svg/RenderSVGResourcePattern.h"
#include "core/svg/PatternAttributes.h"
#include "core/svg/SVGElementInstance.h"
#include "platform/transforms/AffineTransform.h"

namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_ENUMERATION(SVGPatternElement, SVGNames::patternUnitsAttr, PatternUnits, patternUnits, SVGUnitTypes::SVGUnitType)
DEFINE_ANIMATED_ENUMERATION(SVGPatternElement, SVGNames::patternContentUnitsAttr, PatternContentUnits, patternContentUnits, SVGUnitTypes::SVGUnitType)
DEFINE_ANIMATED_TRANSFORM_LIST(SVGPatternElement, SVGNames::patternTransformAttr, PatternTransform, patternTransform)
DEFINE_ANIMATED_STRING(SVGPatternElement, XLinkNames::hrefAttr, Href, href)
DEFINE_ANIMATED_RECT(SVGPatternElement, SVGNames::viewBoxAttr, ViewBox, viewBox)
DEFINE_ANIMATED_PRESERVEASPECTRATIO(SVGPatternElement, SVGNames::preserveAspectRatioAttr, PreserveAspectRatio, preserveAspectRatio)

BEGIN_REGISTER_ANIMATED_PROPERTIES(SVGPatternElement)
    REGISTER_LOCAL_ANIMATED_PROPERTY(patternUnits)
    REGISTER_LOCAL_ANIMATED_PROPERTY(patternContentUnits)
    REGISTER_LOCAL_ANIMATED_PROPERTY(patternTransform)
    REGISTER_LOCAL_ANIMATED_PROPERTY(href)
    REGISTER_LOCAL_ANIMATED_PROPERTY(viewBox)
    REGISTER_LOCAL_ANIMATED_PROPERTY(preserveAspectRatio)
    REGISTER_PARENT_ANIMATED_PROPERTIES(SVGElement)
    REGISTER_PARENT_ANIMATED_PROPERTIES(SVGTests)
END_REGISTER_ANIMATED_PROPERTIES

inline SVGPatternElement::SVGPatternElement(Document& document)
    : SVGElement(SVGNames::patternTag, document)
    , m_x(SVGAnimatedLength::create(this, SVGNames::xAttr, SVGLength::create(LengthModeWidth)))
    , m_y(SVGAnimatedLength::create(this, SVGNames::yAttr, SVGLength::create(LengthModeHeight)))
    , m_width(SVGAnimatedLength::create(this, SVGNames::widthAttr, SVGLength::create(LengthModeWidth)))
    , m_height(SVGAnimatedLength::create(this, SVGNames::heightAttr, SVGLength::create(LengthModeHeight)))
    , m_patternUnits(SVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX)
    , m_patternContentUnits(SVGUnitTypes::SVG_UNIT_TYPE_USERSPACEONUSE)
{
    ScriptWrappable::init(this);

    addToPropertyMap(m_x);
    addToPropertyMap(m_y);
    addToPropertyMap(m_width);
    addToPropertyMap(m_height);
    registerAnimatedPropertiesForSVGPatternElement();
}

PassRefPtr<SVGPatternElement> SVGPatternElement::create(Document& document)
{
    return adoptRef(new SVGPatternElement(document));
}

bool SVGPatternElement::isSupportedAttribute(const QualifiedName& attrName)
{
    DEFINE_STATIC_LOCAL(HashSet<QualifiedName>, supportedAttributes, ());
    if (supportedAttributes.isEmpty()) {
        SVGURIReference::addSupportedAttributes(supportedAttributes);
        SVGTests::addSupportedAttributes(supportedAttributes);
        SVGFitToViewBox::addSupportedAttributes(supportedAttributes);
        supportedAttributes.add(SVGNames::patternUnitsAttr);
        supportedAttributes.add(SVGNames::patternContentUnitsAttr);
        supportedAttributes.add(SVGNames::patternTransformAttr);
        supportedAttributes.add(SVGNames::xAttr);
        supportedAttributes.add(SVGNames::yAttr);
        supportedAttributes.add(SVGNames::widthAttr);
        supportedAttributes.add(SVGNames::heightAttr);
    }
    return supportedAttributes.contains<SVGAttributeHashTranslator>(attrName);
}

void SVGPatternElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    SVGParsingError parseError = NoError;

    if (!isSupportedAttribute(name))
        SVGElement::parseAttribute(name, value);
    else if (name == SVGNames::patternUnitsAttr) {
        SVGUnitTypes::SVGUnitType propertyValue = SVGPropertyTraits<SVGUnitTypes::SVGUnitType>::fromString(value);
        if (propertyValue > 0)
            setPatternUnitsBaseValue(propertyValue);
        return;
    } else if (name == SVGNames::patternContentUnitsAttr) {
        SVGUnitTypes::SVGUnitType propertyValue = SVGPropertyTraits<SVGUnitTypes::SVGUnitType>::fromString(value);
        if (propertyValue > 0)
            setPatternContentUnitsBaseValue(propertyValue);
        return;
    } else if (name == SVGNames::patternTransformAttr) {
        SVGTransformList newList;
        newList.parse(value);
        detachAnimatedPatternTransformListWrappers(newList.size());
        setPatternTransformBaseValue(newList);
        return;
    } else if (name == SVGNames::xAttr)
        m_x->setBaseValueAsString(value, AllowNegativeLengths, parseError);
    else if (name == SVGNames::yAttr)
        m_y->setBaseValueAsString(value, AllowNegativeLengths, parseError);
    else if (name == SVGNames::widthAttr)
        m_width->setBaseValueAsString(value, ForbidNegativeLengths, parseError);
    else if (name == SVGNames::heightAttr)
        m_height->setBaseValueAsString(value, ForbidNegativeLengths, parseError);
    else if (SVGURIReference::parseAttribute(name, value)
             || SVGTests::parseAttribute(name, value)
             || SVGFitToViewBox::parseAttribute(this, name, value)) {
    } else
        ASSERT_NOT_REACHED();

    reportAttributeParsingError(parseError, name, value);
}

void SVGPatternElement::svgAttributeChanged(const QualifiedName& attrName)
{
    if (!isSupportedAttribute(attrName)) {
        SVGElement::svgAttributeChanged(attrName);
        return;
    }

    SVGElementInstance::InvalidationGuard invalidationGuard(this);

    if (attrName == SVGNames::xAttr
        || attrName == SVGNames::yAttr
        || attrName == SVGNames::widthAttr
        || attrName == SVGNames::heightAttr)
        updateRelativeLengthsInformation();

    RenderSVGResourceContainer* renderer = toRenderSVGResourceContainer(this->renderer());
    if (renderer)
        renderer->invalidateCacheAndMarkForLayout();
}

void SVGPatternElement::childrenChanged(bool changedByParser, Node* beforeChange, Node* afterChange, int childCountDelta)
{
    SVGElement::childrenChanged(changedByParser, beforeChange, afterChange, childCountDelta);

    if (changedByParser)
        return;

    if (RenderObject* object = renderer())
        object->setNeedsLayout();
}

RenderObject* SVGPatternElement::createRenderer(RenderStyle*)
{
    return new RenderSVGResourcePattern(this);
}

void SVGPatternElement::collectPatternAttributes(PatternAttributes& attributes) const
{
    HashSet<const SVGPatternElement*> processedPatterns;

    const SVGPatternElement* current = this;
    while (current) {
        if (!attributes.hasX() && current->hasAttribute(SVGNames::xAttr))
            attributes.setX(current->x()->currentValue());

        if (!attributes.hasY() && current->hasAttribute(SVGNames::yAttr))
            attributes.setY(current->y()->currentValue());

        if (!attributes.hasWidth() && current->hasAttribute(SVGNames::widthAttr))
            attributes.setWidth(current->width()->currentValue());

        if (!attributes.hasHeight() && current->hasAttribute(SVGNames::heightAttr))
            attributes.setHeight(current->height()->currentValue());

        if (!attributes.hasViewBox() && current->hasAttribute(SVGNames::viewBoxAttr) && current->viewBoxCurrentValue().isValid())
            attributes.setViewBox(current->viewBoxCurrentValue());

        if (!attributes.hasPreserveAspectRatio() && current->hasAttribute(SVGNames::preserveAspectRatioAttr))
            attributes.setPreserveAspectRatio(current->preserveAspectRatioCurrentValue());

        if (!attributes.hasPatternUnits() && current->hasAttribute(SVGNames::patternUnitsAttr))
            attributes.setPatternUnits(current->patternUnitsCurrentValue());

        if (!attributes.hasPatternContentUnits() && current->hasAttribute(SVGNames::patternContentUnitsAttr))
            attributes.setPatternContentUnits(current->patternContentUnitsCurrentValue());

        if (!attributes.hasPatternTransform() && current->hasAttribute(SVGNames::patternTransformAttr)) {
            AffineTransform transform;
            current->patternTransformCurrentValue().concatenate(transform);
            attributes.setPatternTransform(transform);
        }

        if (!attributes.hasPatternContentElement() && current->childElementCount())
            attributes.setPatternContentElement(current);

        processedPatterns.add(current);

        // Respect xlink:href, take attributes from referenced element
        Node* refNode = SVGURIReference::targetElementFromIRIString(current->hrefCurrentValue(), document());
        if (refNode && refNode->hasTagName(SVGNames::patternTag)) {
            current = toSVGPatternElement(const_cast<const Node*>(refNode));

            // Cycle detection
            if (processedPatterns.contains(current)) {
                current = 0;
                break;
            }
        } else
            current = 0;
    }
}

AffineTransform SVGPatternElement::localCoordinateSpaceTransform(SVGElement::CTMScope) const
{
    AffineTransform matrix;
    patternTransformCurrentValue().concatenate(matrix);
    return matrix;
}

bool SVGPatternElement::selfHasRelativeLengths() const
{
    return m_x->currentValue()->isRelative()
        || m_y->currentValue()->isRelative()
        || m_width->currentValue()->isRelative()
        || m_height->currentValue()->isRelative();
}

}
