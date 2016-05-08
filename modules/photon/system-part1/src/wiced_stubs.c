#include "wwd_resources.h"

extern resource_hnd_t wifi_firmware_image;

const resource_hnd_t* wwd_firmware_image_resource(void)
{
    return &wifi_firmware_image;
}
