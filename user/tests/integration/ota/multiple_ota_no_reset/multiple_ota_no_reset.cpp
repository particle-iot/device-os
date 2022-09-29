#define PARTICLE_USE_UNSTABLE_API

#include "application.h"
#include "test.h"

#include "storage_hal.h"
#include "scope_guard.h"
#include "str_util.h"

namespace {

#if HAL_PLATFORM_NRF52840
const auto OTA_SECTION_STORAGE = HAL_STORAGE_ID_EXTERNAL_FLASH;
const auto OTA_SECTION_ADDRESS = EXTERNAL_FLASH_OTA_ADDRESS;
#elif HAL_PLATFORM_RTL872X
const auto OTA_SECTION_STORAGE = HAL_STORAGE_ID_INTERNAL_FLASH;
uintptr_t OTA_SECTION_ADDRESS = 0; // Runtime
#else
#error "Unsupported platform"
#endif

retained char origAppHash[65] = {}; // Hex-encoded

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

#if HAL_PLATFORM_RTL872X
uintptr_t getOtaAddressRtl() {
    hal_system_info_t info = {};
    info.size = sizeof(info);
    const int r = system_info_get_unstable(&info, 0 /* flags */, nullptr /* reserved */);
    if (r != 0) {
        return 0;
    }
    SCOPE_GUARD({
        system_info_free_unstable(&info, nullptr /* reserved */);
    });
    for (size_t i = 0; i < info.module_count; ++i) {
        const auto& module = info.modules[i];
        if (module.info.module_function == MODULE_FUNCTION_SYSTEM_PART) {
            uintptr_t end = (uintptr_t)module.info.module_end_address;
            end += 4; // CRC32
            end = ((end) & 0xFFFFF000) + 0x1000; // 4K aligned
            return end;
        }
    }
    return 0;
}
#endif // HAL_PLATFORM_RTL872X

} // namespace

test(01_disable_resets_and_connect) {
#if HAL_PLATFORM_COMPRESSED_OTA
    // Disable compressed OTA updates so that it's easier to mess with module checksums
    spark_protocol_set_connection_property(spark_protocol_instance(), protocol::Connection::COMPRESSED_OTA,
            0 /* value */, nullptr /* data */, nullptr /* reserved */);
#endif
    System.disableReset();
    Particle.connect();
    waitUntil(Particle.connected);
}

test(02_flash_binaries) {
    // Get the original app hash prior to sending any updates to the device
    assertTrue(getAppHash(origAppHash, sizeof(origAppHash)));
}

test(03_fix_ota_binary_and_reset) {
    // FIXME: Add workaround for TrackerM units that fail to communicate with QSPI flash chip
    // with an active cellular connection
#if PLATFORM_ID == PLATFORM_TRACKERM
    delay(500);
#endif
    // The original issue (https://github.com/particle-iot/device-os/pull/2346) can't be reproduced
    // easily by flashing application binaries. As a workaround, we're restoring the correct platform
    // ID in the binary that is currently stored in the OTA section so that the bootloader can apply
    // it if there still happens to be a valid module slot in the DCT
    const uint16_t id = PLATFORM_ID;
#if HAL_PLATFORM_RTL872X
    OTA_SECTION_ADDRESS = getOtaAddressRtl();
    assertNotEqual(OTA_SECTION_ADDRESS, (uintptr_t)0);
#endif // HAL_PLATFORM_RTL872X
    assertEqual(hal_storage_write(OTA_SECTION_STORAGE, OTA_SECTION_ADDRESS + offsetof(module_info_t, platform_id),
            (const uint8_t*)&id, sizeof(id)), sizeof(id));
}

test(04_validate_module_info) {
    char appHash[65] = {};
    assertTrue(getAppHash(appHash, sizeof(appHash)));
    // The app hash should not have changed
    assertEqual(strcmp(appHash, origAppHash), 0);
}
