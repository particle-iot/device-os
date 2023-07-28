/*
 * Copyright (c) 2022 Particle Industries, Inc.  All rights reserved.
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
#define PARTICLE_USE_UNSTABLE_API

#include "ota_flash_hal.h"
#include "application.h"
#include "unit-test/unit-test.h"

#include "storage_hal.h"
#include "scope_guard.h"
#include "str_util.h"
#include "dct.h"
#include "flash_common.h"

#if HAL_PLATFORM_NRF52840

namespace {

retained char origAppHash[65] = {}; // Hex-encoded

uint8_t flashBuffer[sFLASH_PAGESIZE]; // Buffer for copying images from OTA -> FACTORY slots one page at a time

bool getAppHash(char* buf, size_t size) {
    hal_system_info_t info = {};
    info.size = sizeof(info);
    const int r = system_info_get_unstable(&info, 0 /* flags */, nullptr /* reserved */);
    if (r != 0) {
        return false;
    }
    SCOPE_GUARD({
        system_info_free_unstable(&info, nullptr /* reserved */);
    });
    for (size_t i = 0; i < info.module_count; ++i) {
        const auto& module = info.modules[i];
        if (module.info.module_function == MODULE_FUNCTION_USER_PART) {
            toHex(module.suffix.sha, sizeof(module.suffix.sha), buf, size);
            return true;
        }
    }
    return false;
}

bool getPendingOtaModule(platform_flash_modules_t* otaModule, uint8_t* moduleIndex) {
    platform_flash_modules_t flash_modules[MAX_MODULES_SLOT];
    uint8_t flash_module_index = 0;
    dct_read_app_data_copy(DCT_FLASH_MODULES_OFFSET, flash_modules, sizeof(flash_modules));

    for (flash_module_index = 0; flash_module_index < MAX_MODULES_SLOT; flash_module_index++) {
        platform_flash_modules_t module = flash_modules[flash_module_index];
        if (module.magicNumber == 0xABCD && module.module_function == MODULE_FUNCTION_USER_PART) {
            *otaModule = module;
            *moduleIndex = flash_module_index;
            return true;
        }
    }
    return false;
}

bool getFactoryModule(hal_module_t* factoryModule) {
    // Search the platform modules for the factory module 
    hal_system_info_t info = {};
    info.size = sizeof(info);
    const int r = system_info_get_unstable(&info, 0 /* flags */, nullptr /* reserved */);
    if (r != 0) {
        return false;
    }
    SCOPE_GUARD({
        system_info_free_unstable(&info, nullptr /* reserved */);
    });

    for (size_t i = 0; i < info.module_count; ++i) {
        const auto& module = info.modules[i];
        if (module.bounds.store == MODULE_STORE_FACTORY) {
            *factoryModule = module;
            return true;
        }
    }
    return false;
}

bool InvalidateDctModuleSlot(int moduleOffset)
{
    // Mark slot as unused
    const size_t magic_num_offs = DCT_FLASH_MODULES_OFFSET + sizeof(platform_flash_modules_t) * moduleOffset + \
            offsetof(platform_flash_modules_t, magicNumber);

    const uint16_t magic_num = 0xffff;
    return (dct_write_app_data(&magic_num, magic_num_offs, sizeof(magic_num)) == 0);
}

} // namespace

void disable_resets_and_connect() {
    #if HAL_PLATFORM_COMPRESSED_OTA
        // Disable compressed OTA updates so that it's easier to mess with module checksums
        spark_protocol_set_connection_property(spark_protocol_instance(), protocol::Connection::COMPRESSED_OTA,
                0 /* value */, nullptr /* data */, nullptr /* reserved */);
    #endif
        System.disableReset();
        Particle.connect();
        waitUntil(Particle.connected);
}

void flash_binary() {
    // Invalidate any existing factory reset and pending OTA apps
    InvalidateDctModuleSlot(FAC_RESET_SLOT);

    platform_flash_modules_t otaModule = {};
    uint8_t otaModuleIndex = 0;
    if(getPendingOtaModule(&otaModule, &otaModuleIndex)) {
        InvalidateDctModuleSlot(otaModuleIndex);
    }

    // See: Host side OTA flashes modified factory_reset test binary

    // Get the original app hash prior to sending any updates to the device
    assertTrue(getAppHash(origAppHash, sizeof(origAppHash)));
}

void move_ota_binary_to_factory_slot() {
    // Use DCT to confirm there is a pending OTA
    platform_flash_modules_t otaModule = {};
    uint8_t otaModuleIndex = 0;
    assertTrue(getPendingOtaModule(&otaModule, &otaModuleIndex));
    assertNotEqual(otaModuleIndex, 0);

    // Determine the factory reset module start address from the platform flash modules
    hal_module_t factoryModule = {};
    assertTrue(getFactoryModule(&factoryModule));

    // Copy the OTA image from the OTA Module location to the Factory Firmware Module location
    int bytesRemaining = otaModule.length; // already includes crc32 trailer
    int bytesCopied = 0;

    uint32_t factoryModuleAddress = factoryModule.bounds.start_address;
    uint32_t factoryImageSizeSectorAligned = CEIL_DIV(bytesRemaining, sFLASH_PAGESIZE) * sFLASH_PAGESIZE;

    // Erase Factory Module in order to allow copy of OTA image
    assertEqual(hal_storage_erase(HAL_STORAGE_ID_EXTERNAL_FLASH, factoryModuleAddress, factoryImageSizeSectorAligned), factoryImageSizeSectorAligned);

    while (bytesRemaining) {
        int bytesToCopy = bytesRemaining > sFLASH_PAGESIZE ? sFLASH_PAGESIZE : bytesRemaining;

        assertEqual(hal_storage_read(HAL_STORAGE_ID_EXTERNAL_FLASH, otaModule.sourceAddress + bytesCopied, flashBuffer, bytesToCopy), bytesToCopy); 
        assertEqual(hal_storage_write(HAL_STORAGE_ID_EXTERNAL_FLASH, factoryModuleAddress + bytesCopied, flashBuffer, bytesToCopy), bytesToCopy);

        bytesCopied += bytesToCopy;
        bytesRemaining -= bytesToCopy;
    }

    // Invalidate the OTA image in DCT so it doesnt get applied instead of Factory Reset when rebooting
    InvalidateDctModuleSlot(otaModuleIndex); 
    
    // Update DCT entry to denote a valid pending Factory Firmware module
    platform_flash_modules_t factoryModuleDCT = {};
    factoryModuleDCT.sourceDeviceID = FLASH_SERIAL;
    factoryModuleDCT.sourceAddress = EXTERNAL_FLASH_FAC_ADDRESS;
    factoryModuleDCT.destinationDeviceID = FLASH_INTERNAL;
    factoryModuleDCT.destinationAddress = USER_FIRMWARE_IMAGE_LOCATION;
    factoryModuleDCT.length = otaModule.length; // INCLUDES CRC
    factoryModuleDCT.magicNumber = 0x0FAC;
    factoryModuleDCT.module_function = FACTORY_RESET_MODULE_FUNCTION;
    factoryModuleDCT.flags = MODULE_VERIFY_CRC|MODULE_VERIFY_FUNCTION|MODULE_VERIFY_DESTINATION_IS_START_ADDRESS;
 
    assertEqual(dct_write_app_data(&factoryModuleDCT, DCT_FLASH_MODULES_OFFSET + (FAC_RESET_SLOT * sizeof(platform_flash_modules_t)), sizeof(platform_flash_modules_t)), 0);
}

void validate_factory_reset_worked() {
    char appHash[65] = {};
    assertTrue(getAppHash(appHash, sizeof(appHash)));

    // The app hash SHOULD have changed
    assertNotEqual(strcmp(appHash, origAppHash), 0);
}

test(01_disable_resets_and_connect) {
    disable_resets_and_connect();
}

test(02_flash_binary) {
    flash_binary();
}

test(03_move_ota_binary_to_factory_slot) {
    move_ota_binary_to_factory_slot();
};

test(04_device_factory_reset) {
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 5000));
    System.factoryReset();
}

test(05_validate_factory_reset_worked) {
    validate_factory_reset_worked();
}

// Repeat for USB triggered factory reset

test(06_disable_resets_and_connect) {
    disable_resets_and_connect();
}

test(07_flash_binary) {
    flash_binary();
}

test(08_move_ota_binary_to_factory_slot) {
    move_ota_binary_to_factory_slot();
};

test(09_usb_command_factory_reset) {
    // Test runner will factory reset the device
}

test(10_validate_factory_reset_worked) {
    validate_factory_reset_worked();
}

#endif // HAL_PLATFORM_NRF52840
