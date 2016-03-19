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

#ifndef __SPARK_WIRING_CELLULAR_PRINTABLE_H
#define __SPARK_WIRING_CELLULAR_PRINTABLE_H

#include "spark_wiring_platform.h"
#include "spark_wiring_printable.h"
#include "spark_wiring_string.h"
#include <string.h>

#if Wiring_Cellular

#include "cellular_hal.h"
#include "modem/enums_hal.h"

/*
 * CellularSignal
 */
class CellularSignal : public Printable {
public:
    int rssi = 0;
    int qual = 0;

    CellularSignal() { /* n/a */ }

    virtual size_t printTo(Print& p) const;
};

/*
 * CellularData
 */
class CellularData : public Printable {
private:
    typedef void (CellularData::*bool_type)() const;
    void this_type_does_not_support_comparisons() const {}

public:
    /* false when constructed, used to indicate whether
     * last operation on object was successful or not. */
    bool ok;

    int cid = -1;
    int tx_session = 0;
    int rx_session = 0;
    int tx_total = 0;
    int rx_total = 0;

    virtual size_t printTo(Print& p) const;

    explicit CellularData(bool b=false):ok(b) {}
    operator bool_type() const {
        return ok==true ? &CellularData::this_type_does_not_support_comparisons : 0;
    }
};
// Bjorn Karlsson's Safe Bool Idiom - http://www.artima.com/cppsource/safebool.html
template <typename T>
bool operator!=(const CellularData& lhs,const T& rhs) {
    lhs.this_type_does_not_support_comparisons();
    return false;
}
template <typename T>
bool operator==(const CellularData& lhs,const T& rhs) {
    lhs.this_type_does_not_support_comparisons();
    return false;
}

/*
 * CellularBand
 */
class CellularBand : public Printable {
private:
    typedef void (CellularBand::*bool_type)() const;
    void this_type_does_not_support_comparisons() const {}
    const int band_enums[11] { BAND_0, BAND_700, BAND_800, BAND_850,
                       BAND_900, BAND_1500, BAND_1700, BAND_1800,
                       BAND_1900, BAND_2100, BAND_2600 };
public:
    /* false when constructed, used to indicate whether
     * last operation on object was successful or not. */
    bool ok;

    int count = 0;
    MDM_Band band[5] = {BAND_DEFAULT};

    virtual size_t printTo(Print& p) const;

    explicit CellularBand(bool b=false):ok(b) {}
    operator bool_type() const {
        return ok==true ? &CellularBand::this_type_does_not_support_comparisons : 0;
    }

    bool isBand(int x)
    {
        for (int i=0; i<(int)(sizeof(band_enums)/sizeof(*band_enums)); i++)
        {
            if (band_enums[i] == x) {
                return true;
            }
        }
        return false;
    }
};
template <typename T>
bool operator!=(const CellularBand& lhs,const T& rhs) {
    lhs.this_type_does_not_support_comparisons();
    return false;
}
template <typename T>
bool operator==(const CellularBand& lhs,const T& rhs) {
    lhs.this_type_does_not_support_comparisons();
    return false;
}

#endif // Wiring_Cellular

#endif // __SPARK_WIRING_CELLULAR_PRINTABLE_H
