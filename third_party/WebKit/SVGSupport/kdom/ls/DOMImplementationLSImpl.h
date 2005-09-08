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

#ifndef KDOM_DOMImplementationLSImpl_H
#define KDOM_DOMImplementationLSImpl_H

namespace KDOM
{
    class LSInputImpl;
    class LSOutputImpl;
    class LSParserImpl;
    class DOMStringImpl;
    class LSSerializerImpl;

    class DOMImplementationLSImpl
    {
    public:
        DOMImplementationLSImpl();
        virtual ~DOMImplementationLSImpl();

        // 'DOMImplementationLS' functions
        virtual LSParserImpl *createLSParser(unsigned short mode, DOMStringImpl *schemaType) const;
        virtual LSInputImpl *createLSInput() const;
        virtual LSOutputImpl *createLSOutput() const;
        virtual LSSerializerImpl *createLSSerializer() const;

    };
};

#endif

// vim:ts=4:noet
