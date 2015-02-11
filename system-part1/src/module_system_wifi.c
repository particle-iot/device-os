
#define DYNALIB_EXPORT
#include "dynalib.h"
#include "module_system_wifi.h"
#include <stddef.h>
#include <string.h>

DYNALIB_TABLE_EXTERN(communication);
DYNALIB_TABLE_EXTERN(wifi_resource);
DYNALIB_TABLE_EXTERN(module_part1);

const void* const system_part1_module[] = {
    DYNALIB_TABLE_NAME(communication),
    DYNALIB_TABLE_NAME(wifi_resource),    
    DYNALIB_TABLE_NAME(module_part1)
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

void* module_pre_init()
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
typedef void  (*constructor_ptr_t)(void);
extern constructor_ptr_t link_constructors_location[];
extern constructor_ptr_t link_constructors_end;

void module_init()
{
    
}
