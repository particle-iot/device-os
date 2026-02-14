///
// This code meant to be a direct port of the canonical startup stats firmware: https://github.com/particle-iot/pqa/blob/main/firmware/publish-startup-stats-once/src/main.cpp
// Since this test framework does not have a setup() + loop() function, it is somewhat different.
///
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

} // anonymous

void PRE_STARTUP() {
    globalInitTimeFromPreStartup = millis();
}

STARTUP({
    globalInitTimeFromStartup = millis();
    testAppInit();
    testAppInitDuration = millis() - globalInitTimeFromStartup;
});

void setup() {
    setupTimeFromStartup = millis();
    testAppSetup();
    testAppSetupDuration = millis() - setupTimeFromStartup;
    setupTimeFromStartup -= testAppInitDuration;
}

void loop() {
    if (!loopCalled) {
        loopTimeFromStartup = millis() - testAppInitDuration - testAppSetupDuration;
        loopCalled = true;
    }
    testAppLoop();
}

test(slo_startup_stats) {
    Particle.connect();
    waitFor(Particle.connected, 10 * 60 * 1000);
    
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

    Particle.publish("startup_stats", stats.toJSON());
}
