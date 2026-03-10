#define PARTICLE_USE_UNSTABLE_API

#include "application.h"
#include "test.h"

//required for linking FLASH_ModuleLength on Gen3 to work: 
extern "C" int hal_exflash_read(uintptr_t addr, uint8_t* data_buf, size_t data_size) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

extern "C" int hal_flash_read(uintptr_t addr, uint8_t* buf, size_t size) {
    const void* ptr = (const void*)addr;
    memcpy(buf, ptr, size);
    return SYSTEM_ERROR_NONE;
}

extern uintptr_t link_module_start;

namespace {

system_tick_t globalInitTimeFromPreStartup = 0;
system_tick_t globalInitTimeFromStartup = 0;
system_tick_t setupTimeFromStartup = 0;
system_tick_t loopTimeFromStartup = 0;
bool loopCalled = false;

system_tick_t testAppInitDuration = 0;
system_tick_t testAppSetupDuration = 0;

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
        Particle.process();
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

} // anonymous

void PRE_STARTUP() {
    globalInitTimeFromPreStartup = micros();
}

STARTUP({
    globalInitTimeFromStartup = micros();
    testAppInit();
    testAppInitDuration = micros() - globalInitTimeFromStartup;
});

void setup() {
    setupTimeFromStartup = micros();
    testAppSetup();
    testAppSetupDuration = micros() - setupTimeFromStartup;
    setupTimeFromStartup -= testAppInitDuration;
}

void loop() {
    if (!loopCalled) {
        loopTimeFromStartup = micros() - testAppInitDuration - testAppSetupDuration;
        loopCalled = true;
    }
    testAppLoop();
}

test(01_prepare) {
    // Startup times are affected somewhat by assets and env vars (as they are also a type of an asset)
    // Make sure that they are not present during this test
    System.clearEnv(false /* reset */);
    unlink("/sys/env_app");
    unlink("/sys/env_app.staged");
    unlink("/sys/env_snapshot");
    unlink("/sys/env_snapshot.staged");
    asset_manager_format_storage(nullptr);
    System.disableFeature(FEATURE_ETHERNET_DETECTION); // just in case
    expectSystemReset();
    System.reset();
}

test(02_prepare) {
    expectSystemReset();
    System.reset();
}

test(03_slo_startup_stats) {
    // get free_mem
    uint32_t free_mem = System.freeMemory();
    
    // get the total in-flash size of the sample application (from which "free flash" is
    // implied)
    size_t app_flash_size =
        FLASH_ModuleLength(FLASH_INTERNAL, (uint32_t)&link_module_start) +
        sizeof(uint32_t);

    Variant stats;
    stats.set("free_mem", free_mem);
    stats.set("app_flash_size", app_flash_size);
    Variant time;
    time.set("pre_startup", globalInitTimeFromPreStartup);
    time.set("startup", globalInitTimeFromStartup);
    time.set("setup", setupTimeFromStartup);
    time.set("loop", loopTimeFromStartup);
    stats.set("time", time);

    pushMailboxMsg(stats.toJSON(), 5000);
}

test(98_cleanup) {
    prepareForFirmwareUpdate();
    Particle.disconnect(CloudDisconnectOptions().clearSession(true));
    Particle.connect();
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
    // We are supposed to get an empty env
}

test(99_cleanup) {
    completeFirmwareUpdate();
}
