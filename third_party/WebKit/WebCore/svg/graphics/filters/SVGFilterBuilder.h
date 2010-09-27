/*
 * Copyright (C) 2008 Alex Mathews <possessedpenguinbob@gmail.com>
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

#ifndef SVGFilterBuilder_h
#define SVGFilterBuilder_h

#include "config.h"

#if ENABLE(SVG) && ENABLE(FILTERS)
#include "FilterEffect.h"
#include "PlatformString.h"

#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/PassRefPtr.h>
#include <wtf/text/AtomicStringHash.h>

namespace WebCore {
    
    class SVGFilterBuilder : public RefCounted<SVGFilterBuilder> {
    public:
        typedef HashSet<FilterEffect*> FilterEffectSet;

        static PassRefPtr<SVGFilterBuilder> create() { return adoptRef(new SVGFilterBuilder); }

        void add(const AtomicString& id, RefPtr<FilterEffect> effect);

        FilterEffect* getEffectById(const AtomicString& id) const;
        FilterEffect* lastEffect() const { return m_lastEffect.get(); }

        void appendEffectToEffectReferences(RefPtr<FilterEffect>);

        inline FilterEffectSet& getEffectReferences(FilterEffect* effect)
        {
            // Only allowed for effects belongs to this builder.
            ASSERT(m_effectReferences.contains(effect));
            return m_effectReferences.find(effect)->second;
        }

        void clearEffects();

    private:
        SVGFilterBuilder();

        inline void addBuiltinEffects()
        {
            HashMap<AtomicString, RefPtr<FilterEffect> >::iterator end = m_builtinEffects.end();
            for (HashMap<AtomicString, RefPtr<FilterEffect> >::iterator iterator = m_builtinEffects.begin(); iterator != end; ++iterator)
                 m_effectReferences.add(iterator->second, FilterEffectSet());
        }

        HashMap<AtomicString, RefPtr<FilterEffect> > m_builtinEffects;
        HashMap<AtomicString, RefPtr<FilterEffect> > m_namedEffects;
        // The value is a list, which contains those filter effects,
        // which depends on the key filter effect.
        HashMap<RefPtr<FilterEffect>, FilterEffectSet> m_effectReferences;

        RefPtr<FilterEffect> m_lastEffect;
    };
    
} //namespace WebCore

#endif // ENABLE(SVG) && ENABLE(FILTERS)
#endif
