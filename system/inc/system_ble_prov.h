
#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

int system_ble_prov_mode(bool enabled, void* reserved);
bool system_get_ble_prov_status(void* reserved);
int system_set_prov_svc_uuid(const uint8_t* svcUuid, const uint8_t* txUuid, const uint8_t* rxUuid, size_t len, void* reserved);
int system_set_prov_adv_svc_uuid(const uint8_t* buf, size_t len, void* reserved);

#ifdef __cplusplus
}
#endif // __cplusplus
