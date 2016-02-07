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

    CellularDataHal data_hal;

    bool CellularClass::getDataUsage(CellularData &data_get) {
        if (cellular_data_usage_get(data_hal, NULL) != 0) {
            data_get.cid = data_hal.cid; // data_hal.cid will indicate -1
            data_get.ok = false; // dataUsage object was not updated
        }
        data_get.cid = data_hal.cid;
        data_get.tx_session = data_hal.tx_session;
        data_get.rx_session = data_hal.rx_session;
        data_get.tx_total = data_hal.tx_total;
        data_get.rx_total = data_hal.rx_total;
        data_get.ok = true;
        return data_get.ok;
    }

    bool CellularClass::setDataUsage(CellularData &data_set) {
        data_hal.tx_session = data_set.tx_session;
        data_hal.rx_session = data_set.rx_session;
        data_hal.tx_total = data_set.tx_total;
        data_hal.rx_total = data_set.rx_total;
        if (cellular_data_usage_set(data_hal, NULL) != 0) {
            data_set.cid = data_hal.cid; // data_hal.cid will indicate -1
            data_set.ok = false; // dataUsage object was not updated
        }
        data_set.cid = data_hal.cid; // update, in case CID changed
        data_set.ok = true;
        return data_set.ok;
    }

    bool CellularClass::resetDataUsage() {
        data_hal.cid = -1;
        data_hal.tx_session = 0;
        data_hal.rx_session = 0;
        data_hal.tx_total = 0;
        data_hal.rx_total = 0;
        if (cellular_data_usage_set(data_hal, NULL) != 0) {
            return false; // dataUsage object was not updated
        }
        return true;
    }

    CellularClass Cellular;
    NetworkClass& Network = Cellular;
}

#endif
