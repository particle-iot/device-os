
#include "dynalib.h"
#include "module_system_part1_init.h"
#include "module_system_part3_init.h"
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
DYNALIB_TABLE_EXTERN(hal_cellular);
DYNALIB_TABLE_EXTERN(hal_usart);
DYNALIB_TABLE_EXTERN(hal_concurrent);
DYNALIB_TABLE_EXTERN(hal_can);
DYNALIB_TABLE_EXTERN(hal_rgbled);
DYNALIB_TABLE_EXTERN(hal_dct);


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
    DYNALIB_TABLE_NAME(hal_cellular),
    DYNALIB_TABLE_NAME(hal_usart),
    DYNALIB_TABLE_NAME(system_net),
    DYNALIB_TABLE_NAME(system_cloud),
    DYNALIB_TABLE_NAME(hal_concurrent),
    DYNALIB_TABLE_NAME(hal_can),
    DYNALIB_TABLE_NAME(hal_rgbled),
    DYNALIB_TABLE_NAME(hal_dct)
};

#include "system_part2_loader.c"

} // extern "C"
