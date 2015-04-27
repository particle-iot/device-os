
#include "module_system_part1.h"
#include <stddef.h>
#include <string.h>

/**
 * An empty no arg, no result function
 */
typedef void  (*constructor_ptr_t)(void);

/**
 * Pointer to the reset handler;
 */
extern void* system_part2_reset_handler_location;
extern char stack_end;

/**
 * No register saving needed.
 */
__attribute__((naked)) void system_part1_reset_handler();

void system_part1_reset_handler() {
    constructor_ptr_t reset_handler = (constructor_ptr_t)*&system_part2_reset_handler_location;
    reset_handler();
}

/**
 * The fake interrupt vectors table that redirects to part2.
 */
__attribute__((externally_visible)) const void* const system_part1_boot_table[97] = {
    &stack_end,
    &system_part1_reset_handler
};

DYNALIB_TABLE_EXTERN(communication);
DYNALIB_TABLE_EXTERN(wifi_resource);
DYNALIB_TABLE_EXTERN(system_module_part1);
DYNALIB_TABLE_EXTERN(services);

__attribute__((externally_visible)) const void* const system_part1_module[] = {
    DYNALIB_TABLE_NAME(communication),
    DYNALIB_TABLE_NAME(wifi_resource),    
    DYNALIB_TABLE_NAME(system_module_part1),    
    DYNALIB_TABLE_NAME(services),    
};


/**
 * Locations of static memory regions from linker.
 */
extern void* link_global_data_initial_values;
extern void* link_global_data_start;
extern void* link_global_data_end;
#define link_global_data_size ((size_t)&link_global_data_end - (size_t)&link_global_data_start)

extern void* link_bss_location;
extern void* link_bss_end;
#define link_bss_size ((size_t)&link_bss_end - (size_t)&link_bss_location)

extern void* link_end_of_static_ram;

void* module_system_part1_pre_init()
{
    if ( (&link_global_data_start!=&link_global_data_initial_values) && (link_global_data_size != 0))
    {
        memcpy(&link_global_data_start, &link_global_data_initial_values, link_global_data_size);
    }

    memset(&link_bss_location, 0, link_bss_size );
    
    return link_end_of_static_ram;
}


/**
 * Array of C++ static constructors.
 */
extern constructor_ptr_t link_constructors_location[];
extern constructor_ptr_t link_constructors_end;
#define link_constructors_size   ((unsigned long)&link_constructors_end  -  (unsigned long)&link_constructors_location )


void module_system_part1_init()
{
    // invoke constructors
    int ctor_num;
    for (ctor_num=0; ctor_num < link_constructors_size/sizeof(constructor_ptr_t); ctor_num++ )
    {
        link_constructors_location[ctor_num]();
    }

}
