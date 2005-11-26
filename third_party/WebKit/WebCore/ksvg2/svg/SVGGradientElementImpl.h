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

#ifndef KSVG_SVGGradientElementImpl_H
#define KSVG_SVGGradientElementImpl_H

#include "SVGURIReferenceImpl.h"
#include "SVGStyledElementImpl.h"
#include "SVGExternalResourcesRequiredImpl.h"

#include "KRenderingPaintServerGradient.h"

namespace KSVG
{
    class SVGGradientElementImpl;
    class SVGAnimatedEnumerationImpl;
    class SVGAnimatedTransformListImpl;
    class SVGGradientElementImpl : public SVGStyledElementImpl,
                                   public SVGURIReferenceImpl,
                                   public SVGExternalResourcesRequiredImpl,
                                   public KCanvasResourceListener
    {
    public:
        SVGGradientElementImpl(const KDOM::QualifiedName& tagName, KDOM::DocumentImpl *doc);
        virtual ~SVGGradientElementImpl();

        // 'SVGGradientElement' functions
        SVGAnimatedEnumerationImpl *gradientUnits() const;
        SVGAnimatedTransformListImpl *gradientTransform() const;
        SVGAnimatedEnumerationImpl *spreadMethod() const;

        virtual void parseMappedAttribute(KDOM::MappedAttributeImpl *attr);
        virtual void notifyAttributeChange() const;
        
        virtual KRenderingPaintServerGradient *canvasResource();
        virtual void resourceNotification() const;

    protected:
        virtual void buildGradient(KRenderingPaintServerGradient *grad) const = 0;
        virtual KCPaintServerType gradientType() const = 0;
        void rebuildStops() const;

    protected:
        mutable SharedPtr<SVGAnimatedEnumerationImpl> m_spreadMethod;
        mutable SharedPtr<SVGAnimatedEnumerationImpl> m_gradientUnits;
        mutable SharedPtr<SVGAnimatedTransformListImpl> m_gradientTransform;
        mutable KRenderingPaintServerGradient *m_resource;
    };
};

#endif

// vim:ts=4:noet
