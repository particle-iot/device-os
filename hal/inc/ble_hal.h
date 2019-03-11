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

#ifndef BLE_HAL_H
#define BLE_HAL_H

#include <stdint.h>
#include <stddef.h>

#include "ble_hal_defines.h"
#include "ble_hal_impl.h"

/**
 * @addtogroup ble_api
 *
 * @brief
 *   This module provides BLE HAL APIs.
 * @{
 */

typedef enum {
    BLE_ANT_DEFAULT  = 0,
    BLE_ANT_INTERNAL = 1,
    BLE_ANT_EXTERNAL = 2,
} hal_ble_ant_type_t;

/* BLE device address */
typedef struct {
    uint8_t addr_type;
    uint8_t addr[BLE_SIG_ADDR_LEN];
} hal_ble_addr_t;

/* BLE UUID, little endian */
typedef struct {
    uint8_t type;
    union {
        uint16_t uuid16;
        uint8_t  uuid128[BLE_SIG_UUID_128BIT_LEN];
    };
} hal_ble_uuid_t;

/* BLE advertising parameters */
typedef struct {
    uint8_t  version;
    uint8_t  type;
    uint8_t  filter_policy;
    uint16_t interval;                  /**< Advertising interval in 625 us units. */
    uint16_t timeout;                   /**< Advertising timeout in 10 ms units.*/
    uint8_t  inc_tx_power;
} hal_ble_adv_params_t;

/* BLE scanning parameters */
typedef struct {
    uint8_t  version;
    uint8_t  active;
    uint8_t  filter_policy;
    uint16_t interval;                  /**< Scan interval in 625 us units. */
    uint16_t window;                    /**< Scan window in 625 us units. */
    uint16_t timeout;                   /**< Scan timeout in 10 ms units. */
} hal_ble_scan_params_t;

/* BLE connection parameters */
typedef struct {
    uint8_t  version;
    uint16_t min_conn_interval;         /**< Minimum Connection Interval in 1.25 ms units.*/
    uint16_t max_conn_interval;         /**< Maximum Connection Interval in 1.25 ms units.*/
    uint16_t slave_latency;             /**< Slave Latency in number of connection events.*/
    uint16_t conn_sup_timeout;          /**< Connection Supervision Timeout in 10 ms units.*/
} hal_ble_conn_params_t;

/* BLE service definition */
typedef struct {
    uint8_t        version;
    uint16_t       start_handle;
    uint16_t       end_handle;
    hal_ble_uuid_t uuid;
} hal_ble_svc_t;

typedef struct {
    uint8_t        version;
    uint8_t        properties;
    uint16_t       service_handle;
    const char*    description;
    hal_ble_uuid_t uuid;
} hal_ble_char_init_t;

typedef struct {
    uint16_t decl_handle;
    uint16_t value_handle;
    uint16_t user_desc_handle;
    uint16_t cccd_handle;
    uint16_t sccd_handle;
} hal_ble_char_handles_t;

/* BLE characteristic definition */
typedef struct {
    uint8_t        version;
    uint8_t        char_ext_props : 1;
    uint8_t        properties;
    uint16_t       decl_handle;
    uint16_t       value_handle;
    uint16_t       user_desc_handle;
    uint16_t       cccd_handle;
    uint16_t       sccd_handle;
    hal_ble_uuid_t uuid;
} hal_ble_char_t;

typedef struct {
    uint8_t        version;
    uint16_t       char_handle;
    uint8_t*       descriptor;
    uint16_t       len;
    hal_ble_uuid_t uuid;
} hal_ble_desc_init_t;

/* BLE descriptor definition */
typedef struct {
    uint8_t        version;
    uint16_t       handle;
    hal_ble_uuid_t uuid;
} hal_ble_desc_t;

/* BLE events structure */
typedef struct {
    uint8_t version;
    void*   reserved;
} hal_ble_gap_on_adv_stopped_evt_t;

typedef struct {
    uint8_t  version;
    int8_t   rssi;
    struct {
        uint16_t connectable   : 1;     /**< Connectable advertising event type. */
        uint16_t scannable     : 1;     /**< Scannable advertising event type. */
        uint16_t directed      : 1;     /**< Directed advertising event type. */
        uint16_t scan_response : 1;     /**< Received a scan response. */
        uint16_t extended_pdu  : 1;     /**< Received an extended advertising set. */
    } type;
    uint16_t data_len;
    uint8_t  data[BLE_MAX_SCAN_REPORT_BUF_LEN];
    hal_ble_addr_t peer_addr;
} hal_ble_gap_on_scan_result_evt_t;

typedef struct {
    uint8_t version;
    void*   reserved;
} hal_ble_gap_on_scan_stopped_evt_t;

typedef struct {
    uint8_t  version;
    uint8_t  role;
    uint16_t conn_handle;
    uint16_t conn_interval;             /**< Connection Interval in 1.25 ms units.*/
    uint16_t slave_latency;             /**< Slave Latency in number of connection events.*/
    uint16_t conn_sup_timeout;          /**< Connection Supervision Timeout in 10 ms units.*/
    hal_ble_addr_t peer_addr;
} hal_ble_gap_on_connected_evt_t;

typedef struct {
    uint8_t  version;
    uint8_t  reason;
    uint16_t conn_handle;
    hal_ble_addr_t peer_addr;
} hal_ble_gap_on_disconnected_evt_t;

typedef struct {
    uint8_t  version;
    uint16_t conn_handle;
    uint16_t conn_interval;             /**< Connection Interval in 1.25 ms units.*/
    uint16_t slave_latency;             /**< Slave Latency in number of connection events.*/
    uint16_t conn_sup_timeout;          /**< Connection Supervision Timeout in 10 ms units.*/
} hal_ble_gap_on_conn_params_evt_t;

typedef struct {
    uint8_t  version;
    uint16_t conn_handle;
    uint8_t  count;
    hal_ble_svc_t* services;
} hal_ble_gattc_on_svc_disc_evt_t;

typedef struct {
    uint8_t  version;
    uint16_t conn_handle;
    uint8_t  count;
    hal_ble_char_t* characteristics;
} hal_ble_gattc_on_char_disc_evt_t;

typedef struct {
    uint8_t  version;
    uint16_t conn_handle;
    uint8_t  count;
    hal_ble_desc_t* descriptors;
} hal_ble_gattc_on_desc_disc_evt_t;

typedef struct {
    uint8_t  version;
    uint16_t conn_handle;
    uint16_t attr_handle;
    uint16_t offset;
    uint16_t data_len;
    uint8_t  data[BLE_MAX_CHAR_VALUE_LEN];
} hal_ble_gatt_on_data_evt_t;

typedef enum {
    BLE_EVT_ADV_STOPPED,
    BLE_EVT_SCAN_RESULT,
    BLE_EVT_SCAN_STOPPED,
    BLE_EVT_CONNECTED,
    BLE_EVT_DISCONNECTED,
    BLE_EVT_CONN_PARAMS_UPDATED,
    BLE_EVT_SVC_DISCOVERED,
    BLE_EVT_CHAR_DISCOVERED,
    BLE_EVT_DESC_DISCOVERED,
    BLE_EVT_DATA_READ,
    BLE_EVT_DATA_WRITTEN,
    BLE_EVT_DATA_NOTIFIED
} hal_ble_evts_type_t;

typedef struct {
    hal_ble_evts_type_t type;
    union {
        hal_ble_gap_on_adv_stopped_evt_t  adv_stopped;
        hal_ble_gap_on_scan_result_evt_t  scan_result;
        hal_ble_gap_on_scan_stopped_evt_t scan_stopped;
        hal_ble_gap_on_connected_evt_t    connected;
        hal_ble_gap_on_disconnected_evt_t disconnected;
        hal_ble_gap_on_conn_params_evt_t  conn_params_updated;
        hal_ble_gattc_on_svc_disc_evt_t   svc_disc;
        hal_ble_gattc_on_char_disc_evt_t  char_disc;
        hal_ble_gattc_on_desc_disc_evt_t  desc_disc;
        hal_ble_gatt_on_data_evt_t        data_rec;
    } params;
} hal_ble_evts_t;

typedef void (*on_ble_evt_cb_t)(hal_ble_evts_t *event, void* context);


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Check if the BLE stack is initialized.
 *
 * @returns     true if initialized, otherwise false.
 */
bool ble_stack_is_initialized(void);

/**
 * Initialize the BLE stack. This function must be called previous to any other BLE APIs.
 *
 * @param[in]   reserved    Reserved for future use.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_stack_init(void* reserved);

/**
 * Select the antenna for BLE radio.
 *
 * @param[in]   antenna The antenna type, either be BLE_ANT_INTERNAL or BLE_ANT_EXTERNAL.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_select_antenna(hal_ble_ant_type_t antenna);

/**
 * Set the callback on BLE events.
 *
 * @param[in]   callback    The callback function.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_set_callback_on_events(on_ble_evt_cb_t callback, void* context);

/**
 * Set local BLE identity address, which type must be either public or random.
 *
 * @param[in]   address Pointer to the BLE local identity address to be set.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_gap_set_device_address(hal_ble_addr_t const* address);

/**
 * Get local BLE identity address, which type is either public or random static.
 *
 * @param[out]  address Pointer to address structure to be filled in.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_gap_get_device_address(hal_ble_addr_t* address);

/**
 * Set the BLE device name.
 *
 * @param[in]   device_name Pointer to a UTF-8 encoded, <b>non NULL-terminated</b> string.
 * @param[in]   len         Length of the UTF-8, <b>non NULL-terminated</b> string pointed to by device_name
 *                          in octets (must be smaller or equal than @ref BLE_GAP_DEVNAME_MAX_LEN).
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_gap_set_device_name(uint8_t const* device_name, uint16_t len);

/**
 * Get the BLE device name.
 *
 * @param[out]      device_name Pointer to an empty buffer where the UTF-8 <b>non NULL-terminated</b> string will be placed.
 *                              Set to NULL to obtain the complete device name length.
 * @param[in,out]   p_len       Length of the buffer pointed by device_name, complete device name length on output.
 *
 * @returns         0 on success, system_error_t on error.
 */
int ble_gap_get_device_name(uint8_t* device_name, uint16_t* len);

/**
 * Set GAP Appearance value.
 *
 * @param[in]   appearance  Appearance (16-bit), see @ref BLE_APPEARANCES.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_gap_set_appearance(uint16_t appearance);

/**
 * Get GAP Appearance value.
 *
 * @param[out]  appearance  Pointer to appearance (16-bit) to be filled in, see @ref BLE_APPEARANCES.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_gap_get_appearance(uint16_t* appearance);

/**
 * For Central, it set the connection parameters.
 * For Peripheral, it set GAP Peripheral Preferred Connection Parameters.
 *
 * @param[in]   ppcp    Pointer to a hal_ble_conn_params_t structure with the desired parameters.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_gap_set_ppcp(hal_ble_conn_params_t* ppcp, void* reserved);

/**
 * Get GAP Peripheral Preferred Connection Parameters.
 *
 * @param[in]   ppcp    Pointer to a hal_ble_conn_params_t structure where the parameters will be stored.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_gap_get_ppcp(hal_ble_conn_params_t* ppcp, void* reserved);

/**
 * Set a BLE device whitelist. Only one whitelist can be used at a time and the whitelist is shared
 * between the BLE roles. The whitelist cannot be set if a BLE role is using the whitelist.
 *
 * @param[in]   addr_list   Pointer to a whitelist of peer addresses.
 * @param[in]   len         Length of the whitelist, maximum @ref BLE_GAP_WHITELIST_ADDR_MAX_COUNT.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_gap_add_whitelist(hal_ble_addr_t* addr_list, uint8_t len, void* reserved);

/**
 * Delete a BLE device whitelist.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_gap_delete_whitelist(void* reserved);

/**
 * Set the TX Power for advertisement.
 *
 * @param[in]   value   Radio transmit power in dBm.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_gap_set_tx_power(int8_t value);

/**
 * Get the TX Power for advertisement.
 *
 * @param[out]  value   Pointer to radio transmit power in dBm to be filled in.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_gap_get_tx_power(int8_t* value);

/**
 * Set the BLE advertising parameters. Changing the advertising parameter during
 * advertising will restart advertising using the new parameters.
 *
 * @param[in]   adv_params  Pointer to the advertising parameters to be set.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_gap_set_advertising_parameters(hal_ble_adv_params_t* adv_params, void* reserved);

/**
 * Set the BLE advertising data. It will update the advertising data immediately if success.
 * Changing the advertising data during advertising will restart advertising using the
 * previous advertising parameters and the new advertising data.
 *
 * @param[in]   data    Pointer to the advertising data to be set.
 * @param[in]   len     Length of the advertising data.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_gap_set_advertising_data(uint8_t* data, uint16_t len, void* reserved);

/**
 * Set the BLE scan response data.
 *
 * @param[in]   data    Pointer to the scan response data to be set.
 * @param[in]   len     Length of the scan response data.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_gap_set_scan_response_data(uint8_t* data, uint16_t len, void* reserved);

/**
 * Start BLE advertising.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_gap_start_advertising(void* reserved);

/**
 * Stop BLE advertising.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_gap_stop_advertising(void);

/**
 * Check if BLE is advertising.
 *
 * @returns     true if it is advertising, otherwise, false.
 */
bool ble_gap_is_advertising(void);

/**
 * Set the BLE scanning parameters.
 *
 * @param[in]   scan_params Pointer to the scanning parameters to be set.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_gap_set_scan_parameters(hal_ble_scan_params_t* scan_params, void* reserved);

/**
 * Start scanning nearby BLE devices.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_gap_start_scan(void* reserved);

/**
 * Check if BLE is scanning nearby devices.
 *
 * @returns     true if it is scanning, otherwise false.
 */
bool ble_gap_is_scanning(void);

/**
 * Stop scanning nearby BLE devices.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_gap_stop_scan(void);

/**
 * Connect to a peer BLE device.
 *
 * @param[in]   address Pointer to the peer BLE identity address.
 * @param[in]   params  The connection parameters for the connection to be established.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_gap_connect(hal_ble_addr_t* address, hal_ble_conn_params_t* params, void* reserved);

/**
 * Check if BLE is connecting with peer device.
 *
 * @returns true if connecting, otherwise false.
 */
bool ble_gap_is_connecting(void);

/**
 * Check if BLE is connected with peer device.
 *
 * @returns true if connected, otherwise false.
 */
bool ble_gap_is_connected(void);

/**
 * Cancel the ongoing procedure that connecting to the peer BLE device.
 *
 * @returns true if success, otherwise false.
 */
int ble_gap_connect_cancel(void);

/**
 * Terminate BLE connection.
 *
 * @param[in]   conn_handle BLE connection handle. If BLE peripheral role, it is ignored.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_gap_disconnect(uint16_t conn_handle, void* reserved);

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
int ble_gap_update_connection_params(uint16_t conn_handle, hal_ble_conn_params_t* conn_params, void* reserved);

/**
 * Get the RSSI value of the specific BLE connection.
 *
 * @param[in]   conn_handle BLE connection handle.
 *
 * @returns     the RSSI value.
 */
int ble_gap_get_rssi(uint16_t conn_handle, void* reserved);

/**
 * Add a BLE 128-bits UUID service.
 *
 * @param[in]   type    BLE service type, either BLE_SERVICE_TYPE_PRIMARY or BLE_SERVICE_TYPE_SECONDARY.
 * @param[in]   uuid    Pointer to BLE service UUID.
 * @param[out]  handle  Service handle.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_gatt_server_add_service(uint8_t type, const hal_ble_uuid_t* uuid, uint16_t* handle, void* reserved);

/**
 * Add a 128-bits UUID BLE Characteristic under a specific BLE Service.
 *
 * @note It is currently only possible to add a characteristic to the last added service (i.e. only sequential population is supported at this time).
 *
 * @param[in]   char_init   Pointer to the hal_ble_char_init_t structure.
 * @param[out]  handles     Pointer to the hal_ble_char_handles_t structure.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_gatt_server_add_characteristic(hal_ble_char_init_t* char_init, hal_ble_char_handles_t* handles, void* reserved);

/**
 * Add descriptor under a specific BLE Characteristic.
 *
 * @note It is currently only possible to add a descriptor to the last added characteristic (i.e. only sequential population is supported at this time).
 *
 * @param[in]   desc_init   Pointer to the hal_ble_desc_init_t structure.
 * @param[out]  handle      Handle of the descriptor.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_gatt_server_add_descriptor(hal_ble_desc_init_t* desc_init, uint16_t* handle, void* reserved);

/**
 * Set Characteristic value..
 *
 * @param[in]   value_handle    Characteristic value handle.
 * @param[in]   data            Pointer to the buffer that contains the data to be set.
 * @param[in]   len             Length of the data to be set.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_gatt_server_set_characteristic_value(uint16_t value_handle, uint8_t* data, uint16_t len, void* reserved);

/**
 * Get Characteristic value.
 *
 * @param[in]       value_handle    Characteristic value handle.
 * @param[in]       data            Pointer to the buffer that to be filled with the Characteristic value.
 * @param[in,out]   len             Length of the given buffer. Return the actual length of the value.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_gatt_server_get_characteristic_value(uint16_t value_handle, uint8_t* data, uint16_t* len, void* reserved);

/**
 * Send a notification to GATT Client. No response from GATT Client is required.
 *
 * @param[in]   value_handle    Characteristic value handle.
 * @param[in]   data            Pointer to the buffer that contains the data to be sent.
 * @param[in]   len             Length of the data to be sent.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_gatt_server_notify_characteristic_value(uint16_t value_handle, uint8_t* data, uint16_t len, void* reserved);

/**
 * Send an indication to GATT Client. A response from GATT Client is required.
 *
 * @param[in]   value_handle    Characteristic value handle.
 * @param[in]   data            Pointer to the buffer that contains the data to be sent.
 * @param[in]   len             Length of the data to be sent.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_gatt_server_indicate_characteristic_value(uint16_t value_handle, uint8_t* data, uint16_t len, void* reserved);

/**
 * Discover all BLE primary services.
 *
 * @param[in]   conn_handle BLE connection handle.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_gatt_client_discover_all_services(uint16_t conn_handle, void* reserved);

/**
 * Discover BLE service with specific 128-bits service UUID.
 *
 * @param[in]   conn_handle BLE connection handle.
 * @param[in]   uuid        Pointer to the service UUID.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_gatt_client_discover_service_by_uuid(uint16_t conn_handle, const hal_ble_uuid_t* uuid, void* reserved);

/**
 * Discover all BLE characteristics within specific handle range.
 *
 * @param[in]   conn_handle     BLE connection handle.
 * @param[in]   start_handle    The start handle of the handle range.
 * @param[in]   end_handle      The end handle of the handle range.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_gatt_client_discover_characteristics(uint16_t conn_handle, uint16_t start_handle, uint16_t end_handle, void* reserved);

/**
 * Discover all BLE characteristics within specific handle range.
 *
 * @param[in]   conn_handle     BLE connection handle.
 * @param[in]   start_handle    The start handle of the handle range.
 * @param[in]   end_handle      The end handle of the handle range.
 * @param[in]   uuid            The specific characteristic UUID.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_gatt_client_discover_characteristics_by_uuid(uint16_t conn_handle, uint16_t start_handle, uint16_t end_handle, const hal_ble_uuid_t* uuid, void* reserved);

/**
 * Discover all BLE descriptors within specific handle range.
 *
 * @param[in]   conn_handle     BLE connection handle.
 * @param[in]   start_handle    The start handle of the handle range.
 * @param[in]   end_handle      The end handle of the handle range.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_gatt_client_discover_descriptors(uint16_t conn_handle, uint16_t start_handle, uint16_t end_handle, void* reserved);

/**
 * Check if the BLE discovery procedure is ongoing or not.
 *
 * @returns True if BLE is discovering services, characteristics or descriptors, otherwise false.
 */
bool ble_gatt_client_is_discovering(void);

/**
 * Configure the Client Characteristic Configuration Descriptor.
 *
 * @param[in]   conn_handle     BLE connection handle.
 * @param[in]   cccd_handle     The peer device's Characteristic CCCD handle.
 * @param[in]   cccd_value      The CCCD value, it can be either BLE_SIG_CCCD_VAL_DISABLED, BLE_SIG_CCCD_VAL_NOTIFICATION or BLE_SIG_CCCD_VAL_INDICATION.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_gatt_client_configure_cccd(uint16_t conn_handle, uint16_t cccd_handle, uint8_t cccd_value, void* reserved);

/**
 * Write data to GATT server with a response required from peer device.
 *
 * @param[in]   conn_handle     BLE connection handle.
 * @param[in]   value_handle    The peer device's Characteristic value handle.
 * @param[in]   data            Pointer to the buffer that contains the data to be written.
 * @param[in]   len             Length of the data to be written.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_gatt_client_write_with_response(uint16_t conn_handle, uint16_t value_handle, uint8_t* data, uint16_t len, void* reserved);

/**
 * Write data to GATT server without a response required from peer device.
 *
 * @param[in]   conn_handle     BLE connection handle.
 * @param[in]   value_handle    The peer device's Characteristic value handle.
 * @param[in]   data            Pointer to the buffer that contains the data to be written.
 * @param[in]   len             Length of the data to be written.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_gatt_client_write_without_response(uint16_t conn_handle, uint16_t value_handle, uint8_t* data, uint16_t len, void* reserved);

/**
 * Read data from GATT server. The data is returned through a BLE event.
 *
 * @param[in]   conn_handle     BLE connection handle.
 * @param[in]   value_handle    The peer device's Characteristic value handle.
 *
 * @returns     0 on success, system_error_t on error.
 */
int ble_gatt_client_read(uint16_t conn_handle, uint16_t attr_handle, void* reserved);


#ifdef __cplusplus
} // extern "C"
#endif


/**
 * @}
 */

#endif /* BLE_HAL_API_H */
