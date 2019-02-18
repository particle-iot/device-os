#include "platform_ncp.h"
#include "network/ncp.h"
#include "wifi_network_manager.h"
#include "wifi_ncp_client.h"
#include "led_service.h"
#include "check.h"
#include "scope_guard.h"
#include "system_led_signal.h"
#include "stream.h"
#include "debug.h"
#include "ota_flash_hal_impl.h"

#include <algorithm>

MeshNCPIdentifier platform_current_ncp_identifier() {
    return MESH_NCP_ESP32;
}

class OtaUpdateSourceStream : public particle::InputStream {
public:
    OtaUpdateSourceStream(const uint8_t* buffer, size_t length) : buffer(buffer), remaining(length) {}

    int read(char* data, size_t size) override {
        CHECK(peek(data, size));
        return skip(size);
    }

    int peek(char* data, size_t size) override {
        if (!remaining) {
            return SYSTEM_ERROR_END_OF_STREAM;
        }
        size = std::min(size, remaining);
        memcpy(data, buffer, size);
        return size;
    }

    int skip(size_t size) override {
        if (!remaining) {
            return SYSTEM_ERROR_END_OF_STREAM;
        }
        size = std::min(size, remaining);
        buffer += size;
        remaining -= size;
        LED_Toggle(LED_RGB);
        return size;
    }

    int availForRead() override {
        return remaining;
    }

    int waitEvent(unsigned flags, unsigned timeout) override {
        if (!flags) {
            return 0;
        }
        if (!(flags & InputStream::READABLE)) {
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }
        if (!remaining) {
            return SYSTEM_ERROR_END_OF_STREAM;
        }
        return InputStream::READABLE;
    }

private:
    const uint8_t* buffer;
    size_t remaining;
};

// FIXME: This function accesses the module info via XIP and may fail to parse it correctly under
// some not entirely clear circumstances. Disabling compiler optimizations helps to work around
// the problem
__attribute__((optimize("O0"))) hal_update_complete_t platform_ncp_update_module(const hal_module_t* module) {
    const auto ncpClient = particle::wifiNetworkManager()->ncpClient();
    SPARK_ASSERT(ncpClient);
    CHECK_RETURN(ncpClient->on(), HAL_UPDATE_ERROR);
    // we pass only the actual binary after the module info and up to the suffix
    const uint8_t* start = (const uint8_t*)module->info;
    static_assert(sizeof(module_info_t)==24, "expected module info size to be 24");
    start += sizeof(module_info_t); // skip the module info
    const uint8_t* end = start + (uint32_t(module->info->module_end_address) - uint32_t(module->info->module_start_address));
    const unsigned length = end-start;
    OtaUpdateSourceStream moduleStream(start, length);
    uint16_t version = 0;
    int r = ncpClient->getFirmwareModuleVersion(&version);
    if (r == 0) {
        LOG(INFO, "Updating ESP32 firmware from version %d to version %d", version, module->info->module_version);
    }
    r = ncpClient->updateFirmware(&moduleStream, length);
    LED_On(LED_RGB);
    CHECK_RETURN(r, HAL_UPDATE_ERROR);
    r = ncpClient->getFirmwareModuleVersion(&version);
    if (r == 0) {
        LOG(INFO, "ESP32 firmware version updated to version %d", version);
    }
    return HAL_UPDATE_APPLIED;
}

int platform_ncp_fetch_module_info(hal_system_info_t* sys_info, bool create) {
    for (int i=0; i<sys_info->module_count; i++) {
        hal_module_t* module = sys_info->modules + i;
        if (!memcmp(&module->bounds, &module_ncp_mono, sizeof(module_ncp_mono))) {
            if (create) {
                const auto ncpClient = particle::wifiNetworkManager()->ncpClient();
                SPARK_ASSERT(ncpClient);
                CHECK(ncpClient->on());
                uint16_t version;
                int error = ncpClient->getFirmwareModuleVersion(&version);
                if (error) {
                    version = 0;
                }
                // todo - we could augment the getFirmwareModuleVersion command to retrieve more details
                auto info = new module_info_t();
                CHECK_TRUE(info, SYSTEM_ERROR_NO_MEMORY);

                info->module_version = version;
                info->platform_id = PLATFORM_ID;
                info->module_function = MODULE_FUNCTION_NCP_FIRMWARE;

                module->info = info;
                // assume all checks pass since it was validated when being flashed to the NCP
                module->validity_result = module->validity_checked;
            }
            else {
                delete module->info;
            }
        }
    }
    return 0;
}
