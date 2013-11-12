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

#include "SVGNames.h"
#include "XLinkNames.h"
#include "core/rendering/svg/RenderSVGResourcePattern.h"
#include "core/svg/PatternAttributes.h"
#include "core/svg/SVGElementInstance.h"
#include "core/svg/SVGFitToViewBox.h"
#include "platform/transforms/AffineTransform.h"

namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_LENGTH(SVGPatternElement, SVGNames::xAttr, X, x)
DEFINE_ANIMATED_LENGTH(SVGPatternElement, SVGNames::yAttr, Y, y)
DEFINE_ANIMATED_LENGTH(SVGPatternElement, SVGNames::widthAttr, Width, width)
DEFINE_ANIMATED_LENGTH(SVGPatternElement, SVGNames::heightAttr, Height, height)
DEFINE_ANIMATED_ENUMERATION(SVGPatternElement, SVGNames::patternUnitsAttr, PatternUnits, patternUnits, SVGUnitTypes::SVGUnitType)
DEFINE_ANIMATED_ENUMERATION(SVGPatternElement, SVGNames::patternContentUnitsAttr, PatternContentUnits, patternContentUnits, SVGUnitTypes::SVGUnitType)
DEFINE_ANIMATED_TRANSFORM_LIST(SVGPatternElement, SVGNames::patternTransformAttr, PatternTransform, patternTransform)
DEFINE_ANIMATED_STRING(SVGPatternElement, XLinkNames::hrefAttr, Href, href)
DEFINE_ANIMATED_BOOLEAN(SVGPatternElement, SVGNames::externalResourcesRequiredAttr, ExternalResourcesRequired, externalResourcesRequired)
DEFINE_ANIMATED_RECT(SVGPatternElement, SVGNames::viewBoxAttr, ViewBox, viewBox)
DEFINE_ANIMATED_PRESERVEASPECTRATIO(SVGPatternElement, SVGNames::preserveAspectRatioAttr, PreserveAspectRatio, preserveAspectRatio)

BEGIN_REGISTER_ANIMATED_PROPERTIES(SVGPatternElement)
    REGISTER_LOCAL_ANIMATED_PROPERTY(x)
    REGISTER_LOCAL_ANIMATED_PROPERTY(y)
    REGISTER_LOCAL_ANIMATED_PROPERTY(width)
    REGISTER_LOCAL_ANIMATED_PROPERTY(height)
    REGISTER_LOCAL_ANIMATED_PROPERTY(patternUnits)
    REGISTER_LOCAL_ANIMATED_PROPERTY(patternContentUnits)
    REGISTER_LOCAL_ANIMATED_PROPERTY(patternTransform)
    REGISTER_LOCAL_ANIMATED_PROPERTY(href)
    REGISTER_LOCAL_ANIMATED_PROPERTY(externalResourcesRequired)
    REGISTER_LOCAL_ANIMATED_PROPERTY(viewBox)
    REGISTER_LOCAL_ANIMATED_PROPERTY(preserveAspectRatio)
    REGISTER_PARENT_ANIMATED_PROPERTIES(SVGElement)
    REGISTER_PARENT_ANIMATED_PROPERTIES(SVGTests)
END_REGISTER_ANIMATED_PROPERTIES

inline SVGPatternElement::SVGPatternElement(const QualifiedName& tagName, Document& document)
    : SVGElement(tagName, document)
    , m_x(LengthModeWidth)
    , m_y(LengthModeHeight)
    , m_width(LengthModeWidth)
    , m_height(LengthModeHeight)
    , m_patternUnits(SVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX)
    , m_patternContentUnits(SVGUnitTypes::SVG_UNIT_TYPE_USERSPACEONUSE)
{
    ASSERT(hasTagName(SVGNames::patternTag));
    ScriptWrappable::init(this);
    registerAnimatedPropertiesForSVGPatternElement();
}

PassRefPtr<SVGPatternElement> SVGPatternElement::create(const QualifiedName& tagName, Document& document)
{
    return adoptRef(new SVGPatternElement(tagName, document));
}

bool SVGPatternElement::isSupportedAttribute(const QualifiedName& attrName)
{
    DEFINE_STATIC_LOCAL(HashSet<QualifiedName>, supportedAttributes, ());
    if (supportedAttributes.isEmpty()) {
        SVGURIReference::addSupportedAttributes(supportedAttributes);
        SVGTests::addSupportedAttributes(supportedAttributes);
        SVGExternalResourcesRequired::addSupportedAttributes(supportedAttributes);
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
        setXBaseValue(SVGLength::construct(LengthModeWidth, value, parseError));
    else if (name == SVGNames::yAttr)
        setYBaseValue(SVGLength::construct(LengthModeHeight, value, parseError));
    else if (name == SVGNames::widthAttr)
        setWidthBaseValue(SVGLength::construct(LengthModeWidth, value, parseError, ForbidNegativeLengths));
    else if (name == SVGNames::heightAttr)
        setHeightBaseValue(SVGLength::construct(LengthModeHeight, value, parseError, ForbidNegativeLengths));
    else if (SVGURIReference::parseAttribute(name, value)
             || SVGTests::parseAttribute(name, value)
             || SVGExternalResourcesRequired::parseAttribute(name, value)
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
            attributes.setX(current->xCurrentValue());

        if (!attributes.hasY() && current->hasAttribute(SVGNames::yAttr))
            attributes.setY(current->yCurrentValue());

        if (!attributes.hasWidth() && current->hasAttribute(SVGNames::widthAttr))
            attributes.setWidth(current->widthCurrentValue());

        if (!attributes.hasHeight() && current->hasAttribute(SVGNames::heightAttr))
            attributes.setHeight(current->heightCurrentValue());

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
            current = static_cast<const SVGPatternElement*>(const_cast<const Node*>(refNode));

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
    return xCurrentValue().isRelative()
        || yCurrentValue().isRelative()
        || widthCurrentValue().isRelative()
        || heightCurrentValue().isRelative();
}

}
