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

#include "cellular_sig_perc_mapping.h"
#include "logging.h"
#include "string.h"
#include <stdlib.h>
#include <limits>

namespace {

// Cellular signal values and mapping
// XXX: Must change the mapping tables associated with region categorization

enum SignalMappingRegions {
    SIG_REGION_POOR =  0,   /* <= 0%   */
    SIG_REGION_BAD =   1,   /* <= 25%  */
    SIG_REGION_FAIR =  2,   /* <= 50%  */
    SIG_REGION_GOOD =  3,   /* <= 75%  */
    SIG_REGION_GREAT = 4,   /* <= 100% */
    MAX_REGION_THRESHOLDS
};

/* RF Characterization internal testing plus data from live network.
Please see https://docs.google.com/spreadsheets/d/11qpipAc7JWzr8rAk-SOClfhflT-gXabDPGOMQU4be8I/edit?usp=sharing */
const int CATM1_STRN_MAP[] = {
    -141,   /* SIG_REGION_POOR */
    -105,   /* SIG_REGION_BAD */
    -95,   /* SIG_REGION_FAIR */
    -85,   /* SIG_REGION_GOOD */
    -70,    /* SIG_REGION_GREAT */
    //-44     /* MAX */
};

const int CATM1_QUAL_MAP[] = {
    -20,   /* SIG_REGION_POOR */
    -16,   /* SIG_REGION_BAD */
    -10,   /* SIG_REGION_FAIR */
    -8,   /* SIG_REGION_GOOD */
    -6,   /* SIG_REGION_GREAT */
    //-3     /* MAX */
};

/* Source: https://github.com/aosp-mirror/platform_frameworks_base/blob/master/telephony/java/android/telephony/CellSignalStrengthLte.java
plus data from live networks */
const int CAT1_STRN_MAP[] = {
    -141,   /* SIG_REGION_POOR */
    -105,   /* SIG_REGION_BAD */
    -95,   /* SIG_REGION_FAIR */
    -85,    /* SIG_REGION_GOOD */
    -70,    /* SIG_REGION_GREAT */
    //-44     /* MAX */
};

const int CAT1_QUAL_MAP[] = {
    -20,   /* SIG_REGION_POOR */
    -16,   /* SIG_REGION_BAD */
    -10,   /* SIG_REGION_FAIR */
    -8,   /* SIG_REGION_GOOD */
    -6,   /* SIG_REGION_GREAT */
    //-3     /* MAX */
};

/* Source: https://github.com/aosp-mirror/platform_frameworks_base/blob/master/telephony/java/android/telephony/CellSignalStrengthWcdma.java */
const int UTRAN_STRN_MAP[] = {
    -121,   /* SIG_REGION_POOR */
    -115,   /* SIG_REGION_BAD */
    -105,   /* SIG_REGION_FAIR */
    -95,    /* SIG_REGION_GOOD */
    -85,    /* SIG_REGION_GREAT */
    //-24     /* MAX */
};

/* Source: https://comtech.vsb.cz/qualmob/ecno.html */
const int UTRAN_QUAL_MAP[] = {
    -24,   /* SIG_REGION_POOR */
    -17,   /* SIG_REGION_BAD */
    -10,   /* SIG_REGION_FAIR */
    -5,    /* SIG_REGION_GOOD */
    -2,    /* SIG_REGION_GREAT */
    //0     /* MAX */
};

/* Source: https://github.com/aosp-mirror/platform_frameworks_base/blob/master/telephony/java/android/telephony/CellSignalStrengthGsm.java */
const int GSM_STRN_MAP[] = {
    -111,   /* SIG_REGION_POOR */
    -107,   /* SIG_REGION_BAD */
    -103,   /* SIG_REGION_FAIR */
    -97,    /* SIG_REGION_GOOD */
    -89,    /* SIG_REGION_GREAT */
    //-48     /* MAX */
};

} /* anonymous */

int cellular_get_scaled_strn(int strn_value, hal_net_access_tech_t access_tech) {
    int res = 0;
    int region = -1;

    // The strength value obtained is a number multiplied by 100. Get the original value.
    strn_value = strn_value / 100;

    int gen_strn_map[MAX_REGION_THRESHOLDS] = {0};

    // Get the mapping data according to the RAT
    switch (access_tech) {
        case NET_ACCESS_TECHNOLOGY_GSM:
        case NET_ACCESS_TECHNOLOGY_EDGE: {
            memcpy(gen_strn_map, GSM_STRN_MAP, sizeof(gen_strn_map));
            break;
        }
        case NET_ACCESS_TECHNOLOGY_UTRAN: {
            memcpy(gen_strn_map, UTRAN_STRN_MAP, sizeof(gen_strn_map));
            break;
        }
        case NET_ACCESS_TECHNOLOGY_LTE: {
            memcpy(gen_strn_map, CAT1_STRN_MAP, sizeof(gen_strn_map));
            break;
        }
        case NET_ACCESS_TECHNOLOGY_LTE_CAT_M1:
        case NET_ACCESS_TECHNOLOGY_LTE_CAT_NB1: {
            memcpy(gen_strn_map, CATM1_STRN_MAP, sizeof(gen_strn_map));
            break;
        }
        default: {
            res = std::numeric_limits<int32_t>::min();
            break;
        }
    }

    // If RAT was not selected, return
    if (res == std::numeric_limits<int32_t>::min()) {
        return res;
    }

    const uint32_t hundredPercent = 65535;
    // Check which region the value falls into. Convert into percetages, but scale it to [0,65535]
    if (strn_value < gen_strn_map[SIG_REGION_POOR]) {
        return 0*65535;
    } else if (strn_value >= gen_strn_map[MAX_REGION_THRESHOLDS-1]) {
        return hundredPercent;
    } else {
        for (int i = MAX_REGION_THRESHOLDS-2; i >= SIG_REGION_POOR; i--) {
            if (strn_value >= gen_strn_map[i]) {
                region = i;
                break;
            }
        }
        // Covert dBm to percetange based on that region and the value from mapping table
        const uint32_t regionPercentage = (hundredPercent * region * 100) / (MAX_REGION_THRESHOLDS - 1);
        int new_range = 100 / (MAX_REGION_THRESHOLDS - 1);
        int old_range = gen_strn_map[region+1] - gen_strn_map[region];
        const uint32_t percentageWithinRegion = (((strn_value - gen_strn_map[region])*new_range*65535)/old_range);
        res = (regionPercentage + percentageWithinRegion)/100;
    }
    return res;
}

int cellular_get_scaled_qual(int qual_value, hal_net_access_tech_t access_tech) {
    int res = 0;
    int region = -1;

    // The quality value obtained is a number multiplied by 100. Get the original value.
    qual_value = qual_value / 100;

    int gen_qual_map[MAX_REGION_THRESHOLDS] = {0};

    // Quality conversion considered only for LTE RAT in this impl
    switch (access_tech) {
        case NET_ACCESS_TECHNOLOGY_LTE: {
            memcpy(gen_qual_map, CAT1_QUAL_MAP, sizeof(gen_qual_map));
            break;
        }
        case NET_ACCESS_TECHNOLOGY_LTE_CAT_M1:
        case NET_ACCESS_TECHNOLOGY_LTE_CAT_NB1: {
            memcpy(gen_qual_map, CATM1_QUAL_MAP, sizeof(gen_qual_map));
            break;
        }
        case NET_ACCESS_TECHNOLOGY_UTRAN:
            memcpy(gen_qual_map, UTRAN_QUAL_MAP, sizeof(gen_qual_map));
            break;
        default: {
            res = std::numeric_limits<int32_t>::min();
            break;
        }
    }

    if (res == std::numeric_limits<int32_t>::min()) {
        return res;
    }

    // Check which region the value falls into. Convert into percetages, but scale it to [0,65535]
    const uint32_t hundredPercent = 65535;
    if (qual_value < gen_qual_map[SIG_REGION_POOR]) {
        return 0;
    } else if (qual_value >= gen_qual_map[MAX_REGION_THRESHOLDS-1]) {
        return hundredPercent;
    } else {
        for (int i = MAX_REGION_THRESHOLDS-2; i >= SIG_REGION_POOR; i--) {
            if (qual_value >= gen_qual_map[i]) {
                region = i;
                break;
            }
        }
        // Covert dBm to percetange based on that region and the value from mapping table
        const uint32_t regionPercentage = (hundredPercent * region * 100) / (MAX_REGION_THRESHOLDS - 1);
        int new_range = 100 / (MAX_REGION_THRESHOLDS - 1);
        int old_range = gen_qual_map[region+1] - gen_qual_map[region];
        const uint32_t percentageWithinRegion = (((qual_value - gen_qual_map[region])*new_range*65535)/old_range);
        res = (regionPercentage + percentageWithinRegion)/100;
    }
    return res;
}