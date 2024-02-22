/*
 * Copyright (c) 2024 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "time.h"
#include "ncp_client.h"
#include "hal_platform.h"

#if HAL_PLATFORM_GNSS

namespace particle {

enum GnssState {
    GNSS_STATE_DISABLED = 0,
    GNSS_STATE_INIT = 1,
    GNSS_STATE_ON = 2,
    GNSS_STATE_OFF = 3,
    GNSS_STATE_MAX = 4
};

enum GnssNmeaType {
    GNSS_NMEA_TYPE_GGA = 0,
    GNSS_NMEA_TYPE_RMC = 1,
    GNSS_NMEA_TYPE_GSV = 2,
    GNSS_NMEA_TYPE_GSA = 3,
    GNSS_NMEA_TYPE_VTG = 4,
    GNSS_NMEA_TYPE_GNS = 5,
    GNSS_NMEA_TYPE_MAX = 6
};

struct GnssPositioningInfo {
    uint16_t version;
    uint16_t size;
    double latitude;
    double longitude;
    float accuracy;
    float altitude;
    float cog;
    float speedKmph;
    float speedKnots;
    struct tm utcTime;
    int satsInView;
    bool locked;
    int posMode;
};

class GnssNcpClientConfig {
public:
    GnssNcpClientConfig()
        : enableGpsOneXtra_(false) {
    }

    GnssNcpClientConfig& enableGpsOneXtra(bool enable) {
        enableGpsOneXtra_ = enable;
        return *this;
    }

    bool isGpsOneXtraEnabled() const {
        return enableGpsOneXtra_;
    }

private:
    bool enableGpsOneXtra_;
};

class GnssNcpClient {
public:
    virtual int gnssConfig(const GnssNcpClientConfig& conf) = 0;
    virtual int gnssOn() = 0;
    virtual int gnssOff() = 0;
    virtual int acquirePositioningInfo(GnssPositioningInfo* info) = 0;
    virtual int acquireNmeaSentences(GnssNmeaType type, char* buf, size_t len) = 0;
    virtual uint32_t getTtff() = 0;
    virtual GnssState gnssState() = 0;

#if HAL_PLATFORM_GPS_ONE_XTRA
    virtual int injectGpsOneXtraTimeAndData(const uint8_t* buf, size_t size) = 0;
    virtual bool isGpsOneXtraEnabled() = 0;
    virtual bool isGpsOneXtraDataValid() = 0;
#endif // HAL_PLATFORM_GPS_ONE_XTRA
};

} // particle

#endif // HAL_PLATFORM_GNSS
