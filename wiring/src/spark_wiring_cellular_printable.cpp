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

#if Wiring_Cellular

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

#endif // Wiring_Cellular
