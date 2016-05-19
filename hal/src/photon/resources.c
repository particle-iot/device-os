
#include "wiced_resource.h"
#include "wwd_resources.h"

#define WIFI_NVRAM_LTXP 1
#include "wifi_nvram_image.h"
#undef WIFI_NVRAM_LTXP
#undef INCLUDED_NVRAM_IMAGE_H_
#include "wifi_nvram_image.h"


static const resource_hnd_t wifi_nvram_resources[] = {
		{ RESOURCE_IN_MEMORY, sizeof( wifi_main_nvram_image ), {.mem = { (const char *) wifi_main_nvram_image }}},
		{ RESOURCE_IN_MEMORY, sizeof( wifi_ltxp_nvram_image ), {.mem = { (const char *) wifi_ltxp_nvram_image }}}
};


static uint8_t wifi_nvram_resource_index = 0;

/**
 * This is a bit of a hack. The resources.c file is compiled in the HAL, but then it's borrowed for
 * system-part1, which is linked as a dynamic library. The resources.o is still linked in the hal in
 * part2, but part2 is also importing the dynamic library, leading to a double definition of these
 * functions. Marking them weak sidesteps this.
 * This is a bit of a hack. The resources.c file is compiled in the HAL, but then it's borrowed for system-part1, which is linked as a
 * dynamic library. The resources.o is still linked in the hal in part2, but part2 is also importing the dynamic library, leading to a double
 * definition of these functions. Marking them weak sidesteps this.
 * I imagine there's a more elegant way to solve this, but this was quick to implement.
 */
const resource_hnd_t* wwd_nvram_image_resource(void) __attribute__((weak)) ;
int wwd_select_nvram_image_resource(uint8_t index, void* reserved) __attribute__((weak));


const resource_hnd_t* wwd_nvram_image_resource(void)
{
    return &wifi_nvram_resources[wifi_nvram_resource_index];
}

/**
 * @param index: 0 for regular, 1 for reduced transmit power (for TELEC certification.)
 */
int wwd_select_nvram_image_resource(uint8_t index, void* reserved)
{
    wifi_nvram_resource_index = index;
    return 0;
}
