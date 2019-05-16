#include <stddef.h>
#include <string.h>

/**
 * An empty no arg, no result function
 */
typedef void  (*constructor_ptr_t)(void);

/**
 * Pointer to the reset handler;
 */
extern void* dynamic_reset_handler_location;
extern char stack_end;
extern char system_part3_ram_start;

void* module_system_part3_pre_init();

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

void* module_system_part3_pre_init()
{
    if ( (&link_global_data_start!=&link_global_data_initial_values) && (link_global_data_size != 0))
    {
        memcpy(&link_global_data_start, &link_global_data_initial_values, link_global_data_size);
    }

    memset(&link_bss_location, 0, link_bss_size );

    return &system_part3_ram_start;
}


/**
 * Array of C++ static constructors.
 */
extern constructor_ptr_t link_constructors_location[];
extern constructor_ptr_t link_constructors_end;
#define link_constructors_size   ((unsigned long)&link_constructors_end  -  (unsigned long)&link_constructors_location )


void module_system_part3_init()
{
    // invoke constructors
    unsigned int ctor_num;
    for (ctor_num=0; ctor_num < link_constructors_size/sizeof(constructor_ptr_t); ctor_num++ )
    {
        link_constructors_location[ctor_num]();
    }

}

// todo - this file is a copy/paste of system_part1_loader.c
// with the boot table and reset handler logic removed.
// factor out common elements between part1/part3 and dedupe
