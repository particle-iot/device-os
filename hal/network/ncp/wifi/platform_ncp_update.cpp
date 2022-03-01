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
 * MERCHAN'TABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#undef LOG_COMPILE_TIME_LEVEL

#include "platform_ncp.h"
#include "network/ncp/wifi/ncp.h"
#include "network/ncp/wifi/wifi_network_manager.h"
#include "network/ncp/wifi/wifi_ncp_client.h"
#include "network/ncp/ncp_client.h"
#include "led_service.h"
#include "check.h"
#include "scope_guard.h"
#include "system_led_signal.h"
#include "stream.h"
#include "debug.h"
#include "ota_flash_hal_impl.h"
#include "system_cache.h"
#include "exflash_hal.h"
#include "flash_hal.h"
#include <functional>
#include <algorithm>

namespace {

class OtaUpdateSourceStream : public particle::InputStream {
public:
    typedef std::function<int(uintptr_t addr, uint8_t* dataBuf, size_t dataSize)> ReadStreamFunc;

    OtaUpdateSourceStream(uintptr_t address, size_t offset, size_t length, ReadStreamFunc func)
            : address_(address), offset_(offset), remaining_(length), readFunc_(func) {
    }

    int read(char* data, size_t size) override {
        CHECK(peek(data, size));
        return skip(size);
    }

    int peek(char* data, size_t size) override {
        CHECK_TRUE(readFunc_, SYSTEM_ERROR_INVALID_STATE);
        if (!remaining_) {
            return SYSTEM_ERROR_END_OF_STREAM;
        }
        size = std::min(size, remaining_);
        CHECK(readFunc_(address_ + offset_, (uint8_t*)data, size));
        return size;
    }

    int skip(size_t size) override {
        if (!remaining_) {
            return SYSTEM_ERROR_END_OF_STREAM;
        }
        size = std::min(size, remaining_);
        offset_ += size;
        remaining_ -= size;
        LED_Toggle(PARTICLE_LED_RGB);
        return size;
    }

    int availForRead() override {
        return remaining_;
    }

    int waitEvent(unsigned flags, unsigned timeout) override {
        if (!flags) {
            return 0;
        }
        if (!(flags & InputStream::READABLE)) {
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }
        if (!remaining_) {
            return SYSTEM_ERROR_END_OF_STREAM;
        }
        return InputStream::READABLE;
    }

private:
    uintptr_t address_;
    size_t offset_;
    size_t remaining_;
    ReadStreamFunc readFunc_;
};

int getWifiNcpFirmwareVersion(uint16_t* ncpVersion) {
    uint16_t version = 0;
    int r = particle::wifiNcpGetCachedModuleVersion(&version);
    if (!r && version > 0) {
        *ncpVersion = version;
        return 0;
    }

    // Not present in cache or caching not supported, call into NCP client
    const auto ncpClient = particle::wifiNetworkManager()->ncpClient();
    SPARK_ASSERT(ncpClient);
    const particle::NcpClientLock lock(ncpClient);
    // const particle::NcpPowerState ncpPwrState = ncpClient->ncpPowerState();
    CHECK(ncpClient->on());
    CHECK(ncpClient->getFirmwareModuleVersion(&version));
    *ncpVersion = version;
    // This is now taken care of by Esp32NcpNetif, leaving here
    // just for reference when backporting to 2.x LTS with slightly
    // less smart behavior.
    // if (ncpPwrState == particle::NcpPowerState::OFF) {
    //     CHECK(ncpClient->off());
    // }

    return 0;
}

} // anonymous

int platform_ncp_update_module(const hal_module_t* module) {
    const auto ncpClient = particle::wifiNetworkManager()->ncpClient();
    SPARK_ASSERT(ncpClient);
    OtaUpdateSourceStream::ReadStreamFunc readCallback;
    if (module->bounds.location == MODULE_BOUNDS_LOC_INTERNAL_FLASH) {
        readCallback = hal_flash_read;
    } else if (module->bounds.location == MODULE_BOUNDS_LOC_EXTERNAL_FLASH) {
        readCallback = hal_exflash_read;
    } else {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    // Holding a lock for the whole duration of the operation, otherwise the netif may potentially poweroff the NCP
    const particle::NcpClientLock lock(ncpClient);
    CHECK(ncpClient->on());
    // we pass only the actual binary after the module info and up to the suffix
    uint32_t startAddress = module->bounds.start_address + module->module_info_offset;
    static_assert(sizeof(module_info_t)==24, "expected module info size to be 24");
    const unsigned length = uint32_t(module->info.module_end_address) - uint32_t(module->info.module_start_address);
    OtaUpdateSourceStream moduleStream(startAddress, sizeof(module_info_t), length, readCallback);
    uint16_t version = 0;
    int r = ncpClient->getFirmwareModuleVersion(&version);
    if (r == 0) {
        LOG(INFO, "Updating ESP32 firmware from version %d to version %d", version, module->info.module_version);
    }
    particle::wifiNcpInvalidateInfoCache();
    r = ncpClient->updateFirmware(&moduleStream, length);
    LED_On(PARTICLE_LED_RGB);
    CHECK(r);
    r = ncpClient->getFirmwareModuleVersion(&version);
    if (r == 0) {
        LOG(INFO, "ESP32 firmware version updated to version %d", version);
    }
    return HAL_UPDATE_APPLIED;
}

int platform_ncp_fetch_module_info(hal_module_t* module) {
    uint16_t version = 0;
    // Defaults to zero in case of failure
    int ret = getWifiNcpFirmwareVersion(&version);

    // todo - we could augment the getFirmwareModuleVersion command to retrieve more details
    module_info_t* info = &module->info;
    info->module_version = version;
    info->platform_id = PLATFORM_ID;
    info->module_function = MODULE_FUNCTION_NCP_FIRMWARE;

    // assume all checks pass since it was validated when being flashed to the NCP
    module->validity_checked = MODULE_VALIDATION_RANGE | MODULE_VALIDATION_DEPENDENCIES |
            MODULE_VALIDATION_PLATFORM | MODULE_VALIDATION_INTEGRITY;
    module->validity_result = module->validity_checked;

    // IMPORTANT: a valid suffix with SHA is required for the communication layer to detect a change
    // in the SYSTEM DESCRIBE state and send a HELLO after the NCP update to
    // cause the DS to request new DESCRIBE info
    module_info_suffix_t* suffix = &module->suffix;
    memset(suffix, 0, sizeof(module_info_suffix_t));

    // FIXME: NCP firmware should return some kind of a unique string/hash
    // For now we simply fill the SHA field with version
    for (uint16_t* sha = (uint16_t*)suffix->sha;
            sha < (uint16_t*)(suffix->sha + sizeof(suffix->sha));
            ++sha) {
        *sha = version;
    }

    module->module_info_offset = 0;

    return ret;
}
