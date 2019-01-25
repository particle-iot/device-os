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


#include "ble_gap.h"
#include "ble_gatts.h"

#define BLE_ROLE_INVALID                            BLE_GAP_ROLE_INVALID
#define BLE_ROLE_PERIPHERAL                         BLE_GAP_ROLE_PERIPH
#define BLE_ROLE_CENTRAL                            BLE_GAP_ROLE_CENTRAL

/* Maximum allowed BLE event callback that can be registered. */
#define BLE_MAX_EVENT_CALLBACK_COUNT                10

/* BLE event queue depth */
#define BLE_EVENT_QUEUE_ITEM_COUNT                  10

/* BLE event thread stack size */
#define BLE_EVENT_THREAD_STACK_SIZE                 2048

/* BLE invalid connection handle. */
#define BLE_INVALID_CONN_HANDLE                     BLE_CONN_HANDLE_INVALID

/* BLE invalid attribute handle. */
#define BLE_INVALID_ATTR_HANDLE                     0x00

/* Maximum number of device address in the whitelist. */
#define BLE_MAX_WHITELIST_ADDR_COUNT                BLE_GAP_WHITELIST_ADDR_MAX_COUNT

/* BLE advertising event type */
#define BLE_ADV_CONNECTABLE_SCANNABLE_UNDIRECRED_EVT        BLE_GAP_ADV_TYPE_CONNECTABLE_SCANNABLE_UNDIRECTED
#define BLE_ADV_CONNECTABLE_UNDIRECTED_EVT                  BLE_GAP_ADV_TYPE_EXTENDED_CONNECTABLE_NONSCANNABLE_UNDIRECTED
#define BLE_ADV_CONNECTABLE_DIRECTED_EVT                    BLE_GAP_ADV_TYPE_CONNECTABLE_NONSCANNABLE_DIRECTED
#define BLE_ADV_NON_CONNECTABLE_NON_SCANABLE_UNDIRECTED_EVT BLE_GAP_ADV_TYPE_NONCONNECTABLE_NONSCANNABLE_UNDIRECTED
#define BLE_ADV_NON_CONNECTABLE_NON_SCANABLE_DIRECTED_EVT   BLE_GAP_ADV_TYPE_EXTENDED_NONCONNECTABLE_NONSCANNABLE_DIRECTED
#define BLE_ADV_SCANABLE_UNDIRECTED_EVT                     BLE_GAP_ADV_TYPE_NONCONNECTABLE_SCANNABLE_UNDIRECTED
#define BLE_ADV_SCANABLE_DIRECTED_EVT                       BLE_GAP_ADV_TYPE_EXTENDED_NONCONNECTABLE_SCANNABLE_DIRECTED

/* Default advertising parameters */
#define BLE_DEFAULT_ADVERTISING_INTERVAL            160     /* The advertising interval: 100ms (in units of 0.625 ms). */
#define BLE_DEFAULT_ADVERTISING_DURATION            18000   /* The advertising duration: 180s (in units of 10 milliseconds). */

/* Default scanning parameters */
#define BLE_DEFAULT_SCANNING_INTERVAL               160     /* The scan interval: 100ms (in units of 0.625 ms). */
#define BLE_DEFAULT_SCANNING_WINDOW                 80      /* The scan window: 50ms (in units of 0.625 ms). */
#define BLE_DEFAULT_SCANNING_TIMEOUT                0       /* The timeout: forever (in units of 10 ms. 0 for scanning forever). */

/* Maximum length of advertising and scan response data */
#define BLE_MAX_ADV_DATA_LEN                        BLE_GAP_ADV_SET_DATA_SIZE_MAX

/* Maximum length of the buffer to store scan report data */
#define BLE_MAX_SCAN_REPORT_BUF_LEN                 BLE_GAP_SCAN_BUFFER_MAX

/* Connection Parameters limits */
#define BLE_CONN_PARAMS_SLAVE_LATENCY_ERR           5
#define BLE_CONN_PARAMS_TIMEOUT_ERR                 100

#define BLE_CONN_PARAMS_UPDATE_DELAY_MS             5000
#define BLE_CONN_PARAMS_UPDATE_ATTEMPS              2

/* Default BLE connection parameters */
#define BLE_DEFAULT_MIN_CONN_INTERVAL               80      /* The minimal connection interval: 100ms (in units of 1.25ms). */
#define BLE_DEFAULT_MAX_CONN_INTERVAL               400     /* The minimal connection interval: 500ms (in units of 1.25ms). */
#define BLE_DEFAULT_SLAVE_LATENCY                   0       /* The slave latency. */
#define BLE_DEFAULT_CONN_SUP_TIMEOUT                400     /* The connection supervision timeout: 4s (in units of 10ms). */

/* BLE Service type */
#define BLE_SERVICE_TYPE_INVALID                    BLE_GATTS_SRVC_TYPE_INVALID
#define BLE_SERVICE_TYPE_PRIMARY                    BLE_GATTS_SRVC_TYPE_PRIMARY
#define BLE_SERVICE_TYPE_SECONDARY                  BLE_GATTS_SRVC_TYPE_SECONDARY

/* BLE UUID type field in the hal_ble_uuid_t */
#define BLE_UUID_TYPE_16BIT                         0
#define BLE_UUID_TYPE_128BIT                        1
#define BLE_UUID_TYPE_128BIT_SHORTED                2

/* Maximum BLE Characteristic value length */
#define BLE_MAX_CHAR_VALUE_LEN                      20

#define BLE_MAX_SVC_COUNT                           5
#define BLE_MAX_CHAR_COUNT                          10
#define BLE_MAX_DESC_COUNT                          10


#endif /* BLE_HAL_API_IMPL_H */

