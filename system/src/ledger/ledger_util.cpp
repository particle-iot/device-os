/*
 * Copyright (c) 2023 Particle Industries, Inc.  All rights reserved.
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

#include "hal_platform.h"

#if HAL_PLATFORM_LEDGER

#include <cstdio>
#include <cstdarg>

#include "ledger.h"
#include "ledger_util.h"

#include "system_error.h"

namespace particle::system {

int formatLedgerPath(char* buf, size_t size, const char* ledgerName, const char* fmt, ...) {
    // Format the prefix part of the path
    int n = std::snprintf(buf, size, "%s/%s/", LEDGER_ROOT_DIR, ledgerName);
    if (n < 0) {
        return SYSTEM_ERROR_INTERNAL;
    }
    size_t pos = n;
    if (pos >= size) {
        return SYSTEM_ERROR_PATH_TOO_LONG;
    }
    if (fmt) {
        // Format the rest of the path
        va_list args;
        va_start(args, fmt);
        n = vsnprintf(buf + pos, size - pos, fmt, args);
        va_end(args);
        if (n < 0) {
            return SYSTEM_ERROR_INTERNAL;
        }
        pos += n;
        if (pos >= size) {
            return SYSTEM_ERROR_PATH_TOO_LONG;
        }
    }
    return pos;
}

} // namespace particle::system

#endif // HAL_PLATFORM_LEDGER
