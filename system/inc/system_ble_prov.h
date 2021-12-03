
#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef enum ble_prov_uuid_category {
    BLE_SERVICE_UUID    = 0,
    BLE_TX_UUID = 1,
    BLE_RX_UUID = 2
} ble_prov_uuid_category;

// TODO: Add system callbacks for BLE prov mode

int system_ble_prov_mode(bool enabled, void* reserved);
bool system_get_ble_prov_status(void* reserved);
int system_set_provisioning_uuid(const char* serviceUuid, const char* txUuid, const char* rxUuid, void* reserved);
int system_set_prov_blah_me(const uint8_t* buf, size_t len, void* reserved);
int system_set_prov_svc_uuid(const uint8_t* buf, size_t len, void* reserved);
int system_set_prov_tx_uuid(const uint8_t* buf, size_t len, void* reserved);
int system_set_prov_rx_uuid(const uint8_t* buf, size_t len, void* reserved);
int system_set_prov_adv_svc_uuid(const uint8_t* buf, size_t len, void* reserved);


#ifdef __cplusplus
}
#endif // __cplusplus