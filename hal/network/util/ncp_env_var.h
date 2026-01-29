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

#pragma once

#include "hal_platform.h"

#if HAL_PLATFORM_ENV

#include <cstddef>
#include <cstdint>

namespace particle {

#define MAX_CELLULAR_PREFFERED_PLMN_ENTRIES (4)

int getEnvVar(const char* name, char* val, size_t valSize);
int getEnv(const char* name, char* val, size_t valSize);
int getEnv(const char* name, int& value);
int getEnv(const char* name, bool& value);
bool hasEnv(const char* name);
void hexString128toUint64Array(const char* hexString128, uint64_t* bands);
void getEnvBands(const char* varName, uint64_t* bands);
int getEnvPreferredPlmn(const char* varName, char plmns[][7]);

} // particle

#endif // HAL_PLATFORM_ENV
