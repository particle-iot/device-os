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

test(slo_startup_stats)
{    
    // get millis_to_connected
    static unsigned long base_time = millis();
    Particle.connect();
    waitUntil(Particle.connected);
    static unsigned long connected_checkpoint = millis();
    static unsigned long millis_to_connected = connected_checkpoint - base_time;
    
    // get free_mem
    uint32_t free_mem = System.freeMemory();
    
    // get the total in-flash size of the sample application (from which "free flash" is
    // implied)
    size_t app_flash_size =
        FLASH_ModuleLength(FLASH_INTERNAL, USER_FIRMWARE_IMAGE_LOCATION) +
        sizeof(uint32_t);

    String stats =
        String::format("{\"millis_to_connect\": %u, \"free_mem\": %u, \"app_flash_size\": %u }",
                        millis_to_connected, free_mem, app_flash_size);
    Particle.publish("startup_stats", stats);

}