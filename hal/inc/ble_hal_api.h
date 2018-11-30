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

#include "ble_hal_defines.h"
#include "ble_hal_api_impl.h"

/**
 * @addtogroup ble_api
 *
 * @brief
 *   This module provides BLE HAL APIs.
 * @{
 */

#define BLE_EVT_TYPE_CONNECTION                     0x01
#define BLE_EVT_TYPE_SCAN_RESULT                    0x02
#define BLE_EVT_TYPE_DISCOVERY                      0x03
#define BLE_EVT_TYPE_DATA                           0x04

#define BLE_CONN_EVT_ID_CONNECTED                   0x01
#define BLE_CONN_EVT_ID_DISCONNECTED                0x02


/* BLE device address */
typedef struct {
    uint8_t addr_type;
    uint8_t addr[BLE_SIG_ADDR_LEN];
} hal_ble_address_t;

/* BLE UUID */
typedef struct {
    uint8_t type;
    union {
        uint16_t uuid16;
        uint8_t* uuid128;
    };
} hal_ble_uuid_t;

/* BLE advertising parameters */
typedef struct {
    uint8_t  type;
    uint8_t  filter_policy;
    uint16_t interval;
    uint16_t duration;
    uint8_t  inc_tx_power;
} hal_ble_adv_params_t;

/* BLE scanning parameters */
typedef struct {
    uint8_t  active;
    uint8_t  filter_policy;
    uint16_t interval;
    uint16_t window;
    uint16_t timeout;
} hal_ble_scan_params_t;

/* BLE connection parameters */
typedef struct {
    uint16_t min_conn_interval;         /**< Minimum Connection Interval in 1.25 ms units.*/
    uint16_t max_conn_interval;         /**< Maximum Connection Interval in 1.25 ms units.*/
    uint16_t slave_latency;             /**< Slave Latency in number of connection events.*/
    uint16_t conn_sup_timeout;          /**< Connection Supervision Timeout in 10 ms units.*/
} hal_ble_conn_params_t;

/* BLE connection status */
typedef struct {
    uint8_t  role;
    uint16_t conn_handle;
    uint16_t conn_interval;
    uint16_t slave_latency;
    uint16_t conn_sup_timeout;
} hal_ble_connection_t;

/* BLE characteristic definition */
typedef struct {
    uint16_t       service_handle;
    hal_ble_uuid_t uuid;
    uint8_t        properties;
    uint16_t       value_handle;
    uint16_t       user_desc_handle;
    uint16_t       cccd_handle;
    uint16_t       sccd_handle;
    uint8_t        value[BLE_MAX_CHAR_VALUE_LEN];
    uint16_t       value_len;
} hal_ble_char_t;

/* BLE service definition */
typedef struct {
    uint8_t         type;
    hal_ble_uuid_t  uuid;
    uint16_t        start_handle;
    uint16_t        end_handle;
    hal_ble_char_t* chars[BLE_MAX_CHARS_PER_SERVICE_COUNT];
    uint8_t         char_count;
} hal_ble_service_t;

typedef struct {
    hal_ble_service_t* service;
    hal_ble_uuid_t     uuid;
    uint8_t            properties;
    uint8_t*           value;
    uint16_t           value_len;
    uint8_t*           desc;
    uint16_t           desc_len;
} hal_ble_char_init_t;

/* BLE connection status changed event */
typedef struct {
    uint16_t conn_handle;
    uint8_t  evt_id;
    union {
        uint8_t reason;             // Disconnect event
    };
} hal_ble_connection_event_t;

/* BLE scan result event */
typedef struct {
    uint8_t           type;
    hal_ble_address_t peer_addr;
    hal_ble_address_t direct_addr;
    int8_t            tx_power;
    int8_t            rssi;
    uint8_t*          data;
    uint16_t          data_len;
} hal_ble_scan_result_event_t;

/* BLE service and characteristic discovery events */
typedef struct {
    uint16_t   status;
    uint16_t   conn_handle;
    uint16_t   service_handle;
    uint16_t   char_handle;
    uint16_t   cccd_handle;
    ble_uuid_t uuid;
} hal_ble_discovery_event_t;

/* GATT Client data transmission events */
typedef struct {
    uint16_t conn_handle;
    uint16_t char_handle;
    uint8_t* data;
    uint16_t data_len;
} hal_ble_data_event_t;

typedef struct {
    uint8_t evt_type;
    union {
        hal_ble_connection_event_t  conn_event;
        hal_ble_scan_result_event_t scan_result_event;
        hal_ble_discovery_event_t   disc_event;
        hal_ble_data_event_t        data_event;
    };
} hal_ble_event_t;

/* BLE event callback */
typedef void *ble_event_callback_t(hal_ble_event_t* event);


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the BLE stack. This function must be called previous to any other BLE APIs.
 *
 * @param[in]   reserved    Reserved for future use.
 *
 * @returns     0 on success, system_error_t on error.
 */
int hal_ble_init(uint8_t role, void* reserved);

/**
 * Register a callback function to subscribe the BLE events.
 *
 * @param[in]   callback    The callback function.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_register_callback(ble_event_callback_t* callback);

/**
 * Deregister the callback function to unsubscribe the BLE events.
 *
 * @param[in]   callback    The callback function.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_deregister_callback(ble_event_callback_t* callback);

/**
 * Set local BLE identity address, which type must be either public or random static.
 *
 * @param[in]   addr    The BLE local identity address to be set.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_set_device_address(hal_ble_address_t addr);

/**
 * Get local BLE identity address, which type is either public or random static.
 *
 * @param[out]  addr    Pointer to address structure to be filled in.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_get_device_address(hal_ble_address_t* addr);

/**
 * Set the BLE device name.
 *
 * @param[in]   device_name Pointer to a UTF-8 encoded, <b>non NULL-terminated</b> string.
 * @param[in]   len         Length of the UTF-8, <b>non NULL-terminated</b> string pointed to by device_name
 *                          in octets (must be smaller or equal than @ref BLE_GAP_DEVNAME_MAX_LEN).
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_set_device_name(uint8_t const* device_name, uint16_t len);

/**
 * Get the BLE device name.
 *
 * @param[out]      device_name Pointer to an empty buffer where the UTF-8 <b>non NULL-terminated</b> string will be placed.
 *                              Set to NULL to obtain the complete device name length.
 * @param[in,out]   p_len       Length of the buffer pointed by device_name, complete device name length on output.
 *
 * @returns         0 on success, system_error_t on error.
 */
int ble_get_device_name(uint8_t* device_name, uint16_t* len);

/**
 * Set GAP Appearance value.
 *
 * @param[in]   appearance  Appearance (16-bit), see @ref BLE_APPEARANCES.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_set_appearance(uint16_t appearance);

/**
 * Get GAP Appearance value.
 *
 * @param[out]  appearance  Pointer to appearance (16-bit) to be filled in, see @ref BLE_APPEARANCES.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_get_appearance(uint16_t* appearance);

/**
 * Set a BLE device whitelist. Only one whitelist can be used at a time and the whitelist is shared
 * between the BLE roles. The whitelist cannot be set if a BLE role is using the whitelist.
 *
 * @param[in]   addr_list   Pointer to a whitelist of peer addresses.
 * @param[in]   len         Length of the whitelist, maximum @ref BLE_GAP_WHITELIST_ADDR_MAX_COUNT.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_add_whitelist(hal_ble_address_t* addr_list, uint8_t len);

/**
 * Delete a BLE device whitelist.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_delete_whitelist(void);

/**
 * Set the TX Power for advertisement.
 *
 * @param[in]   value   Radio transmit power in dBm.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_set_tx_power(int8_t value);

/**
 * Get the TX Power for advertisement.
 *
 * @param[out]  value   Pointer to radio transmit power in dBm to be filled in.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_get_tx_power(int8_t* value);

/**
 * Set the BLE advertising parameters.
 *
 * @param[in]   value   Pointer to the advertising parameters to be set.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_set_advertising_params(hal_ble_adv_params_t* adv_params);

/**
 * Add, update or delete data snippet from advertising data.
 * After this function being called, it won't update the advertising data,
 * Unless the ble_refresh_adv_data() is called.
 * By setting the data pointer to NULl will delete the data snippet.
 *
 * @param[in]   adType  BLE GAP advertising data structure type.
 * @param[in]   data    Pointer to BLE GAP advertising data structure data.
 * @param[in]   len     Length of the BLE GAP advertising data structure data.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_set_adv_data_snippet(uint8_t adType, uint8_t* data, uint16_t len);

/**
 * Set the BLE advertising data. It will update the advertising data immediately if success.
 *
 * @param[in]   data    Pointer to the advertising data to be set.
 * @param[in]   len     Length of the advertising data.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_set_adv_data(uint8_t* data, uint16_t len);

/**
 * Refresh the BLE advertising data. It should be called after calling the ble_set_adv_data_snippet(),
 * so that the advertising data can be updated.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_refresh_adv_data(void);

/**
 * Add, update or delete data snippet from scan response data.
 * After this function being called, it won't update the scan response data,
 * Unless the ble_refresh_scan_resp_data() is called.
 * By setting the data pointer to NULl will delete the data snippet.
 *
 * @param[in]   adType  BLE GAP advertising data structure type.
 * @param[in]   data    Pointer to BLE GAP advertising data structure data.
 * @param[in]   len     Length of the BLE GAP advertising data structure data.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_set_scan_resp_data_snippet(uint8_t adType, uint8_t* data, uint16_t len);

/**
 * Set the BLE scan response data.
 *
 * @param[in]   data    Pointer to the scan response data to be set.
 * @param[in]   len     Length of the scan response data.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_set_scan_resp_data(uint8_t* data, uint16_t len);

/**
 * Refresh the BLE scan response data. It should be called after calling the ble_set_scan_resp_data_snippet(),
 * so that the scan response data can be updated.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_refresh_scan_resp_data(void);

/**
 * Start BLE advertising.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_start_advertising(void);

/**
 * Stop BLE advertising.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_stop_advertising(void);

/**
 * Check if BLE is advertising.
 *
 * @returns     true if it is advertising, otherwise, false.
 */
bool ble_is_advertising(void);

/**
 * Set the BLE scanning parameters.
 *
 * @param[in]   scan_params Pointer to the scanning parameters to be set.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_set_scanning_params(hal_ble_scan_params_t* scan_params);

/**
 * Start scanning nearby BLE devices.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_start_scanning(void);

/**
 * Stop scanning nearby BLE devices.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_stop_scanning(void);

/**
 * Add a BLE service.
 *
 * @param[in]       type    BLE service type, either BLE_SERVICE_TYPE_PRIMARY or BLE_SERVICE_TYPE_SECONDARY.
 * @param[in]       uuid    Pointer to the BLE service UUID.
 * @param[in,out]   service Pointer to the hal_ble_service_t structure.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_add_service(uint8_t type, hal_ble_uuid_t* uuid, hal_ble_service_t* service);

/**
 * Add a BLE Characteristic under a specific BLE Service.
 *
 * @param[in]       char_init   Pointer to the structure that contains the Characteristic configurations.
 * @param[in]       ble_char    Pointer to the hal_ble_char_t structure.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_add_characteristic(hal_ble_char_init_t* char_init, hal_ble_char_t* ble_char);

/**
 * Send a BLE notification or indication to client.
 *
 * @param[in]   conn_handle BLE connection handle.
 * @param[in]   data        Pointer to the buffer that contains the data to be sent.
 * @param[in]   len         Length of the data to be sent.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_publish(uint16_t conn_handle, hal_ble_char_t* ble_char, uint8_t* data, uint16_t len);

/**
 * Update local characteristic value. The value won't send to client.
 *
 * @param[in]   ble_char    Pointer to the hal_ble_char_t structure.
 * @param[in]   data        Pointer to the buffer that contains the data to be set.
 * @param[in]   len         Length of the data to be set.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_set_characteristic_value(hal_ble_char_t* ble_char, uint8_t* data, uint16_t len);

/**
 * Get local characteristic value.
 *
 * @param[in]       ble_char    Pointer to the hal_ble_char_t structure.
 * @param[in]       data        Pointer to the buffer that to be filled with the Characteristic value.
 * @param[in,out]   len         Length of the given buffer. Return the actual length of the value.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_get_characteristic_value(hal_ble_char_t* ble_char, uint8_t* data, uint16_t* len);

int ble_connect(hal_ble_address_t* addr);
int ble_connect_cancel(void);

/**
 * Terminate BLE connection.
 *
 * @param[in]   conn_handle BLE connection handle.
 * @param[in]   reason      Disconnect reason, either BLE_DISCONN_REASON_USER_INITIATE or BLE_DISCONN_REASON_CONN_INTERVAL.
 *
 * @returns     0 on success, system_error_t on error.
 */
int hal_ble_disconnect(uint16_t conn_handle, uint8_t reason);

/**
 * Update connection parameters. In the central role this will initiate a Link Layer connection parameter update procedure.
 * In the peripheral role, this will send the corresponding request and wait for the central to perform the procedure.
 *
 * @param[in]   conn_handle BLE connection handle.
 * @param[in]   conn_params Pointer to a hal_ble_conn_params_t structure with the desired parameters.
 *                          If NULL is provided on a peripheral role, the parameters in the PPCP characteristic of the GAP
 *                          service will be used instead. If NULL is provided on a central role and in response to a
 *                          connection parameters update request, the peripheral request will be rejected.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_update_connection_params(uint16_t conn_handle, hal_ble_conn_params_t* conn_params);

/**
 * Set GAP Peripheral Preferred Connection Parameters.
 *
 * @param[in]   conn_params Pointer to a hal_ble_conn_params_t structure with the desired parameters.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_set_ppcp(hal_ble_conn_params_t* conn_params);

/**
 * Get GAP Peripheral Preferred Connection Parameters.
 *
 * @param[in]   conn_params Pointer to a hal_ble_conn_params_t structure where the parameters will be stored.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_get_ppcp(hal_ble_conn_params_t* conn_params);

int ble_get_rssi(uint16_t conn_handle);

/* Functions for GATT Client */
int ble_discovery_services(uint16_t conn_handle);
int ble_discovery_characteristics(uint16_t conn_handle, uint16_t service_handle);
int ble_discovery_descriptors(uint16_t conn_handle, uint16_t char_handle);
int ble_subscribe(uint16_t conn_handle, uint8_t char_handle);
int ble_unsubscribe(uint16_t conn_handle, uint8_t char_handle);
int ble_write_with_response(uint16_t conn_handle, uint8_t char_handle, uint8_t* data, uint16_t len);
int ble_write_without_response(uint16_t conn_handle, uint8_t char_handle, uint8_t* data, uint16_t len);
int ble_read(uint16_t conn_handle, uint8_t char_handle, uint8_t* data, uint16_t* len);


#ifdef __cplusplus
} // extern "C"
#endif


/**
 * @}
 */

#endif /* BLE_HAL_API_H */
