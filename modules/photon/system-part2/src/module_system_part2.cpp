
#include "dynalib.h"
#include "module_system_part1_init.h"
#include "system_mode.h"
#include "module_user_init.h"
#include "core_hal.h"
#include <stdint.h>
#include <stddef.h>

extern "C" {

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
DYNALIB_TABLE_EXTERN(hal_concurrent);
DYNALIB_TABLE_EXTERN(hal_can);
DYNALIB_TABLE_EXTERN(hal_usb);
DYNALIB_TABLE_EXTERN(hal_rgbled);
DYNALIB_TABLE_EXTERN(hal_bootloader);
DYNALIB_TABLE_EXTERN(hal_dct);
DYNALIB_TABLE_EXTERN(system_module_part2);
DYNALIB_TABLE_EXTERN(hal_storage);


// strange that this is needed given that the entire block is scoped extern "C"
// without it, the section name doesn't match *.system_part2_module as expected in the linker script
extern "C" __attribute__((externally_visible)) const void* const system_part2_module[];

/**
 * The module export table. This lists the addresses of individual library dynalib jump tables.
 * Libraries must not be reordered or removed, only new ones added to the end.
 */
extern "C" __attribute__((externally_visible)) const void* const system_part2_module[] = {
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
    DYNALIB_TABLE_NAME(hal_concurrent),
    DYNALIB_TABLE_NAME(hal_can),
    DYNALIB_TABLE_NAME(hal_usb),
    DYNALIB_TABLE_NAME(hal_rgbled),
    DYNALIB_TABLE_NAME(hal_bootloader),
    DYNALIB_TABLE_NAME(hal_dct),
    DYNALIB_TABLE_NAME(system_module_part2),
    DYNALIB_TABLE_NAME(hal_storage)
};

#include "system_part2_loader.c"

} // extern "C"
