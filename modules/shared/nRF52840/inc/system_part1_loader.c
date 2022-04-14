#include "platforms.h"
#include <string.h>
#include "user_hal.h"

extern void** dynalib_location_user;

static hal_user_module_descriptor user_descriptor = {};

/**
 * Determines if the user module is present and valid.
 * @return
 */
bool run_user_module()
{
    return system_mode() != SAFE_MODE;
}

/**
 * Global initialization function. Called after memory has been initialized in this module
 * but before C++ constructors are executed and before any dynamic memory has been allocated.
 */
void system_part1_pre_init() {
    // HAL_Core_Config() has been invoked in startup_nrf52840.S
    if (HAL_Core_Enter_Safe_Mode_Requested()) {
        set_system_mode(SAFE_MODE);
    }
}

/*
 * Invoked after all module-scope instances have been constructed.
 */
void system_part1_init() {
}

void system_part2_post_init() __attribute__((alias("system_part1_post_init")));

void system_part1_post_init() {
    // NOTE: it may fetch the NCP and radio stack version, so some ealy
    // initialization work should have been done before getting here.
    const bool bootloader_validated = HAL_Core_Validate_Modules(1, NULL);
    if (!bootloader_validated || hal_user_module_get_descriptor(&user_descriptor)) {
        // indicate to the system that it shouldn't run user code
        set_system_mode(SAFE_MODE);
    }
    
    if (run_user_module()) {
        user_descriptor.init();
    }
}

void setup() {
    if (run_user_module()) {
        user_descriptor.setup();
    }
}

void loop() {
    if (run_user_module()) {
        user_descriptor.loop();
    }
}

__attribute__((externally_visible, section(".module_pre_init"))) const void* system_part1_pre_init_fn = (const void*)system_part1_pre_init;
__attribute__((externally_visible, section(".module_init"))) const void* system_part1_init_fn = (const void*)system_part1_init;

