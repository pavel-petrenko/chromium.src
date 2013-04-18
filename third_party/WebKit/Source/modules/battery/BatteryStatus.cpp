/*
 *  Copyright (C) 2012 Samsung Electronics
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
 */

#include "config.h"
#include "modules/battery/BatteryStatus.h"

#if ENABLE(BATTERY_STATUS)

#include <limits>

namespace WebCore {

PassRefPtr<BatteryStatus> BatteryStatus::create()
{
    return adoptRef(new BatteryStatus);
}

PassRefPtr<BatteryStatus> BatteryStatus::create(bool charging, double chargingTime, double dischargingTime, double level)
{
    return adoptRef(new BatteryStatus(charging, chargingTime, dischargingTime, level));
}

BatteryStatus::BatteryStatus()
    : m_charging(true)
    , m_chargingTime(0)
    , m_dischargingTime(std::numeric_limits<double>::infinity())
    , m_level(1)
{
}

BatteryStatus::BatteryStatus(bool charging, double chargingTime, double dischargingTime, double level)
    : m_charging(charging)
    , m_chargingTime(chargingTime)
    , m_dischargingTime(dischargingTime)
    , m_level(level)
{
}

} // namespace WebCore

#endif // BATTERY_STATUS

