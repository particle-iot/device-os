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

#include "spark_wiring_cellular.h"

#if Wiring_Cellular

namespace spark {

    CellularSignal CellularClass::RSSI() {
        CellularSignal sig;
        if (!network_ready(*this, 0, NULL)) {
            sig.rssi = 0;
            return sig;
        }

        CellularSignalHal sig_hal;
        if (cellular_signal(sig_hal, NULL) != 0) {
            sig.rssi = 1;
            return sig;
        }
        sig.rssi = sig_hal.rssi;
        sig.qual = sig_hal.qual;
        if (sig.rssi == 0) {
            sig.rssi = 2;
        }
        return sig;
    }

    CellularClass Cellular;
    NetworkClass& Network = Cellular;
}

#endif
