
#define DYNALIB_EXPORT
#include "module_user_init.h"
#include "system_user.h"
#include <stddef.h>
#include <string.h>
#include "core_hal.h"

#include "memproc.h"


/**
 * Locations of static memory regions from linker.
 */
extern uintptr_t link_global_data_initial_values;
extern uintptr_t link_global_data_start;
extern uintptr_t link_global_data_end;
#define link_global_data_size ((uintptr_t)&link_global_data_end - (uintptr_t)&link_global_data_start)

extern uintptr_t link_bss_location;
extern uintptr_t link_bss_end;
#define link_bss_size ((uintptr_t)&link_bss_end - (uintptr_t)&link_bss_location)

extern uintptr_t link_psram_code_flash_start;
extern uintptr_t link_psram_code_start;
extern uintptr_t link_psram_code_end;
#define link_psram_code_size ((uintptr_t)&link_psram_code_end - (uintptr_t)&link_psram_code_start)

extern uintptr_t link_dynalib_flash_start;
extern uintptr_t link_dynalib_start;
extern uintptr_t link_dynalib_end;
#define link_dynalib_size ((uintptr_t)&link_dynalib_end - (uintptr_t)&link_dynalib_start)


/**
 * Array of C++ static constructors.
 */
typedef void  (*constructor_ptr_t)(void);
extern constructor_ptr_t link_constructors_location[];
extern constructor_ptr_t link_constructors_end;

void* dynalib_table_location = (void*)0x02000000; // system part 1 dynalib location in PSRAM

/**
 * Initializes this user module. Returns the start of the heap.
 */
__attribute__((section(".xip.text"))) void* module_user_pre_init() {
    // NOTE: Must invoke APIs in ROM
    // Copy .data
    if ( (&link_global_data_start != &link_global_data_initial_values) && (link_global_data_size != 0))
    {
        _memcpy(&link_global_data_start, &link_global_data_initial_values, link_global_data_size);
    }

    // Initialize .bss
    _memset(&link_bss_location, 0, link_bss_size );

    // Copy .dynalib
    if ( (&link_dynalib_start != &link_dynalib_flash_start) && (link_dynalib_size != 0))
    {
        _memcpy(&link_dynalib_start, &link_dynalib_flash_start, link_dynalib_size);
    }

    // Copy .psram_text
    if ( (&link_psram_code_start != &link_psram_code_flash_start) && (link_psram_code_size != 0))
    {
        _memcpy(&link_psram_code_start, &link_psram_code_flash_start, link_psram_code_size);
    }

    return &link_global_data_start;
}

/**
 * Array of C++ static constructors.
 */
typedef void  (*constructor_ptr_t)(void);
extern constructor_ptr_t link_constructors_location[];
extern constructor_ptr_t link_constructors_end;
#define link_constructors_size   ((unsigned long)&link_constructors_end  -  (unsigned long)&link_constructors_location )

void module_user_init()
{
    module_user_init_hook();

    // invoke constructors
    unsigned ctor_num;
    for (ctor_num=0; ctor_num < link_constructors_size/sizeof(constructor_ptr_t); ctor_num++ )
    {
        link_constructors_location[ctor_num]();
    }
}

/**
 * Export these functions with a fuller name so they don't clash with the setup/loop wrappers in the system module.
 */
void module_user_setup() {
    setup();
}

void module_user_loop() {
    loop();
    _post_loop();
}

#include "user_preinit_dynalib.h"
#include "user_dynalib.h"

