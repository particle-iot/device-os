
#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "ble_hal.h"

#if HAL_PLATFORM_BLE

int system_ble_prov_mode(bool enabled, void* reserved);
bool system_get_ble_prov_status(void* reserved);
int system_set_custom_prov_svc_uuid(hal_ble_uuid_t svcUuid, void* reserved);
int system_set_custom_prov_tx_uuid(hal_ble_uuid_t txUuid, void* reserved);
int system_set_custom_prov_rx_uuid(hal_ble_uuid_t rxUuid, void* reserved);
int system_set_custom_prov_ver_uuid(hal_ble_uuid_t verUuid, void* reserved);

#endif

#ifdef __cplusplus
}
#endif // __cplusplus
