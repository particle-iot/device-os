
#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

int system_ble_prov_mode(bool enabled, void* reserved);
bool system_get_ble_prov_status(void* reserved);

#ifdef __cplusplus
}
#endif // __cplusplus
