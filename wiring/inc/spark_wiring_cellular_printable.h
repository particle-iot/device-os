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

#include <string.h>
#include "spark_wiring_printable.h"
#include "spark_wiring_string.h"
#include "cellular_hal.h"

class CellularSignal : public Printable {

public:
    int rssi = 0;
    int qual = 0;

    CellularSignal() { /* n/a */ }

    virtual size_t printTo(Print& p) const;
};

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

#endif
