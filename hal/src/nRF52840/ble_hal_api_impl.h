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

#ifndef BLE_HAL_API_IMPL_H
#define BLE_HAL_API_IMPL_H

/* Headers included from nRF5_SDK/components/softdevice/s140/headers */
#include "ble.h"
#include "ble_gap.h"
#include "ble_gatt.h"
#include "ble_gatts.h"
#include "ble_gattc.h"
#include "ble_types.h"
/* Headers included from nRF5_SDK/components/softdevice/common */
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_soc.h"

#include "app_util.h"


#define BLE_ROLE_INVALID                            BLE_GAP_ROLE_INVALID
#define BLE_ROLE_PERIPHERAL                         BLE_GAP_ROLE_PERIPH
#define BLE_ROLE_CENTRAL                            BLE_GAP_ROLE_CENTRAL

/* Maximum allowed BLE event callback that can be registered. */
#define BLE_MAX_EVENT_CALLBACK_COUNT                10

/* BLE event queue depth */
#define BLE_EVENT_QUEUE_ITEM_COUNT                  10

/* BLE event thread stack size */
#define BLE_EVENT_THREAD_STACK_SIZE                 1024

/* BLE invalid connection handle. */
#define BLE_INVALID_CONN_HANDLE                     BLE_CONN_HANDLE_INVALID

/* BLE disconnect reason */
#define BLE_DISCONN_REASON_USER_INITIATE            BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION
#define BLE_DISCONN_REASON_CONN_INTERVAL            BLE_HCI_CONN_INTERVAL_UNACCEPTABLE

/* Maximum number of device address in the whitelist. */
#define BLE_MAX_WHITELIST_ADDR_COUNT                BLE_GAP_WHITELIST_ADDR_MAX_COUNT

/* Default advertising parameters */
#define BLE_DEFAULT_ADVERTISING_INTERVAL            160     /* The advertising interval (in units of 0.625 ms). */
#define BLE_DEFAULT_ADVERTISING_DURATION            18000   /* The advertising duration (in units of 10 milliseconds). */

/* Default scanning parameters */
#define BLE_DEFAULT_SCANNING_INTERVAL               0x00A0  /* The scan interval in units of 0.625 millisecond. */
#define BLE_DEFAULT_SCANNING_WINDOW                 0x0050  /* The scan window in units of 0.625 millisecond. */
#define BLE_DEFAULT_SCANNING_TIMEOUT                0x0000  /* The timeout in units of 10 millisecond when scanning. 0x0000 disables timeout. */

/* Maximum length of advertising and scan response data */
#define BLE_MAX_ADV_DATA_LEN                        BLE_GAP_ADV_SET_DATA_SIZE_MAX

/* Maximum length of the buffer to store scan report data */
#define BLE_MAX_SCAN_REPORT_BUF_LEN                 BLE_GAP_SCAN_BUFFER_MAX

/* Connection Parameters limits */
#define BLE_CONN_PARAMS_SLAVE_LATENCY_ERR           5
#define BLE_CONN_PARAMS_TIMEOUT_ERR                 100

#define BLE_CONN_PARAMS_UPDATE_DELAY_MS             5000
#define BLE_CONN_PARAMS_UPDATE_ATTEMPS              2


/* BLE Service type */
#define BLE_SERVICE_TYPE_INVALID                    BLE_GATTS_SRVC_TYPE_INVALID
#define BLE_SERVICE_TYPE_PRIMARY                    BLE_GATTS_SRVC_TYPE_PRIMARY
#define BLE_SERVICE_TYPE_SECONDARY                  BLE_GATTS_SRVC_TYPE_SECONDARY

/* BLE UUID type field in the ble_uuid_t */
#define BLE_UUID_TYPE_16BIT                         0
#define BLE_UUID_TYPE_128BIT                        1

/* Maximum allowed Characteristic and Service */
#define BLE_MAX_SERVICE_COUNT                       5
#define BLE_MAX_CHARS_PER_SERVICE_COUNT             10

/* Maximum BLE Characteristic value length */
#define BLE_MAX_CHAR_VALUE_LEN                      20


#endif /* BLE_HAL_API_IMPL_H */

