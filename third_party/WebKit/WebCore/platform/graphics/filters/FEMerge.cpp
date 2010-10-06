/*
 * Copyright (C) 2004, 2005, 2006, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
 * Copyright (C) 2005 Eric Seidel <eric@webkit.org>
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

#if ENABLE(FILTERS)
#include "FEMerge.h"

#include "Filter.h"
#include "GraphicsContext.h"

namespace WebCore {

FEMerge::FEMerge() 
    : FilterEffect()
{
}

PassRefPtr<FEMerge> FEMerge::create()
{
    return adoptRef(new FEMerge);
}

void FEMerge::apply(Filter* filter)
{
    unsigned size = numberOfEffectInputs();
    ASSERT(size > 0);
    for (unsigned i = 0; i < size; ++i) {
        FilterEffect* in = inputEffect(i);
        in->apply(filter);
        if (!in->resultImage())
            return;
    }

    GraphicsContext* filterContext = effectContext(filter);
    if (!filterContext)
        return;

    for (unsigned i = 0; i < size; ++i) {
        FilterEffect* in = inputEffect(i);
        filterContext->drawImageBuffer(in->resultImage(), DeviceColorSpace, drawingRegionOfInputImage(in->absolutePaintRect()));
    }
}

void FEMerge::dump()
{
}

TextStream& FEMerge::externalRepresentation(TextStream& ts, int indent) const
{
    writeIndent(ts, indent);
    ts << "[feMerge";
    FilterEffect::externalRepresentation(ts);
    unsigned size = numberOfEffectInputs();
    ASSERT(size > 0);
    ts << " mergeNodes=\"" << size << "\"]\n";
    for (unsigned i = 0; i < size; ++i)
        inputEffect(i)->externalRepresentation(ts, indent + 1);
    return ts;
}

} // namespace WebCore

#endif // ENABLE(FILTERS)
