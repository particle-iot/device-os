/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef BLE_HAL_API_H
#define BLE_HAL_API_H

#include <stdint.h>
#include <stddef.h>

namespace particle { namespace ble {

/* BLE device address type */
typedef enum {
    BLE_ADDR_TYPE_PUBLIC = 0x00, /**< Public (identity) address.*/
    BLE_ADDR_TYPE_RANDOM_STATIC = 0x01, /**< Random static (identity) address. */
    BLE_ADDR_TYPE_RANDOM_PRIVATE_RESOLVABLE = 0x02, /**< Random private resolvable address. */
    BLE_ADDR_TYPE_RANDOM_PRIVATE_NON_RESOLVABLE = 0x03, /**< Random private non-resolvable address. */
    BLE_ADDR_TYPE_ANONYMOUS = 0x7F /**< An advertiser may advertise without its address. This type of advertising is called anonymous. */
} ble_address_type_t;

/* BLE device address */
typedef struct {
    ble_address_type_t type;
    uint8_t            data[6];
} ble_address_t;

/* BLE UUID */
typedef struct {
    uint8_t type;
    union {
        uint16_t uuid16;
        uint8_t  uuid128[16];
    };
} ble_uuid_t;

/* BLE advertising parameters */
typedef struct {
    uint8_t  type;
    uint8_t  filter_policy;
    uint16_t interval;
    uint16_t duration;
    uint8_t  inc_tx_power;
} ble_adv_params_t;

/* BLE scanning parameters */
typedef struct {
    uint8_t  active;
    uint8_t  filter_policy;
    uint16_t interval;
    uint16_t window;
    uint16_t timeout;
} ble_scan_params_t;

/* BLE connection parameters */
typedef struct {
    uint16_t min_conn_interval;         /**< Minimum Connection Interval in 1.25 ms units.*/
    uint16_t max_conn_interval;         /**< Maximum Connection Interval in 1.25 ms units.*/
    uint16_t slave_latency;             /**< Slave Latency in number of connection events.*/
    uint16_t conn_sup_timeout;          /**< Connection Supervision Timeout in 10 ms units.*/
} ble_conn_params_t;

/* BLE connection status changed event */
typedef struct {
    uint16_t conn_handle;
    uint8_t  status;
    uint8_t  reason;
} ble_connection_event_t;

/* BLE scan result event */
typedef struct {
    uint8_t       type;
    ble_address_t peer_addr;
    ble_address_t direct_addr;
    int8_t        tx_power;
    int8_t        rssi;
    uint8_t*      data;
    uint16_t      data_len;
} ble_scan_result_event_t;

typedef void *ble_connection_callback_t(ble_connection_event_t* event);
typedef void *ble_scan_result_callback_t(ble_scan_result_event_t* event);

/* Callbacks on GAP events */
typedef struct {
    ble_connection_callback_t  conn_callback;
    ble_scan_result_callback_t scan_callback;
} ble_gap_event_callbacks_t;

/* BLE service and characteristic discovery events */
typedef struct {
    uint16_t   status;
    uint16_t   conn_handle;
    uint16_t   service_handle;
    uint16_t   char_handle;
    uint16_t   cccd_handle;
    ble_uuid_t uuid;
} ble_discovery_event_t;

/* BLE characteristic definition */
typedef struct {
    ble_uuid_t uuid;
    uint8_t*   value;
    uint8_t    properties;
} ble_characteristic_t;

/* GATT Client data transmission events */
typedef struct {
    uint16_t status;
    uint16_t conn_handle;
    uint16_t char_handle;
    uint8_t* data;
    uint16_t data_len;
} ble_gattc_data_event_t;

typedef void *ble_discovery_callback_t(ble_discovery_event_t* event);
typedef void *ble_gattc_data_callback_t(ble_gattc_data_event_t* event);

/* Callbacks on GATT Client events */
typedef struct {
    ble_discovery_callback_t  discovery_callback;
    ble_gattc_data_callback_t data_callback;
} ble_gattc_event_callbacks_t;

/* GATT Server data transmission events */
typedef struct {
    uint16_t status;
    uint16_t conn_handle;
    uint16_t char_handle;
    uint8_t* data;
    uint16_t data_len;
} ble_gatts_data_event_t;

typedef void *ble_gatts_data_callback_t(ble_gatts_data_event_t* event);

/* Callbacks on GATT Server events */
typedef struct {
    ble_gatts_data_callback_t data_callback;
} ble_gatts_event_callbacks_t;


#ifdef __cplusplus
extern "C" {
#endif

/* Generic BLE functions */
int hal_ble_init(void* reserved);
int hal_ble_on_gap_event_callbacks(ble_gap_event_callbacks_t* callbacks);
int hal_ble_on_gattc_event_callbacks(ble_gattc_event_callbacks_t* callback);
int hal_ble_on_gatts_event_callbacks(ble_gatts_event_callbacks_t* callback);
int hal_ble_set_device_address(ble_address_t addr);
int hal_ble_get_device_address(ble_address_t* addr);
int hal_ble_set_device_name(const char* device_name);
int hal_ble_get_device_name(uint8_t* device_name, uint16_t* len);
int hal_ble_set_appearance(uint16_t appearance);
int hal_ble_get_appearance(uint16_t* appearance);
int hal_ble_add_to_whitelist(ble_address_t* addr);
int hal_ble_remove_from_whitelist(ble_address_t* addr);

/* Functions for BLE Broadcaster */
int hal_ble_set_tx_power(int8_t value);
int hal_ble_get_tx_power(int8_t* value);
int hal_ble_set_advertising_params(ble_adv_params_t* adv_params);
int hal_ble_set_advertising_interval(uint16_t interval);
int hal_ble_set_advertising_duration(uint16_t duration);
int hal_ble_set_advertising_type(uint8_t type);
int hal_ble_enable_advertising_filter(bool enable);
int hal_ble_set_advertising_data(uint8_t* data, uint8_t len);
int hal_ble_set_scan_response_data(uint8_t* data, uint8_t len);
int hal_ble_start_advertising(void);
int hal_ble_stop_advertising(void);
bool hal_ble_is_advertising(void);

/* Functions for BLE Observer */
int hal_ble_set_scanning_params(ble_scan_params_t* scan_params);
int hal_ble_set_scanning_interval(uint16_t interval);
int hal_ble_set_scanning_window(uint16_t window);
int hal_ble_set_scanning_timeout(uint16_t timeout);
int hal_ble_enable_scanning_filter(bool enable);
int hal_ble_set_scanning_policy(uint8_t value);
int hal_ble_start_scanning(void);
int hal_ble_stop_scanning(void);

/* Functions for BLE Central and Peripheral */
int hal_ble_connect(ble_address_t* addr);
int hal_ble_connect_cancel(void);
int hal_ble_disconnect(uint16_t conn_handle);
int hal_ble_update_connection_params(uint16_t conn_handle, ble_conn_params_t* conn_params);
int hal_ble_set_ppcp(ble_conn_params_t* conn_params);
int hal_ble_get_ppcp(ble_conn_params_t* conn_params);
int hal_ble_get_rssi(uint16_t conn_handle);

/* Functions for GATT Client */
int hal_ble_discovery_services(uint16_t conn_handle);
int hal_ble_discovery_characteristics(uint16_t conn_handle, uint16_t service_handle);
int hal_ble_discovery_descriptors(uint16_t conn_handle, uint16_t char_handle);
int hal_ble_subscribe(uint16_t conn_handle, uint8_t char_handle);
int hal_ble_unsubscribe(uint16_t conn_handle, uint8_t char_handle);
int hal_ble_write_with_response(uint16_t conn_handle, uint8_t char_handle, uint8_t* data, uint16_t len);
int hal_ble_write_without_response(uint16_t conn_handle, uint8_t char_handle, uint8_t* data, uint16_t len);
int hal_ble_read(uint16_t conn_handle, uint8_t char_handle, uint8_t* data, uint16_t* len);

/* Functions for GATT Server */
int hal_ble_add_service(particle::ble::ble_uuid_t const* uuid, uint16_t* service_handle);
int hal_ble_add_characteristic(uint16_t service_handle, ble_characteristic_t* characteristic, uint16_t *char_handle);
int hal_ble_publish(uint16_t conn_handle, uint8_t char_handle, uint8_t* data, uint16_t len);
int hal_ble_set_characteristic_value(uint8_t char_handle, uint8_t* data, uint16_t len);
int hal_ble_get_characteristic_value(uint8_t char_handle, uint8_t* data, uint16_t* len);

#ifdef __cplusplus
} // extern "C"
#endif

} } /* particle::ble */

#endif /* BLE_HAL_API_H */
