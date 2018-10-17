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

// #include "cellular_internal.h"

namespace spark {

    CellularSignal CellularClass::RSSI() {
        CellularSignal sig;
        if (!network_ready(*this, 0, NULL)) {
            sig.rssi = 0;
            return sig;
        }

        CellularSignalHal sig_hal = {0};
        cellular_signal_t sigext = {0};
        sigext.size = sizeof(sigext);
        if (cellular_signal(&sig_hal, &sigext) != 0) {
            sig.rssi = 1;
            return sig;
        }
        sig.rssi = sig_hal.rssi;
        sig.qual = sig_hal.qual;
        if (sig.rssi == 0) {
            sig.rssi = 2;
        }
        sig.fromHalCellularSignal(sigext);
        return sig;
    }

    CellularDataHal data_hal;

    bool CellularClass::getDataUsage(CellularData &data_get) {
        if (cellular_data_usage_get(&data_hal, NULL) != 0) {
            data_get.ok = false; // dataUsage object was not updated
        } else {
            data_get.tx_session = data_hal.tx_session;
            data_get.rx_session = data_hal.rx_session;
            data_get.tx_total = data_hal.tx_total;
            data_get.rx_total = data_hal.rx_total;
            data_get.ok = true;
        }
        data_get.cid = data_hal.cid; // data_hal.cid will indicate -1 in case of an error
        return data_get.ok;
    }

    bool CellularClass::setDataUsage(CellularData &data_set) {
        data_hal.tx_session = data_set.tx_session;
        data_hal.rx_session = data_set.rx_session;
        data_hal.tx_total = data_set.tx_total;
        data_hal.rx_total = data_set.rx_total;
        if (cellular_data_usage_set(&data_hal, NULL) != 0) {
            data_set.ok = false; // dataUsage object was not updated
        } else {
            data_set.ok = true;
        }
        data_set.cid = data_hal.cid; // update, in case CID changed
        return data_set.ok;
    }

    bool CellularClass::resetDataUsage() {
        data_hal.cid = -1;
        data_hal.tx_session = 0;
        data_hal.rx_session = 0;
        data_hal.tx_total = 0;
        data_hal.rx_total = 0;
        if (cellular_data_usage_set(&data_hal, NULL) != 0) {
            return false; // dataUsage object was not updated
        }
        return true;
    }

    bool CellularClass::setBandSelect(CellularBand &band_set) {
        MDM_BandSelect band_hal;
        for (int i=0; i<band_set.count; i++) {
            if (band_set.isBand(band_set.band[i])) {
                band_hal.band[i] = band_set.band[i];
            } else {
                return (band_set.ok = false); // mark band_set object invalid
            }
        }
        band_hal.count = band_set.count;
        // mark band_set object valid
        band_set.ok = true;
        if (cellular_band_select_set(&band_hal, NULL) != 0) {
            return false; // band select was not updated
        }
        return true;
    }

    bool CellularClass::setBandSelect(const char* bands) {
        CellularBand band_set;
        int c = 0;
        int b[4];
        if ((c = sscanf(bands, "%d,%d,%d,%d", &b[0],&b[1],&b[2],&b[3])) > 0) {
            for (int i=0; i<c; i++) {
                // ensure each matched band also matches an enumerated band
                if (band_set.isBand(b[i])) {
                    band_set.band[i] = (MDM_Band)b[i];
                } else {
                    return false;
                }
            }
            band_set.count = c;
            // if the first band is 0, this superceeds all other band input (factory defaults)
            if (band_set.band[0] == 0) band_set.count = 1;
            return setBandSelect(band_set);
        }
        return false; // no integers parsed from string.
    }

    bool CellularClass::getBandSelect(CellularBand &band_get) {
        MDM_BandSelect band_hal;
        if (cellular_band_select_get(&band_hal, NULL) != 0) {
            return (band_get.ok = false); // band_hal object was not updated
        }
        band_get.count = band_hal.count;
        memcpy(band_get.band, &band_hal.band, sizeof(band_get.band));
        return (band_get.ok = true);
    }

    bool CellularClass::getBandAvailable(CellularBand &band_get) {
        MDM_BandSelect band_hal;
        if (cellular_band_available_get(&band_hal, NULL) != 0) {
            return (band_get.ok = false); // band_hal object was not updated
        }
        band_get.count = band_hal.count;
        memcpy(band_get.band, &band_hal.band, sizeof(band_get.band));
        return (band_get.ok = true);
    }

    CellularClass Cellular;
    // NetworkClass& Network = Cellular;
}

#endif
