/*
    Copyright (C) 2004 Nikolas Zimmermann <wildfox@kde.org>
				  2004 Rob Buis <buis@kde.org>

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

#ifndef KSVG_SVGTransformable_H
#define KSVG_SVGTransformable_H

#include <ksvg2/ecma/SVGLookup.h>
#include <ksvg2/dom/SVGLocatable.h>

namespace KSVG
{
	class SVGAnimatedTransformList;
	class SVGTransformableImpl;
	class SVGTransformable : public SVGLocatable
	{
	public:
		SVGTransformable();
		explicit SVGTransformable(SVGTransformableImpl *i);
		SVGTransformable(const SVGTransformable &other);
		virtual ~SVGTransformable();

		// Operators
		SVGTransformable &operator=(const SVGTransformable &other);
		SVGTransformable &operator=(SVGTransformableImpl *other);

		// 'SVGTransformable' functions
		SVGAnimatedTransformList transform() const;

		// Internal
		KSVG_INTERNAL_BASE(SVGTransformable)

	protected:
		SVGTransformableImpl *impl;

	public: // EcmaScript section
		KDOM_GET

		KJS::ValueImp *getValueProperty(KJS::ExecState *exec, int token) const;
	};
};

#endif

// vim:ts=4:noet
