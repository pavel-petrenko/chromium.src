/*
    Copyright (C) 2004, 2005 Nikolas Zimmermann <wildfox@kde.org>
				  2004, 2005 Rob Buis <buis@kde.org>

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

#ifndef KSVG_SVGUseElement_H
#define KSVG_SVGUseElement_H

#include <ksvg2/ecma/SVGLookup.h>
#include <ksvg2/dom/SVGElement.h>
#include <ksvg2/dom/SVGTests.h>
#include <ksvg2/dom/SVGLangSpace.h>
#include <ksvg2/dom/SVGExternalResourcesRequired.h>
#include <ksvg2/dom/SVGStylable.h>
#include <ksvg2/dom/SVGTransformable.h>
#include <ksvg2/dom/SVGURIReference.h>

namespace KSVG
{
	class SVGElementInstance;
	class SVGAnimatedLength;
	class SVGUseElementImpl;
	class SVGUseElement : public SVGElement,
						  public SVGTests,
						  public SVGLangSpace,
						  public SVGExternalResourcesRequired,
						  public SVGStylable,
						  public SVGTransformable,
						  public SVGURIReference
	{
	public:
		SVGUseElement();
		explicit SVGUseElement(SVGUseElementImpl *i);
		SVGUseElement(const SVGUseElement &other);
		SVGUseElement(const KDOM::Node &other);
		virtual ~SVGUseElement();

		// Operators
		SVGUseElement &operator=(const SVGUseElement &other);
		SVGUseElement &operator=(const KDOM::Node &other);

		// 'SVGUseElement' functions
		SVGAnimatedLength x() const;
		SVGAnimatedLength y() const;

		SVGAnimatedLength width() const;
		SVGAnimatedLength height() const;

		//SVGElementInstance instanceRoot() const;
		//SVGElementInstance animatedInstanceRoot() const;

		// Internal
		KSVG_INTERNAL(SVGUseElement)

	public: // EcmaScript section
		KDOM_GET
		KDOM_FORWARDPUT

		KJS::ValueImp *getValueProperty(KJS::ExecState *exec, int token) const;
	};
};

#endif

// vim:ts=4:noet
