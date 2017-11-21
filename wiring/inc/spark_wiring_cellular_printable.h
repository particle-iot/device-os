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
#include "spark_wiring_signal.h"
#include <string.h>

#if Wiring_Cellular || defined(UNIT_TEST)

#include "cellular_hal.h"
#include "modem/enums_hal.h"

/*
 * CellularSignal
 */
class CellularSignal : public particle::Signal, public Printable {
public:
    int rssi = 0;
    int qual = 0;

    CellularSignal() {}
    CellularSignal(const cellular_signal_t& sig);
    virtual ~CellularSignal() {};

    bool fromHalCellularSignal(const cellular_signal_t& sig);

    virtual hal_net_access_tech_t getAccessTechnology() const;
    virtual float getStrength() const;
    virtual float getStrengthValue() const;
    virtual float getQuality() const;
    virtual float getQualityValue() const;

    virtual size_t printTo(Print& p) const;

private:
    cellular_signal_t sig_ = {0};
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

#ifndef UNIT_TEST

// Bjorn Karlsson's Safe Bool Idiom - http://www.artima.com/cppsource/safebool.html
// Note: clang, which we use for unit tests on Mac, doesn't consider this idiom a valid C++ code
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

#endif // defined(UNIT_TEST)

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

#ifndef UNIT_TEST

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

#endif // defined(UNIT_TEST)

#endif // Wiring_Cellular || defined(UNIT_TEST)

#endif // __SPARK_WIRING_CELLULAR_PRINTABLE_H
