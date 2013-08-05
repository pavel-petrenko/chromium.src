/*
 * Copyright (C) 2004, 2005, 2007, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006 Rob Buis <buis@kde.org>
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
#include "core/svg/SVGAngle.h"

#include "bindings/v8/ExceptionState.h"
#include "core/dom/ExceptionCode.h"
#include "core/svg/SVGParserUtilities.h"
#include "wtf/MathExtras.h"
#include "wtf/text/WTFString.h"

namespace WebCore {

SVGAngle::SVGAngle()
    : m_unitType(SVG_ANGLETYPE_UNSPECIFIED)
    , m_valueInSpecifiedUnits(0)
{
}

float SVGAngle::value() const
{
    switch (m_unitType) {
    case SVG_ANGLETYPE_GRAD:
        return grad2deg(m_valueInSpecifiedUnits);
    case SVG_ANGLETYPE_RAD:
        return rad2deg(m_valueInSpecifiedUnits);
    case SVG_ANGLETYPE_UNSPECIFIED:
    case SVG_ANGLETYPE_UNKNOWN:
    case SVG_ANGLETYPE_DEG:
        return m_valueInSpecifiedUnits;
    }

    ASSERT_NOT_REACHED();
    return 0;
}

void SVGAngle::setValue(float value)
{
    switch (m_unitType) {
    case SVG_ANGLETYPE_GRAD:
        m_valueInSpecifiedUnits = deg2grad(value);
        break;
    case SVG_ANGLETYPE_RAD:
        m_valueInSpecifiedUnits = deg2rad(value);
        break;
    case SVG_ANGLETYPE_UNSPECIFIED:
    case SVG_ANGLETYPE_UNKNOWN:
    case SVG_ANGLETYPE_DEG:
        m_valueInSpecifiedUnits = value;
        break;
    }
}

template<typename CharType>
static SVGAngle::SVGAngleType stringToAngleType(const CharType*& ptr, const CharType* end)
{
    // If there's no unit given, the angle type is unspecified.
    if (ptr == end)
        return SVGAngle::SVG_ANGLETYPE_UNSPECIFIED;

    const CharType firstChar = *ptr;

    // If the unit contains only one character, the angle type is unknown.
    ++ptr;
    if (ptr == end)
        return SVGAngle::SVG_ANGLETYPE_UNKNOWN;

    const CharType secondChar = *ptr;

    // If the unit contains only two characters, the angle type is unknown.
    ++ptr;
    if (ptr == end)
        return SVGAngle::SVG_ANGLETYPE_UNKNOWN;

    const CharType thirdChar = *ptr;
    if (firstChar == 'd' && secondChar == 'e' && thirdChar == 'g')
        return SVGAngle::SVG_ANGLETYPE_DEG;
    if (firstChar == 'r' && secondChar == 'a' && thirdChar == 'd')
        return SVGAngle::SVG_ANGLETYPE_RAD;

    // If the unit contains three characters, but is not deg or rad, then it's unknown.
    ++ptr;
    if (ptr == end)
        return SVGAngle::SVG_ANGLETYPE_UNKNOWN;

    const CharType fourthChar = *ptr;

    if (firstChar == 'g' && secondChar == 'r' && thirdChar == 'a' && fourthChar == 'd')
        return SVGAngle::SVG_ANGLETYPE_GRAD;

    return SVGAngle::SVG_ANGLETYPE_UNKNOWN;
}

String SVGAngle::valueAsString() const
{
    switch (m_unitType) {
    case SVG_ANGLETYPE_DEG: {
        DEFINE_STATIC_LOCAL(String, degString, (ASCIILiteral("deg")));
        return String::number(m_valueInSpecifiedUnits) + degString;
    }
    case SVG_ANGLETYPE_RAD: {
        DEFINE_STATIC_LOCAL(String, radString, (ASCIILiteral("rad")));
        return String::number(m_valueInSpecifiedUnits) + radString;
    }
    case SVG_ANGLETYPE_GRAD: {
        DEFINE_STATIC_LOCAL(String, gradString, (ASCIILiteral("grad")));
        return String::number(m_valueInSpecifiedUnits) + gradString;
    }
    case SVG_ANGLETYPE_UNSPECIFIED:
    case SVG_ANGLETYPE_UNKNOWN:
        return String::number(m_valueInSpecifiedUnits);
    }

    ASSERT_NOT_REACHED();
    return String();
}

template<typename CharType>
static bool parseValue(const String& value, float& valueInSpecifiedUnits, SVGAngle::SVGAngleType& unitType)
{
    const CharType* ptr = value.getCharacters<CharType>();
    const CharType* end = ptr + value.length();

    if (!parseNumber(ptr, end, valueInSpecifiedUnits, false))
        return false;

    unitType = stringToAngleType(ptr, end);
    if (unitType == SVGAngle::SVG_ANGLETYPE_UNKNOWN)
        return false;

    return true;
}

void SVGAngle::setValueAsString(const String& value, ExceptionState& es)
{
    if (value.isEmpty()) {
        m_unitType = SVG_ANGLETYPE_UNSPECIFIED;
        return;
    }

    float valueInSpecifiedUnits = 0;
    SVGAngleType unitType = SVG_ANGLETYPE_UNKNOWN;

    bool success = value.is8Bit() ? parseValue<LChar>(value, valueInSpecifiedUnits, unitType)
                                  : parseValue<UChar>(value, valueInSpecifiedUnits, unitType);
    if (!success) {
        es.throwDOMException(SyntaxError);
        return;
    }

    m_unitType = unitType;
    m_valueInSpecifiedUnits = valueInSpecifiedUnits;
}

void SVGAngle::newValueSpecifiedUnits(unsigned short unitType, float valueInSpecifiedUnits, ExceptionState& es)
{
    if (unitType == SVG_ANGLETYPE_UNKNOWN || unitType > SVG_ANGLETYPE_GRAD) {
        es.throwDOMException(NotSupportedError);
        return;
    }

    if (unitType != m_unitType)
        m_unitType = static_cast<SVGAngleType>(unitType);

    m_valueInSpecifiedUnits = valueInSpecifiedUnits;
}

void SVGAngle::convertToSpecifiedUnits(unsigned short unitType, ExceptionState& es)
{
    if (unitType == SVG_ANGLETYPE_UNKNOWN || m_unitType == SVG_ANGLETYPE_UNKNOWN || unitType > SVG_ANGLETYPE_GRAD) {
        es.throwDOMException(NotSupportedError);
        return;
    }

    if (unitType == m_unitType)
        return;

    switch (m_unitType) {
    case SVG_ANGLETYPE_RAD:
        switch (unitType) {
        case SVG_ANGLETYPE_GRAD:
            m_valueInSpecifiedUnits = rad2grad(m_valueInSpecifiedUnits);
            break;
        case SVG_ANGLETYPE_UNSPECIFIED:
        case SVG_ANGLETYPE_DEG:
            m_valueInSpecifiedUnits = rad2deg(m_valueInSpecifiedUnits);
            break;
        case SVG_ANGLETYPE_RAD:
        case SVG_ANGLETYPE_UNKNOWN:
            ASSERT_NOT_REACHED();
            break;
        }
        break;
    case SVG_ANGLETYPE_GRAD:
        switch (unitType) {
        case SVG_ANGLETYPE_RAD:
            m_valueInSpecifiedUnits = grad2rad(m_valueInSpecifiedUnits);
            break;
        case SVG_ANGLETYPE_UNSPECIFIED:
        case SVG_ANGLETYPE_DEG:
            m_valueInSpecifiedUnits = grad2deg(m_valueInSpecifiedUnits);
            break;
        case SVG_ANGLETYPE_GRAD:
        case SVG_ANGLETYPE_UNKNOWN:
            ASSERT_NOT_REACHED();
            break;
        }
        break;
    case SVG_ANGLETYPE_UNSPECIFIED:
        // Spec: For angles, a unitless value is treated the same as if degrees were specified.
    case SVG_ANGLETYPE_DEG:
        switch (unitType) {
        case SVG_ANGLETYPE_RAD:
            m_valueInSpecifiedUnits = deg2rad(m_valueInSpecifiedUnits);
            break;
        case SVG_ANGLETYPE_GRAD:
            m_valueInSpecifiedUnits = deg2grad(m_valueInSpecifiedUnits);
            break;
        case SVG_ANGLETYPE_UNSPECIFIED:
            break;
        case SVG_ANGLETYPE_DEG:
        case SVG_ANGLETYPE_UNKNOWN:
            ASSERT_NOT_REACHED();
            break;
        }
        break;
    case SVG_ANGLETYPE_UNKNOWN:
        ASSERT_NOT_REACHED();
        break;
    }

    m_unitType = static_cast<SVGAngleType>(unitType);
}

}
