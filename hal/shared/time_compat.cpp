/*
 * Copyright (c) 2020 Particle Industries, Inc.  All rights reserved.
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

#include "time_compat.h"

#ifndef HAL_TIME_COMPAT_EXCLUDE

struct tm* localtime32_r(const time32_t* timep, struct tm* result) {
    if (!timep) {
        return nullptr;
    }
    time_t tmp = *timep;
    return localtime_r(&tmp, result);
}

time32_t mktime32(struct tm* tm) {
    return (time32_t)mktime(tm);
}

#endif // HAL_TIME_COMPAT_EXCLUDE
