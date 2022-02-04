
#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "ble_hal.h"

#if HAL_PLATFORM_BLE

int system_ble_prov_mode(bool enabled, void* reserved);
bool system_ble_prov_get_status(void* reserved);
int system_ble_prov_set_custom_svc_uuid(hal_ble_uuid_t* svcUuid, void* reserved);
int system_ble_prov_set_custom_tx_uuid(hal_ble_uuid_t* txUuid, void* reserved);
int system_ble_prov_set_custom_rx_uuid(hal_ble_uuid_t* rxUuid, void* reserved);
int system_ble_prov_set_custom_ver_uuid(hal_ble_uuid_t* verUuid, void* reserved);

#endif

#ifdef __cplusplus
}
#endif // __cplusplus
