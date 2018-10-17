
#include "dynalib.h"
#include "system_mode.h"
#include "module_user_init.h"
#include "core_hal.h"
#include <stdint.h>
#include <stddef.h>
#include "hal_platform.h"

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
DYNALIB_TABLE_EXTERN(communication);
DYNALIB_TABLE_EXTERN(services);
DYNALIB_TABLE_EXTERN(hal_inet);
DYNALIB_TABLE_EXTERN(hal_netdb);
DYNALIB_TABLE_EXTERN(hal_ifapi);
DYNALIB_TABLE_EXTERN(hal_resolvapi);
#if HAL_PLATFORM_CELLULAR
DYNALIB_TABLE_EXTERN(hal_cellular);
#endif // HAL_PLATFORM_CELLULAR
#if HAL_PLATFORM_WIFI
DYNALIB_TABLE_EXTERN(hal_wlan);
#endif // HAL_PLATFORM_WIFI

// strange that this is needed given that the entire block is scoped extern "C"
// without it, the section name doesn't match *.system_part2_module as expected in the linker script
extern "C" __attribute__((externally_visible)) const void* const system_part1_module[];

/**
 * The module export table. This lists the addresses of individual library dynalib jump tables.
 * Libraries must not be reordered or removed, only new ones added to the end.
 */
extern "C" __attribute__((externally_visible)) const void* const system_part1_module[] = {
    DYNALIB_TABLE_NAME(hal),
    DYNALIB_TABLE_NAME(rt),
    DYNALIB_TABLE_NAME(system),
    DYNALIB_TABLE_NAME(hal_peripherals),
    DYNALIB_TABLE_NAME(hal_i2c),
    DYNALIB_TABLE_NAME(hal_gpio),
    DYNALIB_TABLE_NAME(hal_spi),
    DYNALIB_TABLE_NAME(hal_core),
    DYNALIB_TABLE_NAME(hal_socket),
    DYNALIB_TABLE_NAME(hal_usart),
    DYNALIB_TABLE_NAME(system_net),
    DYNALIB_TABLE_NAME(system_cloud),
    DYNALIB_TABLE_NAME(hal_concurrent),
    DYNALIB_TABLE_NAME(hal_usb),
    DYNALIB_TABLE_NAME(hal_rgbled),
    DYNALIB_TABLE_NAME(hal_bootloader),
    DYNALIB_TABLE_NAME(hal_dct),
    DYNALIB_TABLE_NAME(communication),
    DYNALIB_TABLE_NAME(services),
    DYNALIB_TABLE_NAME(hal_inet),
    DYNALIB_TABLE_NAME(hal_netdb),
    DYNALIB_TABLE_NAME(hal_ifapi),
    DYNALIB_TABLE_NAME(hal_resolvapi)
#if HAL_PLATFORM_CELLULAR
    , DYNALIB_TABLE_NAME(hal_cellular)
#endif // HAL_PLATFORM_CELLULAR
#if HAL_PLATFORM_WIFI
    , DYNALIB_TABLE_NAME(hal_wlan)
#endif // HAL_PLATFORM_WIFI
};

#include "system_part1_loader.c"

} // extern "C"
