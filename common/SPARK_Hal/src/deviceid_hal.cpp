#include "deviceid_hal.h"
#include "platform_config.h"
#include <algorithm>
#include <cstring>

const unsigned device_id_len = 12;

unsigned HAL_device_ID(uint8_t* dest, unsigned destLen)
{    
    if (dest!=NULL && destLen!=0)
        memcpy(dest, (char*)ID1, std::min(destLen, device_id_len));
    return device_id_len;
}