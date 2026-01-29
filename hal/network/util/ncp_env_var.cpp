/*
 * Copyright (c) 2026 Particle Industries, Inc.  All rights reserved.
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

#include "ncp_env_var.h"

#if HAL_PLATFORM_ENV

#include "logging.h"
LOG_SOURCE_CATEGORY("ncp.client");

#if PLATFORM_ID != PLATFORM_GCC
#include "system_env.h"
#else
#include <stdlib.h>
#include <string.h>
#endif

#include "system_error.h"

namespace particle {

#define ENV_VAR_STR_BUF_MAX (256)

#if PLATFORM_ID == PLATFORM_GCC
int getEnvVar(const char* name, char* val, size_t valSize) {
    (void) name;
    (void) val;
    (void) valSize;

    return -1;
}

int getEnv(const char* name, char* val, size_t valSize) {
    (void) name;
    (void) val;
    (void) valSize;

    return -1;
}

int getEnv(const char* name, int& value) {
    (void) name;
    (void) value;

    return -1;
}

int getEnv(const char* name, bool& value) {
    (void) name;
    (void) value;

    return -1;
}

bool hasEnv(const char* name) {
    (void) name;

    return false;
}

#else // PLATFORM_ID != PLATFORM_GCC

int getEnvVar(const char* name, char* val, size_t valSize) {
    if (!val || valSize == 0) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    char buf[ENV_VAR_STR_BUF_MAX] = {};
    size_t n = system_get_env(name, buf, sizeof(buf), nullptr /* reserved */);
    if (n >= valSize) {
        return SYSTEM_ERROR_TOO_LARGE;
    }

    if (n < sizeof(buf)) {
        std::memcpy(val, buf, n);
        val[n] = 0;
    } else {
        return SYSTEM_ERROR_NO_MEMORY;
    }

    return 0;
}

int getEnv(const char* name, char* val, size_t valSize) {
    if (!val || valSize == 0) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    int r = getEnvVar(name, val, valSize);
    if (r < 0) {
        val[0] = 0;

        return r;
    }

    return 0;
}

int getEnv(const char* name, int& value) {
    return system_get_env_int(name, &value, nullptr);
}

int getEnv(const char* name, bool& value) {
    return system_get_env_bool(name, &value, nullptr);
}

bool hasEnv(const char* name) {
    int r = system_get_env(name, nullptr /* buf */, 0 /* buf_size */, nullptr /* reserved */);
    return r >= 0;
}

#endif // PLATFORM_ID == PLATFORM_GCC

void hexString128toUint64Array(const char* hexString128, uint64_t* bands) {
    size_t len = strlen(hexString128);
    char* endptr = (char*)hexString128;
    if (len > 16) {
        char upper[17] = {};
        char lower[17] = {};
        size_t upperLen = len - 16;
        memcpy(upper, hexString128, upperLen);
        memcpy(lower, hexString128 + upperLen, 16);
        bands[1] = strtoull(upper, &endptr, 16);
        if (endptr == hexString128 || *endptr != '\0') {
            bands[1] = 0;
        }
        bands[0] = strtoull(lower, &endptr, 16);
        if (endptr == hexString128 || *endptr != '\0') {
            bands[0] = 0;
        }
    } else {
        bands[0] = strtoull(hexString128, &endptr, 16);
        if (endptr == hexString128 || *endptr != '\0') {
            bands[0] = 0;
        }
    }
}

void getEnvBands(const char* varName, uint64_t* bands) {
    if (!bands | !varName) {
        return;
    }

    char envBands[32+1] = {};
    if (hasEnv(varName)) {
        if (getEnv(varName, envBands, sizeof(envBands)) == SYSTEM_ERROR_NONE) {
            hexString128toUint64Array(envBands, bands);
            // LOG(INFO, "%s=0x%llX%016llX", varName, bands[1], bands[0]);
        }
    }
}

int getEnvPreferredPlmn(const char* varName, char plmns[][7]) {
    if (!plmns || !varName) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    char envPlmns[32+1] = {};
    int plmnCount = 0;
    if (hasEnv(varName)) {
        if (getEnv(varName, envPlmns, sizeof(envPlmns)) == SYSTEM_ERROR_NONE) {
            char* token = strtok(envPlmns, ",");
            while (token && plmnCount < MAX_CELLULAR_PREFFERED_PLMN_ENTRIES) {
                size_t len = strlen(token);
                if (len == 5 || len == 6) {
                    strncpy(plmns[plmnCount], token, len);
                    plmns[plmnCount][len] = '\0';
                    plmnCount++;
                }
                token = strtok(NULL, ",");
            }
        }
    }

    return plmnCount;
}

} // particle

#endif // HAL_PLATFORM_ENV

