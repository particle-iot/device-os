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



// test(my_test) {
//     auto t1 = millis();
//     Particle.connect();
//     waitUntil(Particle.connected);
//     auto millis_to_connected = millis() - t1;
// }

test(slo_startup_stats)
{    
    // capture the time at which app initialization takes place
    static unsigned long base_time = millis();
    
    // the time at which we enter setup()
    // static unsigned long setup_checkpoint = 0;

    // how long it took for device OS to run before setup() was called
    // static unsigned long millis_to_setup = 0;

    // simulating the time it takes to "get to setup"
    Particle.connect();
    waitUntil(Particle.connected);

    // this is code that runs "in setup()"
    // setup_checkpoint = millis();
    // millis_to_setup = setup_checkpoint - base_time;
    
    // code below was ported from loop()
    // capture the moment when we connected
    unsigned long connected_checkpoint = millis();
    unsigned long millis_to_connected = connected_checkpoint - base_time;

    uint32_t free_mem = System.freeMemory();
    // get the total in-flash size of the sample application (from which "free flash" is
    // implied)
    size_t app_flash_size =
        FLASH_ModuleLength(FLASH_INTERNAL, USER_FIRMWARE_IMAGE_LOCATION) +
        sizeof(uint32_t);

    String stats =
        // commented out the version with millis_to_setup
        // String::format("{\"millis_to_setup\": %u, \"millis_to_connect\": %u, "
        //                 "\"free_mem\": %u, \"app_flash_size\": %u }",
        //                 millis_to_setup, millis_to_connected, free_mem, app_flash_size);
        String::format("{\"millis_to_connect\": %u, \"free_mem\": %u, \"app_flash_size\": %u }",
                        millis_to_connected, free_mem, app_flash_size);
    Particle.publish("startup_stats", stats);

}