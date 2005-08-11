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

#ifndef KDOM_CSSRuleList_H
#define KDOM_CSSRuleList_H

#include <kdom/ecma/DOMLookup.h>

namespace KDOM
{
	class StyleListImpl;
	class CSSStyleSheet;
	class CSSRule;
	class CSSRuleListImpl;
	class CSSRuleList 
	{
	public:
		CSSRuleList();
		explicit CSSRuleList(StyleListImpl *i);
		explicit CSSRuleList(CSSRuleListImpl *i);
		CSSRuleList(const CSSRuleList &other);
		~CSSRuleList();

		// Operators
		CSSRuleList &operator=(const CSSRuleList &other);
		bool operator==(const CSSRuleList &other) const;
		bool operator!=(const CSSRuleList &other) const;

		// 'CSSRuleList' functions
		unsigned long length() const;
		CSSRule item(unsigned long index) const;

		// Internal
		KDOM_INTERNAL_BASE(CSSRuleList)

	private:
		CSSRuleListImpl *d;

	public: // EcmaScript section
		KDOM_GET

		KJS::ValueImp *getValueProperty(KJS::ExecState *exec, int token) const;
	};
};

KDOM_DEFINE_PROTOTYPE(CSSRuleListProto)
KDOM_IMPLEMENT_PROTOFUNC(CSSRuleListProtoFunc, CSSRuleList)

#endif

// vim:ts=4:noet
