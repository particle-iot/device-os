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

// Cellular signal values and mapping

// XXX: Must change the mapping tables associated with region categorization
typedef enum {
    CELLULAR_HAL_SIG_REGION_POOR = 0,
    CELLULAR_HAL_SIG_REGION_BAD = 1,
    CELLULAR_HAL_SIG_REGION_FAIR = 2,
    CELLULAR_HAL_SIG_REGION_GOOD = 3,
    CELLULAR_HAL_SIG_REGION_GREAT = 4,
    MAX_NUM_OF_REGION_THRESHOLDS
} SignalMappingRegions;

const int sig_strn_perc_thres = 100 / (MAX_NUM_OF_REGION_THRESHOLDS - 1);

const int catm1_strn_map[] = {
    -141,   /* CELLULAR_HAL_SIG_REGION_POOR */
    -122,   /* CELLULAR_HAL_SIG_REGION_BAD */
    -116,   /* CELLULAR_HAL_SIG_REGION_FAIR */
    -107,   /* CELLULAR_HAL_SIG_REGION_GOOD */
    -95,    /* CELLULAR_HAL_SIG_REGION_GREAT */
    //-44     /* MAX */
};

const int catm1_qual_map[] = {
    -20,   /* CELLULAR_HAL_SIG_REGION_POOR */
    -19,   /* CELLULAR_HAL_SIG_REGION_BAD */
    -17,   /* CELLULAR_HAL_SIG_REGION_FAIR */
    -14,   /* CELLULAR_HAL_SIG_REGION_GOOD */
    -12,   /* CELLULAR_HAL_SIG_REGION_GREAT */
    //-3     /* MAX */
};

const int cat1_strn_map[] = {
    -141,   /* CELLULAR_HAL_SIG_REGION_POOR */
    -115,   /* CELLULAR_HAL_SIG_REGION_BAD */
    -105,   /* CELLULAR_HAL_SIG_REGION_FAIR */
    -95,    /* CELLULAR_HAL_SIG_REGION_GOOD */
    -85,    /* CELLULAR_HAL_SIG_REGION_GREAT */
    //-44     /* MAX */
};

const int cat1_qual_map[] = {
    -20,   /* CELLULAR_HAL_SIG_REGION_POOR */
    -19,   /* CELLULAR_HAL_SIG_REGION_BAD */
    -17,   /* CELLULAR_HAL_SIG_REGION_FAIR */
    -14,   /* CELLULAR_HAL_SIG_REGION_GOOD */
    -12,   /* CELLULAR_HAL_SIG_REGION_GREAT */
    //-3     /* MAX */
};

const int utran_strn_map[] = {
    -121,   /* CELLULAR_HAL_SIG_REGION_POOR */
    -115,   /* CELLULAR_HAL_SIG_REGION_BAD */
    -105,   /* CELLULAR_HAL_SIG_REGION_FAIR */
    -95,    /* CELLULAR_HAL_SIG_REGION_GOOD */
    -85,    /* CELLULAR_HAL_SIG_REGION_GREAT */
    //-24     /* MAX */
};

const int gsm_strn_map[] = {
    -111,   /* CELLULAR_HAL_SIG_REGION_POOR */
    -107,   /* CELLULAR_HAL_SIG_REGION_BAD */
    -103,   /* CELLULAR_HAL_SIG_REGION_FAIR */
    -97,    /* CELLULAR_HAL_SIG_REGION_GOOD */
    -89,    /* CELLULAR_HAL_SIG_REGION_GREAT */
    //-48     /* MAX */
};

int get_scaled_strn(int strn_value, hal_net_access_tech_t access_tech) {
    int res = 0;
    int region = -1;

    // The strength value obtained is a number multiplied by 100. Get the original value.
    strn_value = strn_value / 100;


    int gen_strn_map[MAX_NUM_OF_REGION_THRESHOLDS] = {0};

    // Get the mapping data according to the RAT
    switch (access_tech) {
        case NET_ACCESS_TECHNOLOGY_GSM:
        case NET_ACCESS_TECHNOLOGY_EDGE: {
            memcpy(gen_strn_map,gsm_strn_map, sizeof(gen_strn_map));
            break;
        }
        case NET_ACCESS_TECHNOLOGY_UTRAN: {
            memcpy(gen_strn_map,utran_strn_map, sizeof(gen_strn_map));
            break;
        }
        case NET_ACCESS_TECHNOLOGY_LTE: {
            memcpy(gen_strn_map,cat1_strn_map, sizeof(gen_strn_map));
            break;
        }
        case NET_ACCESS_TECHNOLOGY_LTE_CAT_M1:
        case NET_ACCESS_TECHNOLOGY_LTE_CAT_NB1: {
            memcpy(gen_strn_map,catm1_strn_map, sizeof(gen_strn_map));
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

    // Check which region the value falls into. Convert into percetages, but scale it to [0,65535]
    if (strn_value < gen_strn_map[CELLULAR_HAL_SIG_REGION_POOR]) {
        return 0*65535;
    } else if (strn_value >= gen_strn_map[MAX_NUM_OF_REGION_THRESHOLDS-1]) {
        return 65535;
    } else {
        for (int i=MAX_NUM_OF_REGION_THRESHOLDS-2; i>=CELLULAR_HAL_SIG_REGION_POOR; i--) {
            if (strn_value >= gen_strn_map[i]) {
                region = i;
                break;
            }
        }
        // Covert dBm to percetange based on that region and the value from mapping table
        int new_range = sig_strn_perc_thres;
        int old_range = gen_strn_map[region+1] - gen_strn_map[region];
        res = ((((strn_value - gen_strn_map[region])*new_range*65535)/old_range) +
                                        (sig_strn_perc_thres*region*65535))/100;
    }
    return res;
}

int get_scaled_qual(int qual_value, hal_net_access_tech_t access_tech) {
    int res = 0;
    int region = -1;

    // The quality value obtained is a number multiplied by 100. Get the original value.
    qual_value = qual_value / 100;

    int gen_qual_map[MAX_NUM_OF_REGION_THRESHOLDS] = {0};

    // Quality conversion considered only for LTE RAT in this impl
    switch (access_tech) {
        case NET_ACCESS_TECHNOLOGY_LTE: {
            memcpy(gen_qual_map,cat1_qual_map, sizeof(gen_qual_map));
            break;
        }
        case NET_ACCESS_TECHNOLOGY_LTE_CAT_M1:
        case NET_ACCESS_TECHNOLOGY_LTE_CAT_NB1: {
            memcpy(gen_qual_map,catm1_qual_map, sizeof(gen_qual_map));
            break;
        }
        default: {
            res = std::numeric_limits<int32_t>::min();
            break;
        }
    }

    if (res == std::numeric_limits<int32_t>::min()) {
        return res;
    }

    // Check which region the value falls into. Convert into percetages, but scale it to [0,65535]
    if (qual_value < gen_qual_map[CELLULAR_HAL_SIG_REGION_POOR]) {
        return 0;
    } else if (qual_value >= gen_qual_map[MAX_NUM_OF_REGION_THRESHOLDS-1]) {
        return 65535;
    } else {
        for (int i=MAX_NUM_OF_REGION_THRESHOLDS-2; i>=CELLULAR_HAL_SIG_REGION_POOR; i--) {
            if (qual_value >= gen_qual_map[i]) {
                region = i;
                break;
            }
        }
        // Covert dBm to percetange based on that region and the value from mapping table
        int new_range = sig_strn_perc_thres;
        int old_range = gen_qual_map[region+1] - gen_qual_map[region];
        res = ((((qual_value - gen_qual_map[region])*new_range*65535)/old_range) +
                                        (sig_strn_perc_thres*region*65535))/100;
    }
    return res;
}