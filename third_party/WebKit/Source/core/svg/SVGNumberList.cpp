/*
 * Copyright (C) 2004, 2005, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006 Rob Buis <buis@kde.org>
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

#include "core/svg/SVGNumberList.h"

#include "core/svg/SVGParserUtilities.h"
#include <wtf/text/StringBuilder.h>

namespace WebCore {

template<typename CharType>
void SVGNumberList::parseInternal(const CharType*& ptr, const CharType* end)
{
    // The spec strangely doesn't allow leading whitespace.  We might choose to violate that intentionally. (section 4.1)
    while (ptr < end) {
        float number = 0;
        if (!parseNumber(ptr, end, number))
            return;
        append(number);
    }
}

void SVGNumberList::parse(const String& value)
{
    clear();
    if (value.isEmpty())
        return;
    if (value.is8Bit()) {
        const LChar* ptr = value.characters8();
        const LChar* end = ptr + value.length();
        parseInternal(ptr, end);
    } else {
        const UChar* ptr = value.characters16();
        const UChar* end = ptr + value.length();
        parseInternal(ptr, end);
    }
}

String SVGNumberList::valueAsString() const
{
    StringBuilder builder;

    unsigned size = this->size();
    for (unsigned i = 0; i < size; ++i) {
        if (i > 0)
            builder.append(' ');

        builder.append(String::number(at(i)));
    }

    return builder.toString();
}

}
