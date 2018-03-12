#include <cstdlib>
#include "wiced.h"
#include "device_name.h"

#ifdef __cplusplus
extern "C" {
#endif

extern bool fetch_or_generate_setup_ssid(wiced_ssid_t* SSID);

void HAL_Local_Name(local_name_t *local_name)
{
	wiced_ssid_t ssid;
    fetch_or_generate_setup_ssid( &ssid );

    memcpy(local_name, &ssid, sizeof(local_name_t));
}

#ifdef __cplusplus
}
#endif

