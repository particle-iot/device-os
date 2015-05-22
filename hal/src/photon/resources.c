
#include "wiced_resource.h"
#include "wifi_nvram_image.h"

#define NVRAM_SIZE             sizeof( wifi_nvram_image )
#define NVRAM_IMAGE_VARIABLE   wifi_nvram_image

const resource_hnd_t* wwd_nvram_image_resource(void) __attribute__((weak)) ;

const resource_hnd_t* wwd_nvram_image_resource(void)
{
    static const resource_hnd_t wifi_nvram_resource = { RESOURCE_IN_MEMORY, NVRAM_SIZE, {.mem = { (const char *) NVRAM_IMAGE_VARIABLE}}};
    return &wifi_nvram_resource;
}


