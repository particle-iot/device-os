/*
 * Copyright (c) 2019 Particle Industries, Inc.  All rights reserved.
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

#include "network_config_db.h"
#include "logging.h"

#include "cellular_network_manager.h"

#include <cstring>

namespace particle {

namespace {

struct NetworkConfig {
    const char* mcc;
    const char* mnc;
    const char *iccid_prefix;
    const char* apn;
    const char* user;
    const char* password;
};

const NetworkConfig NETWORK_CONFIG[] = {
    { "214", "07", "", "spark.telefonica.com", "", "" }, // Telefonica
    { "310", "410", "", "10569.mcs", "", "" }, // Kore/AT&T
    { "204", "04", "", "vfd1.korem2m.com", "kore", "kore" } // Kore/Vodafone
};

const NetworkConfig NETWORK_CONFIG_ICCID[] = {
    { "", "", "8988307", "super", "", "" }, // Twilio
    { "", "", "8988323", "super", "", "" }  // Twilio
};

const size_t NETWORK_CONFIG_SIZE = sizeof(NETWORK_CONFIG) / sizeof(NETWORK_CONFIG[0]);
const size_t NETWORK_CONFIG_ICCID_SIZE = sizeof(NETWORK_CONFIG_ICCID) / sizeof(NETWORK_CONFIG_ICCID[0]);

const size_t MCC_SIZE = 3;
const size_t ICCID_MIN_SIZE = 19;
const size_t ICCID_PREFIX_LEN = 7;

} // unnamed

CellularNetworkConfig networkConfigForIccid(const char* iccid, size_t size) {
    if (size < ICCID_MIN_SIZE + 7) {    // 7 bytes for "+CCID: " in AT response
        return CellularNetworkConfig();
    }

    for (size_t i = 0; i < NETWORK_CONFIG_ICCID_SIZE; ++i) {
        const NetworkConfig& conf = NETWORK_CONFIG_ICCID[i];
        if (strncmp(iccid+7, conf.iccid_prefix, ICCID_PREFIX_LEN) == 0) {
            return CellularNetworkConfig().apn(conf.apn).user(conf.user).password(conf.password);
        }
    }
    return CellularNetworkConfig();
}

CellularNetworkConfig networkConfigForImsi(const char* imsi, size_t size) {
    if (size < MCC_SIZE) {
        return CellularNetworkConfig();
    }
    for (size_t i = 0; i < NETWORK_CONFIG_SIZE; ++i) {
        const NetworkConfig& conf = NETWORK_CONFIG[i];
        if (strncmp(imsi, conf.mcc, MCC_SIZE) == 0) {
            const size_t mncSize = strlen(conf.mnc);
            if (size < MCC_SIZE + mncSize) {
                return CellularNetworkConfig();
            }
            if (strncmp(imsi + MCC_SIZE, conf.mnc, mncSize) == 0) {
                return CellularNetworkConfig().apn(conf.apn).user(conf.user).password(conf.password);
            }
        }
    }
    return CellularNetworkConfig();
}

} // particle
