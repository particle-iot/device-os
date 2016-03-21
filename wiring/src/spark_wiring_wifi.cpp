/**
******************************************************************************
* @file    spark_wiring_wifi.cpp
* @author  Satish Nair
* @version V1.0.0
* @date    7-Mar-2013
* @brief   WiFi utility class to help users manage the WiFi connection
******************************************************************************
Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation, either
version 3 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, see <http://www.gnu.org/licenses/>.
******************************************************************************
*/

#include "spark_wiring_wifi.h"
#include "spark_wiring_ticks.h"
#include "inet_hal.h"
#include "system_task.h"
#include "system_tick_hal.h"
#include <string.h>

#if Wiring_WiFi

namespace spark {

    class APArrayPopulator
    {
        WiFiAccessPoint* results;

        int index;

        void addResult(WiFiAccessPoint* result) {
            if (index<count) {
                results[index++] = *result;
            }
        }

    protected:
        static void callback(WiFiAccessPoint* result, void* cookie)
        {
            ((APArrayPopulator*)cookie)->addResult(result);
        }

        int count;
    public:
        APArrayPopulator(WiFiAccessPoint* results, int size) {
            this->results = results;
            this->count = size;
            this->index = 0;
        }
    };

    class APScan : public APArrayPopulator {
        public:
        using APArrayPopulator::APArrayPopulator;

        int start()
        {
            return std::min(count, wlan_scan(callback, this));
        }
    };

    class APList : public APArrayPopulator {
        public:
        using APArrayPopulator::APArrayPopulator;

        int start()
        {
            return std::min(count, wlan_get_credentials(callback, this));
        }
    };


    int WiFiClass::scan(WiFiAccessPoint* results, size_t result_count) {
        APScan apScan(results, result_count);
        return apScan.start();
    }

    int WiFiClass::getCredentials(WiFiAccessPoint* results, size_t result_count) {
        APList apList(results, result_count);
        return apList.start();
    }

    int8_t WiFiClass::RSSI() {
        if (!network_ready(*this, 0, NULL))
            return 0;

        system_tick_t _functionStart = millis();
        while ((millis() - _functionStart) < 1000) {
            int rv = wlan_connected_rssi();
            if (rv != 0)
                return (rv);
        }
        return (2);
    }

/********************************* Bug Notice *********************************
On occasion, "wlan_ioctl_get_scan_results" only returns a single bad entry
(with index 0). I suspect this happens when the CC3000 is refreshing the
scan table; I think it deletes the current entries, does a new scan then
repopulates the table. If the function is called during this process
the table only contains the invalid zero indexed entry.
The good news is the way I've designed the function mitigates this problem.
The main while loop prevents the function from running for more than one
second; the inner while loop prevents the function from reading more than
16 entries from the scan table (which is the maximum amount it can hold).
The first byte of the scan table lists the number of entries remaining;
we use this to break out of the inner loop when we reach the last entry.
This is done so that we read out the entire scan table (ever after finding
our SSID) so the data isn't stale on the next function call. If the function
is called when the table contains invalid data, the index will be zero;
this causes the inner loop to break and start again; this action will
repeat until the scan table has been repopulated with valid entries (or the
one second timeout is reached). If the aforementioned "bug" is ever fixed by
TI, no changes need to be made to this function, as it would be implemented
the same way.
*****************************************************************************/

    WiFiClass WiFi;
    NetworkClass& Network = WiFi;
}

#endif
