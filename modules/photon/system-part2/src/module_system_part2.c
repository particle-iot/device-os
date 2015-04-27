
#include "dynalib.h"
#include "module_system_part1_init.h"
#include "system_mode.h"
#include "module_user_init.h"
#include "core_hal.h"
#include <stdint.h>
#include <stddef.h>

DYNALIB_TABLE_EXTERN(hal);
DYNALIB_TABLE_EXTERN(rt);
DYNALIB_TABLE_EXTERN(system);
DYNALIB_TABLE_EXTERN(system_net);
DYNALIB_TABLE_EXTERN(system_cloud);
DYNALIB_TABLE_EXTERN(hal_peripherals);
DYNALIB_TABLE_EXTERN(hal_i2c);
DYNALIB_TABLE_EXTERN(hal_gpio);
DYNALIB_TABLE_EXTERN(hal_spi);
DYNALIB_TABLE_EXTERN(hal_core);
DYNALIB_TABLE_EXTERN(hal_socket);
DYNALIB_TABLE_EXTERN(hal_wlan);
DYNALIB_TABLE_EXTERN(hal_usart);

/**
 * The module export table. This lists the addresses of individual library dynalib jump tables.
 */
__attribute__((externally_visible)) const void* const system_part2_module[] = {
    DYNALIB_TABLE_NAME(hal),
    DYNALIB_TABLE_NAME(rt),
    DYNALIB_TABLE_NAME(system),
    DYNALIB_TABLE_NAME(hal_peripherals),
    DYNALIB_TABLE_NAME(hal_i2c),
    DYNALIB_TABLE_NAME(hal_gpio),
    DYNALIB_TABLE_NAME(hal_spi),
    DYNALIB_TABLE_NAME(hal_core),
    DYNALIB_TABLE_NAME(hal_socket),
    DYNALIB_TABLE_NAME(hal_wlan),
    DYNALIB_TABLE_NAME(hal_usart),
    DYNALIB_TABLE_NAME(system_net),
    DYNALIB_TABLE_NAME(system_cloud),
};

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
 * The current start of heap.
 */
extern void* sbrk_heap_top;

/**
 * Global initialization function. Called after memory has been initialized in this module
 * but before C++ constructors are executed and before any dynamic memory has been allocated.
 */
void system_part2_pre_init() {
    // initialize dependent modules
    module_system_part1_pre_init();
    
    HAL_Core_Config();
    
    module_user_part_validated = HAL_Core_Validate_User_Module();

    if (is_user_module_valid()) {
        void* new_heap_top = module_user_pre_init();
        if (new_heap_top>sbrk_heap_top)
            sbrk_heap_top = new_heap_top;
    }
    else {
        // indicate to the system that it shouldn't run user code
        set_system_mode(SAFE_MODE);
    }

    // now call any C++ constructors in this module's dependencies
    module_system_part1_init();
}

/*
 * Invoked after all module-scope instances have been constructed.
 */
void system_part2_init() {
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

__attribute__((externally_visible, section(".module_pre_init"))) const void* system_part2_pre_init_fn = system_part2_pre_init;
__attribute__((externally_visible, section(".module_init"))) const void* system_part2_init_fn = system_part2_init;
