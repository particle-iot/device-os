
#include "dynalib.h"
#include "module_system_part1_init.h"
#include "module_user_init.h"
#include <stdint.h>
#include <stddef.h>

DYNALIB_TABLE_EXTERN(hal);
DYNALIB_TABLE_EXTERN(rt);
DYNALIB_TABLE_EXTERN(system);

/**
 * The module export table. This lists the addresses of individual library dynalib jump tables.
 */
const void* const system_part2_module[] = {
    DYNALIB_TABLE_NAME(hal),
    DYNALIB_TABLE_NAME(rt),
    DYNALIB_TABLE_NAME(system),
};

extern void** dynalib_location_user;

uint8_t is_user_function_valid(uint8_t index) {
    size_t fn = (size_t)dynalib_location_user[index];
    return fn > (size_t)&dynalib_location_user && fn <= (size_t)0x80A00000;
}

/**
 * Determines if the user module is present and valid.
 * @return 
 */
uint8_t is_user_module_valid()
{
    // todo - CRC check the user module
    return is_user_function_valid(0) && is_user_function_valid(1);
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
    if (is_user_module_valid()) {
        void* new_heap_top = module_user_pre_init();
        if (new_heap_top>sbrk_heap_top)
            sbrk_heap_top = new_heap_top;
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

__attribute__((section(".module_pre_init"))) const void* system_part2_pre_init_fn = system_part2_pre_init;
__attribute__((section(".module_init"))) const void* system_part2_init_fn = system_part2_init;