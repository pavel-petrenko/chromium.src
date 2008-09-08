/*
 *  Copyright (C) 2003, 2007 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "CommonIdentifiers.h"

namespace JSC {

const char* const nullCString = 0;

#define INITIALIZE_PROPERTY_NAME(name) , name(globalData, #name)

CommonIdentifiers::CommonIdentifiers(JSGlobalData* globalData)
    : nullIdentifier(globalData, nullCString)
    , underscoreProto(globalData, "__proto__")
    , thisIdentifier(globalData, "this")
    JSC_COMMON_IDENTIFIERS_EACH_PROPERTY_NAME(INITIALIZE_PROPERTY_NAME)
{
}

} // namespace JSC
