#include "platforms.h"
#include "ota_flash_hal.h"
#include "flash_mal.h"
#include "module_info.h"

#include <string.h>

#define MODULE_VERSION_V1_3_0 1320

extern void malloc_enable(uint8_t);
extern void malloc_set_heap_start(void*);
extern void malloc_set_heap_end(void*);
extern void* malloc_heap_start();

extern void** dynalib_location_user;
extern const module_bounds_t module_system_part1;
extern const module_bounds_t module_system_part3;

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
void system_part2_pre_init() {
    // initialize dependent modules
#if defined(MODULE_HAS_SYSTEM_PART3) && MODULE_HAS_SYSTEM_PART3
    // Starting from Device OS 1.2.0, module_system_part3_pre_init() on Electron returns the
    // start address of the module's static RAM region. Device OS 1.3.0 relocates that region
    // so that its start address also signifies the end address of the heap. The same applies
    // to module_system_part1_pre_init() on Photon
    void* const heap_end = module_system_part3_pre_init();
    module_system_part1_pre_init();
#else
    void* const heap_end = module_system_part1_pre_init();
#endif

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
        void* heap_start = module_user_pre_init();
        if (heap_start > malloc_heap_start()) {
            malloc_set_heap_start(heap_start);
        }
    }
    else {
        // indicate to the system that it shouldn't run user code
        set_system_mode(SAFE_MODE);
    }

    // Override the end address of the heap depending on the module version of the system part 3
    // on Electron and system part 1 on Photon
#if defined(MODULE_HAS_SYSTEM_PART3) && MODULE_HAS_SYSTEM_PART3
    const uintptr_t module_addr = module_system_part3.start_address;
#else
    const uintptr_t module_addr = module_system_part1.start_address;
#endif
    const module_info_t* const module_info = FLASH_ModuleInfo(FLASH_INTERNAL, module_addr);
    if (module_info->module_version >= MODULE_VERSION_V1_3_0 && heap_end > malloc_heap_start()) {
        malloc_set_heap_end(heap_end);
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

