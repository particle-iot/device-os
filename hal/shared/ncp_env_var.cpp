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

#if PLATFORM_ID != PLATFORM_GCC

#include "logging.h"
LOG_SOURCE_CATEGORY("ncp.client");

#include "system_env.h"
#include "system_error.h"
#include "check.h"

namespace particle {

using namespace particle::system;

void getEnvBands(const char* varName, CellularBandMask& bands) {
    if (!varName) {
        return;
    }

    char envBands[32+1] = {};
    if (hasEnv(varName)) {
        if (getEnv(varName, envBands, sizeof(envBands))) {
            bands.setFromHexString(envBands);
            // LOG(INFO, "%s=0x%llX%016llX", varName, bands.high(), bands.low());
        }
    }
}

int getEnvPreferredPlmn(const char* varName, Vector<CString>& plmns) {
    if (!varName) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    char envPlmns[32+1] = {};
    int plmnCount = 0;
    if (hasEnv(varName)) {
        if (getEnv(varName, envPlmns, sizeof(envPlmns))) {
            // LOG(INFO, "%s=%s", varName, envPlmns);
            char* saveptr = nullptr;
            char* token = strtok_r(envPlmns, ",", &saveptr);
            while (token && plmnCount < MAX_CELLULAR_PREFFERED_PLMN_ENTRIES) {
                size_t len = strlen(token);
                if (len == 5 || len == 6) {
                    CString plmn(token);
                    CHECK_TRUE(plmn, SYSTEM_ERROR_NO_MEMORY);
                    CHECK_TRUE(plmns.append(std::move(plmn)), SYSTEM_ERROR_NO_MEMORY);
                    plmnCount++;
                }
                token = strtok_r(NULL, ",", &saveptr);
            }
        }
    }
    return plmnCount;
}

} // namespace particle

#endif // HAL_PLATFORM_ENV

#endif // PLATFORM_ID != PLATFORM_GCC
