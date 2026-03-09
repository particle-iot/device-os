/*
 * Copyright (c) 2025 Particle Industries, Inc.  All rights reserved.
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

#include "application.h"
#include "test.h"
#include "softcrc32.h"
#include "check.h"
#include "storage_hal.h"

namespace {

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

enum class FirmwareUpdateStatus {
    NONE,
    STARTED,
    SUCCESS,
    ERROR
};

auto firmwareUpdateStatus = FirmwareUpdateStatus::NONE;
std::atomic<int> firmwareUpdateProgressCount;

void firmwareUpdateEventHandler(system_event_t, int data, void*) {
    switch (data) {
    case firmware_update_begin:
        Test::out->println("firmware_update_begin");
        firmwareUpdateStatus = FirmwareUpdateStatus::STARTED;
        break;
    case firmware_update_complete:
        Test::out->println("firmware_update_complete");
        firmwareUpdateStatus = FirmwareUpdateStatus::SUCCESS;
        break;
    case firmware_update_progress:
        ++firmwareUpdateProgressCount;
        break;
    default:
        Test::out->printlnf("Unexpected firmware update status: %d", data);
        firmwareUpdateStatus = FirmwareUpdateStatus::ERROR;
        break;
    }
}

void prepareForFirmwareUpdate() {
    System.disableReset();
    System.on(firmware_update, firmwareUpdateEventHandler);
    firmwareUpdateStatus = FirmwareUpdateStatus::NONE;
    firmwareUpdateProgressCount = 0;
}

void completeFirmwareUpdate(bool expectSafeMode = false) {
    bool ok = false;
    auto t1 = millis();
    for (;;) {
        if (firmwareUpdateStatus == FirmwareUpdateStatus::SUCCESS) {
            ok = true;
            break;
        }
        if (firmwareUpdateStatus == FirmwareUpdateStatus::ERROR) {
            Test::out->println("Firmware update failed");
            break;
        }
        // The JS part of the test waits until the OTA completes so the timeout here is for
        // finalizing the update on the device
        if (millis() - t1 >= 30000) {
            Test::out->println("Firmware update timeout");
            break;
        }
    }
    Test::out->printlnf("firmware_update_progress count: %d", firmwareUpdateProgressCount.load());
    System.off(firmware_update);
    if (ok) {
        if (expectSafeMode) {
            TestRunner::instance()->expectSafeMode();
        } else {
            TestRunner::instance()->expectSystemReset();
        }
    }
    assertTrue(ok);
    System.enableReset();
}

} // namespace

test(01_erase_factory_module) {
    // Determine the factory reset module start address from the platform flash modules
    hal_module_t factoryModule = {};
    bool isFactoryModule = getFactoryModule(&factoryModule);
    // Log.info("getFactoryModule(): %d", isFactoryModule);

    if (isFactoryModule) {
#if HAL_PLATFORM_NRF52840 // FIXME for RTL872x-based platforms
        // Erase entire Factory Module
        hal_storage_erase(HAL_STORAGE_ID_EXTERNAL_FLASH, EXTERNAL_FLASH_FAC_ADDRESS, EXTERNAL_FLASH_FAC_LENGTH);
#endif // HAL_PLATFORM_NRF52840
    }
}

test(02_remove_static_ip) {
    // Easiest way to erase all NETWORK_CONFIG settings, and default to dynamic IP
    unlink("/sys/network.dat");
}

test(03_enable_listening_mode) {
    System.disableFeature(FEATURE_DISABLE_LISTENING_MODE);
}

#if HAL_PLATFORM_ENV
test(04_cleanup_env) {
    System.clearEnv(false /* reset */);
    unlink("/sys/env_app");
    unlink("/sys/env_app.staged");
    unlink("/sys/env_snapshot");
    unlink("/sys/env_snapshot.staged");
    expectSystemReset();
    System.reset();
}

test(05_cleanup_env) {
    prepareForFirmwareUpdate();
    Particle.connect();
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
    // We are supposed to get an empty env
}

test(06_cleanup_env) {
    completeFirmwareUpdate();
}
#endif // HAL_PLATFORM_ENV
