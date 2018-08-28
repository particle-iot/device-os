#include "platforms.h"

#include <string.h>

extern void** dynalib_location_user;

static bool module_user_part_validated = false;

extern void malloc_enable(uint8_t);
extern void malloc_set_heap_start(void*);
extern void* malloc_heap_start();

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
    // This function has been invoked in startup_nrf52840.S
    //HAL_Core_Config();

    const bool bootloader_validated = HAL_Core_Validate_Modules(1, NULL);

    // Validate user module
    if (bootloader_validated) {
        module_user_part_validated = HAL_Core_Validate_User_Module();
    }

    if (bootloader_validated && is_user_module_valid()) {
        void* new_heap_top = module_user_pre_init();
        if (new_heap_top>malloc_heap_start()) {
            malloc_set_heap_start(new_heap_top);
        }
    }
    else {
        // indicate to the system that it shouldn't run user code
        set_system_mode(SAFE_MODE);
    }

    malloc_enable(1);
}

/*
 * Invoked after all module-scope instances have been constructed.
 */
void system_part1_init() {
}

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

