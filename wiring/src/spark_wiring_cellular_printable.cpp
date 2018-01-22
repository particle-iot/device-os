/**
 ******************************************************************************
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

#include "spark_wiring_cellular_printable.h"
#include "spark_wiring_print.h"
#include "string.h"
#include <limits>

#if Wiring_Cellular || defined(UNIT_TEST)

CellularSignal::CellularSignal(const cellular_signal_t& sig)
    : sig_(sig)
{
}

bool CellularSignal::fromHalCellularSignal(const cellular_signal_t& sig)
{
    sig_ = sig;
    return true;
}

hal_net_access_tech_t CellularSignal::getAccessTechnology() const
{
    return static_cast<hal_net_access_tech_t>(sig_.rat);
}

float CellularSignal::getStrength() const
{
    if (sig_.rat != NET_ACCESS_TECHNOLOGY_NONE && sig_.strength >= 0) {
        return (float)sig_.strength / 65535.0f * 100.0f;
    }

    return -1.0f;
}

float CellularSignal::getStrengthValue() const
{
    if (sig_.rat != NET_ACCESS_TECHNOLOGY_NONE && sig_.rssi != std::numeric_limits<int32_t>::min()) {
        return (float)sig_.rssi / 100.0f;
    }

    return 0.0f;
}

float CellularSignal::getQuality() const
{
    if (sig_.rat != NET_ACCESS_TECHNOLOGY_NONE && sig_.quality >= 0) {
        return (float)sig_.quality / 65535.0f * 100.0f;
    }

    return -1.0f;
}

float CellularSignal::getQualityValue() const
{
    if (sig_.rat != NET_ACCESS_TECHNOLOGY_NONE && sig_.qual != std::numeric_limits<int32_t>::min()) {
        return (float)sig_.qual / 100.0f;
    }

    return 0.0f;
}

size_t CellularSignal::printTo(Print& p) const
{
    size_t n = 0;
    n += p.print((*this).rssi, DEC);
    n += p.print(',');
    n += p.print((*this).qual, DEC);
    return n;
}

size_t CellularData::printTo(Print& p) const
{
    size_t n = 0;
    n += p.print((*this).cid, DEC);
    n += p.print(',');
    n += p.print((*this).tx_session, DEC);
    n += p.print(',');
    n += p.print((*this).rx_session, DEC);
    n += p.print(',');
    n += p.print((*this).tx_total, DEC);
    n += p.print(',');
    n += p.print((*this).rx_total, DEC);
    return n;
}

size_t CellularBand::printTo(Print& p) const
{
    size_t n = 0;
    for (int i=0; i<(*this).count; i++) {
      n += p.print((*this).band[i], DEC);
      if (i+1<(*this).count) n += p.print(',');
    }
    return n;
}

#endif // Wiring_Cellular || defined(UNIT_TEST)
