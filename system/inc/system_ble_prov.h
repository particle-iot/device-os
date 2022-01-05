
#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "ble_hal.h"

#if HAL_PLATFORM_BLE

int system_ble_prov_mode(bool enabled, void* reserved);
bool system_get_ble_prov_status(void* reserved);
int system_set_prov_svc_uuid(hal_ble_uuid_t svcUuid, hal_ble_uuid_t txUuid, hal_ble_uuid_t rxUuid, void* reserved);
int system_set_prov_adv_svc_uuid(const uint8_t* buf, size_t len, void* reserved);

#endif

#ifdef __cplusplus
}
#endif // __cplusplus
