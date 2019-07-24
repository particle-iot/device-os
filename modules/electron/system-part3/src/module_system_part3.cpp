#include "module_system_part3.h"

extern "C" {
#include "system_part3_loader.c"
}

/**
 * Declares the master dynamic link table, that exports individual dynamic link libraries.
 */

DYNALIB_TABLE_EXTERN(system_module_part3);
DYNALIB_TABLE_EXTERN(hal_usb);
DYNALIB_TABLE_EXTERN(hal_cellular);
DYNALIB_TABLE_EXTERN(hal_socket);
DYNALIB_TABLE_EXTERN(hal_bootloader);
DYNALIB_TABLE_EXTERN(services2);

/**
 * The order of these declarations MUST MATCH the order of declarations in
 * the module_system_part3_exports.ld
 */
extern "C" __attribute__((externally_visible)) const void* const system_part3_module[] = {
    DYNALIB_TABLE_NAME(system_module_part3),
    DYNALIB_TABLE_NAME(hal_usb),
    DYNALIB_TABLE_NAME(hal_cellular),
    DYNALIB_TABLE_NAME(hal_socket),
    DYNALIB_TABLE_NAME(hal_bootloader),
    DYNALIB_TABLE_NAME(services2)
};

