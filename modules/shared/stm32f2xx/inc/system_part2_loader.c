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
void system_part2_pre_init() {
    // initialize dependent modules
#if defined(MODULE_HAS_SYSTEM_PART3) && MODULE_HAS_SYSTEM_PART3
    module_system_part3_pre_init();
#endif
    module_system_part1_pre_init();

    HAL_Core_Config();

#if PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION
    // Electron's firmware still contains an embedded bootloader image, so there's no need to
    // check that dependency on the bootloader is satisfied
    const bool bootloader_validated = true;
#else
    const bool bootloader_validated = HAL_Core_Validate_Modules(1, NULL);
#endif

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

    // now call any C++ constructors in this module's dependencies

#if defined(MODULE_HAS_SYSTEM_PART3) && MODULE_HAS_SYSTEM_PART3
    module_system_part3_init();
#endif
    module_system_part1_init();
}

/*
 * Invoked after all module-scope instances have been constructed.
 */
void system_part2_init() {
}

void system_part2_post_init() {
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

extern void* link_global_data_initial_values;
extern void* link_global_data_start;
extern void* link_global_data_end;
extern void* link_bss_location;
extern void* link_bss_end;
extern void* link_end_of_static_ram;

#define link_global_data_size ((size_t)&link_global_data_end - (size_t)&link_global_data_start)
#define link_bss_size ((size_t)&link_bss_end - (size_t)&link_bss_location)

/*
 * Static data of this module is normally initialized by the startup code (e.g. startup_stm32f2xx.S),
 * but on certain platforms we also need to initialize it separately in order to support dynamically
 * loaded functions in the bootloader (see bootloader/src/photon/bootloader_dct.c).
 */
void* module_system_part2_pre_init() {
    if ((&link_global_data_start != &link_global_data_initial_values) && (link_global_data_size != 0)) {
        memcpy(&link_global_data_start, &link_global_data_initial_values, link_global_data_size);
    }
    memset(&link_bss_location, 0, link_bss_size);
    return link_end_of_static_ram;
}

__attribute__((externally_visible, section(".module_pre_init"))) const void* system_part2_pre_init_fn = (const void*)system_part2_pre_init;
__attribute__((externally_visible, section(".module_init"))) const void* system_part2_init_fn = (const void*)system_part2_init;

