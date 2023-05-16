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

#include "deviceid_hal.h"
#include "exflash_hal.h"
#include "str_util.h"
#include "system_error.h"
#include "check.h"
#include "bytes2hexbuf.h"
#ifndef HAL_DEVICE_ID_NO_DCT
#include "dct.h"
#endif /* HAL_DEVICE_ID_NO_DCT */
#include "module_info.h"
#include "core_hal.h"

#include <algorithm>
#include <memory>

extern "C" {
#include "rtl8721d.h"
#include "rtl8721d_efuse.h"
}

#include "platform_ncp.h"

namespace {

using namespace particle;

const uint8_t DEVICE_ID_PREFIX[] = {0x0a, 0x10, 0xac, 0xed, 0x20, 0x21};

} // Anonymous

int readLogicalEfuse(uint32_t offset, uint8_t* buf, size_t size) {
#if MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    std::unique_ptr<uint8_t[]> efuseData(new uint8_t[LOGICAL_EFUSE_SIZE]);
    CHECK_TRUE(efuseData, SYSTEM_ERROR_NO_MEMORY);
    uint8_t* efuseBuf = efuseData.get();
#else
    // No heap in bootloader
    static uint8_t efuseBuf[LOGICAL_EFUSE_SIZE];
#endif // MODULE_FUNCTION != MOD_FUNC_BOOTLOADER

    memset(efuseBuf, 0xff, LOGICAL_EFUSE_SIZE);

    bool dataConsistent = false;
    for (int i = 0; i < 5 && !dataConsistent; i++) {
        EFUSE_LMAP_READ(efuseBuf);
        uint32_t crc1 = HAL_Core_Compute_CRC32(efuseBuf, LOGICAL_EFUSE_SIZE);

        EFUSE_LMAP_READ(efuseBuf);
        uint32_t crc2 = HAL_Core_Compute_CRC32(efuseBuf, LOGICAL_EFUSE_SIZE);

        dataConsistent = (crc1 == crc2);
    }

    if (!dataConsistent) {
        return SYSTEM_ERROR_INTERNAL;
    }
    memcpy(buf, efuseBuf + offset, size);

    return SYSTEM_ERROR_NONE;
};

unsigned hal_get_device_id(uint8_t* dest, unsigned destLen) {
    // Device ID is composed of prefix and MAC address
    uint8_t id[HAL_DEVICE_ID_SIZE] = {};
    memcpy(id, DEVICE_ID_PREFIX, DEVICE_ID_PREFIX_SIZE);
    CHECK_RETURN(readLogicalEfuse(WIFI_MAC_OFFSET, id + DEVICE_ID_PREFIX_SIZE, HAL_DEVICE_ID_SIZE - DEVICE_ID_PREFIX_SIZE), 0);
    if (dest && destLen > 0) {
        memcpy(dest, id, std::min(destLen, sizeof(id)));
    }
    return HAL_DEVICE_ID_SIZE;
}

unsigned hal_get_platform_id() {
    return PLATFORM_ID;
}

int HAL_Get_Device_Identifier(const char** name, char* buf, size_t buflen, unsigned index, void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_get_mac_address(uint8_t type, uint8_t* dest, size_t destLen, void* reserved) {
    CHECK_TRUE(dest && destLen >= HAL_DEVICE_MAC_ADDR_SIZE, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(isSupportedMacType(type), SYSTEM_ERROR_INVALID_ARGUMENT);
    uint8_t mac[HAL_DEVICE_MAC_ADDR_SIZE] = {};
    if (type == HAL_DEVICE_MAC_BLE) {
        CHECK(readLogicalEfuse(BLE_MAC_OFFSET, mac, HAL_DEVICE_MAC_ADDR_SIZE));
    } else {
        CHECK(readLogicalEfuse(WIFI_MAC_OFFSET, mac, HAL_DEVICE_MAC_ADDR_SIZE));
        // Derive the final MAC address
        mac[5] += type;
    }
    memcpy(dest, mac, std::min(destLen, sizeof(mac)));
    return HAL_DEVICE_MAC_ADDR_SIZE;
}

int hal_get_device_serial_number(char* str, size_t size, void* reserved) {
    char fullSerialNumber[HAL_DEVICE_SERIAL_NUMBER_SIZE] = {};

    // Serial Number (15 chars) is comprised of:
    // "Serial Number Front Part" (9 bytes): Product Code (4 chars), Manufacturer ID (2 chars), Year (1 chars), Week (2 chars)
    // and Unique Code (6 hex chars) 
    // We save the front part (9 bytes) in the logical efuse during manufacturing provisioning,
    // the Unique Code is the 24 non-OUI bits of the MAC address, represented as a hex string
    
    CHECK(readLogicalEfuse(SERIAL_NUMBER_OFFSET, (uint8_t*)fullSerialNumber, SERIAL_NUMBER_FRONT_PART_SIZE));

    // generate hex string from non-OUI MAC bytes
    uint8_t wifiMacRandomBytes[HAL_DEVICE_MAC_ADDR_SIZE - WIFI_OUID_SIZE] = {};
    CHECK(readLogicalEfuse(WIFI_MAC_OFFSET + WIFI_OUID_SIZE, wifiMacRandomBytes, sizeof(wifiMacRandomBytes)));
    bytes2hexbuf(wifiMacRandomBytes, sizeof(wifiMacRandomBytes), &fullSerialNumber[SERIAL_NUMBER_FRONT_PART_SIZE]);

    if (!isPrintable(fullSerialNumber, sizeof(fullSerialNumber))) {
        return SYSTEM_ERROR_INTERNAL;
    }
    if (str) {
        memcpy(str, fullSerialNumber, std::min(size, sizeof(fullSerialNumber)));
        // Ensure the output is null-terminated
        if (sizeof(fullSerialNumber) < size) {
            str[sizeof(fullSerialNumber)] = '\0';
        }
    }

    return HAL_DEVICE_SERIAL_NUMBER_SIZE;
}

int hal_get_device_hw_version(uint32_t* revision, void* reserved) {
    // HW Data format: | NCP_ID (LSB) | HW_VERSION | HW Feature Flags |
    //                 |    byte 0    |   byte 1   |    byte 2/3      |
    uint8_t hw_data[4] = {};
    CHECK(readLogicalEfuse(HARDWARE_DATA_OFFSET, (uint8_t*)hw_data, HARDWARE_DATA_SIZE));
    if (hw_data[1] == 0xFF) {
        return SYSTEM_ERROR_BAD_DATA;
    }
    *revision = hw_data[1];
    return SYSTEM_ERROR_NONE;
}

int hal_get_device_hw_model(uint32_t* model, uint32_t* variant, void* reserved) {
    // HW Model format: | Model Number LSB | Model Number MSB | Model Variant LSB | Model Variant MSB |
    //                  |      byte 0      |      byte 1      |      byte 2       |      byte 3       |
    uint8_t hw_model[4] = {};
    CHECK(readLogicalEfuse(HARDWARE_MODEL_OFFSET, (uint8_t*)hw_model, HARDWARE_MODEL_SIZE));
    // Model and variant values of 0xFFFF are acceptable
    *model = ((uint32_t)hw_model[1] << 8) | (uint32_t)hw_model[0];
    *variant = ((uint32_t)hw_model[3] << 8) | (uint32_t)hw_model[2];
    return SYSTEM_ERROR_NONE;
}

int hal_get_device_hw_info(hal_device_hw_info* info, void* reserved) {
    CHECK_TRUE(info, SYSTEM_ERROR_INVALID_ARGUMENT);
    // HW Data format: | NCP_ID (LSB) | HW_VERSION | HW Feature Flags |
    //                 |    byte 0    |   byte 1   |    byte 2/3      |
    uint8_t hw_data[4] = {};
    CHECK(readLogicalEfuse(HARDWARE_DATA_OFFSET, (uint8_t*)hw_data, sizeof(hw_data)));
    CHECK(hal_get_device_hw_model(&info->model, &info->variant, nullptr));
    info->revision = hw_data[1];
    if (info->revision == 0xff) {
        info->revision = 0xffffffff;
    }
    info->features = ((uint32_t)hw_data[3] << 8) | (uint32_t)hw_data[2];
    memset(info->ncp, 0xff, sizeof(info->ncp));
    for (int i = 0; i < HAL_PLATFORM_NCP_COUNT; i++) {
        PlatformNCPInfo ncpInfo = {};
        if (!platform_ncp_get_info(i, &ncpInfo)) {
            info->ncp[i] = (uint16_t)ncpInfo.identifier;
        }
    }
    return 0;
}

#ifndef HAL_DEVICE_ID_NO_DCT
int hal_get_device_secret(char* data, size_t size, void* reserved) {
    // Check if the device secret data is initialized in the DCT
    char secret[HAL_DEVICE_SECRET_SIZE] = {};
    static_assert(sizeof(secret) == DCT_DEVICE_SECRET_SIZE, "");
    int ret = dct_read_app_data_copy(DCT_DEVICE_SECRET_OFFSET, secret, sizeof(secret));
    if (ret < 0) {
        return ret;
    }
    if (!isPrintable(secret, sizeof(secret))) {
        // Check the OTP memory
        CHECK(readLogicalEfuse(MOBILE_SECRET_OFFSET, (uint8_t*)secret, sizeof(secret)));
        if (!isPrintable(secret, sizeof(secret))) {
            return SYSTEM_ERROR_NOT_FOUND;
        };
    }
    if (data && size > 0) {
        memcpy(data, secret, std::min(size, sizeof(secret)));
        if (size > HAL_DEVICE_SECRET_SIZE) {
            data[HAL_DEVICE_SECRET_SIZE] = 0;
        }
    }
    return HAL_DEVICE_SECRET_SIZE;
}

int hal_set_device_secret(char* data, size_t size, void* reserved) {
    uint8_t blankSecret[DCT_DEVICE_SECRET_SIZE] = {0};
    if ((data == nullptr) || (size == 0)) {
        dct_write_app_data(blankSecret, DCT_DEVICE_SECRET_OFFSET, sizeof(blankSecret));
        return SYSTEM_ERROR_NONE;
    }

    if (size != HAL_DEVICE_SECRET_SIZE || !isPrintable(data, HAL_DEVICE_SECRET_SIZE)) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    int ret = dct_write_app_data(data, DCT_DEVICE_SECRET_OFFSET, size);
    if (ret < 0) {
        dct_write_app_data(blankSecret, DCT_DEVICE_SECRET_OFFSET, sizeof(blankSecret));
    }
    return ret;
}

#endif /* HAL_DEVICE_ID_NO_DCT */

