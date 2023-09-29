/*
 * Copyright (c) 2021 Particle Industries, Inc.  All rights reserved.
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

#include <unordered_map>
#include <vector>
#include <cstdint>
#include <cstddef>
#include "system_cache.h"
#include "system_error.h"
#include "enumclass.h"
#include "underlying_type.h"

using namespace particle::services;
using namespace particle;
namespace {

std::unordered_map<UnderlyingType<SystemCacheKey>::Type, std::vector<uint8_t>> sCache;

}

SystemCache::SystemCache() {
}

SystemCache& SystemCache::instance() {
    static SystemCache cache;
    return cache;
}

int SystemCache::get(SystemCacheKey key, void* value, size_t length) {
    auto it = sCache.find(to_underlying(key));
    if (it != sCache.end()) {
        auto& vec = it->second;
        auto len = std::min(length, vec.size());
        uint8_t data[length];
        std::copy(vec.begin(), vec.end(), data);
        memcpy(value, data, len);
        return vec.size();
    }
    return SYSTEM_ERROR_NOT_FOUND;
}

int SystemCache::set(SystemCacheKey key, const void* value, size_t length) {
    std::vector<uint8_t> vec;
    vec.resize(length);
    memcpy(&vec[0], value, length);
    sCache[to_underlying(key)] = vec;
    return vec.size();
}

int SystemCache::del(SystemCacheKey key) {
    sCache.erase(to_underlying(key));
    return SYSTEM_ERROR_NONE;
}
