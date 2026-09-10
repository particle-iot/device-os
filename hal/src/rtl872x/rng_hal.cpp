/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
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

#include "rng_hal.h"

#include "backup_ram_hal.h"
#include "check.h"
#include "entropy_hal.h"
#include "platform_headers.h"
#include "scope_guard.h"
#include "service_debug.h"
#include "static_recursive_mutex.h"

#include <mbedtls/ctr_drbg.h>

#include <mutex>
#include <string.h>

namespace {

const size_t ENTROPY_SIZE = 128;
const size_t RETAINED_SEED_SIZE = 32;
const size_t REFRESH_SIZE = 8;
const uint32_t RETAINED_SEED_MAGIC = 0x524e4753;
const uint8_t RETAINED_SEED_VERSION = 1;

struct RetainedSeed {
    uint32_t magic;
    uint8_t version;
    uint8_t reserved[3];
    uint8_t data[RETAINED_SEED_SIZE];
} __attribute__((packed));

static_assert(sizeof(RetainedSeed) == 40, "Invalid retained seed size");

retained_system RetainedSeed retainedSeed;

mbedtls_ctr_drbg_context drbg = {};
StaticRecursiveMutex drbgMutex;
uint8_t seedMaterial[ENTROPY_SIZE] = {};
bool seedMaterialReady = false;
bool drbgReady = false;

int entropyPoll(void*, unsigned char* data, size_t size) {
    if (seedMaterialReady) {
        CHECK_TRUE(size == sizeof(seedMaterial), SYSTEM_ERROR_INVALID_ARGUMENT);
        memcpy(data, seedMaterial, size);
        seedMaterialReady = false;
        return SYSTEM_ERROR_NONE;
    }
    return hal_entropy_read(data, size, size * 8);
}

int initDrbg() {
    if (drbgReady) {
        return SYSTEM_ERROR_NONE;
    }

    memset(seedMaterial, 0, sizeof(seedMaterial));
    SCOPE_GUARD({
        memset(seedMaterial, 0, sizeof(seedMaterial));
    });
    const bool haveRetainedSeed = (retainedSeed.magic == RETAINED_SEED_MAGIC &&
            retainedSeed.version == RETAINED_SEED_VERSION);
    if (haveRetainedSeed) {
        memcpy(seedMaterial, retainedSeed.data, sizeof(retainedSeed.data));
        retainedSeed.magic = 0;
        CHECK(hal_entropy_read(seedMaterial + RETAINED_SEED_SIZE, REFRESH_SIZE, REFRESH_SIZE * 8));
    } else {
        CHECK(hal_entropy_read(seedMaterial, sizeof(seedMaterial), sizeof(seedMaterial) * 8));
    }
    seedMaterialReady = true;

    mbedtls_ctr_drbg_init(&drbg);
    mbedtls_ctr_drbg_set_entropy_len(&drbg, sizeof(seedMaterial));
    CHECK(mbedtls_ctr_drbg_set_nonce_len(&drbg, 0));
    CHECK(mbedtls_ctr_drbg_seed(&drbg, entropyPoll, nullptr, nullptr, 0));

    retainedSeed.magic = 0;
    CHECK(mbedtls_ctr_drbg_random(&drbg, retainedSeed.data, sizeof(retainedSeed.data)));
    retainedSeed.version = RETAINED_SEED_VERSION;
    retainedSeed.magic = RETAINED_SEED_MAGIC;

    if (!haveRetainedSeed) {
        CHECK(hal_backup_ram_sync(nullptr));
    }

    drbgReady = true;
    return SYSTEM_ERROR_NONE;
}

uint32_t randomNumber() {
    uint32_t value = 0;
    SPARK_ASSERT(mbedtls_ctr_drbg_random(&drbg, reinterpret_cast<uint8_t*>(&value), sizeof(value)) == 0);
    return value;
}

} // anonymous namespace

void HAL_RNG_Configuration() {
    SPARK_ASSERT(initDrbg() == 0);
}

uint32_t HAL_RNG_GetRandomNumber() {
    SPARK_ASSERT(drbgReady);
    std::lock_guard<StaticRecursiveMutex> lk(drbgMutex);
    return randomNumber();
}

extern "C" int __wrap_rtw_get_random_bytes(void* data, uint32_t size) {
    CHECK_TRUE(data || !size, SYSTEM_ERROR_INVALID_ARGUMENT);
    auto p = static_cast<uint8_t*>(data);
    while (size >= sizeof(uint32_t)) {
        const uint32_t value = HAL_RNG_GetRandomNumber();
        memcpy(p, &value, sizeof(value));
        p += sizeof(value);
        size -= sizeof(value);
    }
    if (size) {
        const uint32_t value = HAL_RNG_GetRandomNumber();
        memcpy(p, &value, size);
    }
    return SYSTEM_ERROR_NONE;
}
