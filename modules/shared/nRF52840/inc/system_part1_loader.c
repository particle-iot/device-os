#include "platforms.h"
#include <string.h>

extern void** dynalib_location_user;

static bool module_user_part_validated = false;

/**
 * Determines if the user module is present and valid.
 * @return
 */
bool is_user_module_valid()
{
    return module_user_part_validated;
}

/**
 * Global initialization function. Called after memory has been initialized in this module
 * but before C++ constructors are executed and before any dynamic memory has been allocated.
 */
void system_part1_pre_init() {
    // HAL_Core_Config() has been invoked in startup_nrf52840.S

    const bool bootloader_validated = HAL_Core_Validate_Modules(1, NULL);

    // Validate user module
    if (bootloader_validated) {
        module_user_part_validated = HAL_Core_Validate_User_Module();
    }

    bool safe_mode = HAL_Core_Enter_Safe_Mode_Requested();

    if (!bootloader_validated || !is_user_module_valid() || safe_mode) {
        // indicate to the system that it shouldn't run user code
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
    if (is_user_module_valid()) {
        module_user_init();
    }
}

void setup() {
    if (is_user_module_valid()) {
        module_user_setup();
    }
}

void loop() {
    if (is_user_module_valid()) {
        module_user_loop();
    }
}

__attribute__((externally_visible, section(".module_pre_init"))) const void* system_part1_pre_init_fn = (const void*)system_part1_pre_init;
__attribute__((externally_visible, section(".module_init"))) const void* system_part1_init_fn = (const void*)system_part1_init;

