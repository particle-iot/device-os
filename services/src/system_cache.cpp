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

#include "hal_platform.h"
#include <limits>

#if HAL_PLATFORM_FILESYSTEM

#include "system_cache.h"
#include "enumclass.h"
#include "check.h"

namespace particle { namespace services {

SystemCache::SystemCache()
        : tlv_("/sys/cache.dat") {
}

SystemCache& SystemCache::instance() {
    static SystemCache cache;
    cache.tlv_.init();
    return cache;
}

int SystemCache::get(SystemCacheKey key, void* value, size_t length) {
    return tlv_.get(to_underlying(key), (uint8_t*)value, length);
}

int SystemCache::set(SystemCacheKey key, const void* value, size_t length) {
    CHECK_TRUE(length <= std::numeric_limits<uint16_t>::max(), SYSTEM_ERROR_TOO_LARGE);
    return tlv_.set(to_underlying(key), (const uint8_t*)value, length, 0);
}

int SystemCache::del(SystemCacheKey key) {
    return tlv_.del(to_underlying(key));
}

} } // particle::services

#endif // HAL_PLATFORM_FILESYSTEM
