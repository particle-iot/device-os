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

#ifndef BLE_HAL_IMPL_H
#define BLE_HAL_IMPL_H


#if HAL_PLATFORM_BLE

#include "ble_gap.h"
#include "ble_gatts.h"
#include "sdk_config_system.h"


/**< Number of microseconds in 0.625 milliseconds. */
#define BLE_UNIT_0_625_MS                           625
/**< Number of microseconds in 1.25 milliseconds. */
#define BLE_UNIT_1_25_MS                            1250
/**< Number of microseconds in 10 milliseconds. */
#define BLE_UNIT_10_MS                              10000

#define BLE_MSEC_TO_UNITS(TIME, RESOLUTION)         (((TIME) * 1000) / (RESOLUTION))

#define BLE_ROLE_INVALID                            BLE_GAP_ROLE_INVALID
#define BLE_ROLE_PERIPHERAL                         BLE_GAP_ROLE_PERIPH
#define BLE_ROLE_CENTRAL                            BLE_GAP_ROLE_CENTRAL

#define BLE_MAX_LINK_COUNT                          ((NRF_SDH_BLE_CENTRAL_LINK_COUNT) + (NRF_SDH_BLE_PERIPHERAL_LINK_COUNT))

/* Maximum allowed BLE event callback that can be registered. */
#define BLE_MAX_EVENT_CALLBACK_COUNT                10

/* BLE event queue depth */
#define BLE_EVENT_QUEUE_ITEM_COUNT                  10

/* Maximum length of device name, non null-terminated */
#define BLE_MAX_DEV_NAME_LEN                        20

/* BLE event thread stack size */
#define BLE_EVENT_THREAD_STACK_SIZE                 2048

/* BLE invalid connection handle. */
#define BLE_INVALID_CONN_HANDLE                     BLE_CONN_HANDLE_INVALID

/* BLE invalid attribute handle. */
#define BLE_INVALID_ATTR_HANDLE                     0x0000

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
#define BLE_DEFAULT_ADVERTISING_TIMEOUT             0       /* The advertising duration: infinite (in units of 10 milliseconds). */

/* BLE advertisement filter policy */
#define BLE_ADV_FP_ANY                              0x00   /**< Allow scan requests and connect requests from any device. */
#define BLE_ADV_FP_FILTER_SCANREQ                   0x01   /**< Filter scan requests with whitelist. */
#define BLE_ADV_FP_FILTER_CONNREQ                   0x02   /**< Filter connect requests with whitelist. */
#define BLE_ADV_FP_FILTER_BOTH                      0x03   /**< Filter both scan and connect requests with whitelist. */

/* Default scanning parameters */
#define BLE_DEFAULT_SCANNING_INTERVAL               160     /* The scan interval: 100ms (in units of 0.625 ms). */
#define BLE_DEFAULT_SCANNING_WINDOW                 80      /* The scan window: 50ms (in units of 0.625 ms). */
#define BLE_DEFAULT_SCANNING_TIMEOUT                500     /* The timeout: forever (in units of 10 ms. 0 for scanning forever). */

/* BLE scan filter policy */
#define BLE_SCAN_FP_ACCEPT_ALL                      0x00  /**< Accept all advertising packets except directed advertising packets
                                                                   not addressed to this device. */
#define BLE_SCAN_FP_WHITELIST                       0x01  /**< Accept advertising packets from devices in the whitelist except directed
                                                                   packets not addressed to this device. */
#define BLE_SCAN_FP_ALL_NOT_RESOLVED_DIRECTED       0x02  /**< Accept all advertising packets specified in @ref BLE_GAP_SCAN_FP_ACCEPT_ALL.
                                                                   In addition, accept directed advertising packets, where the advertiser's
                                                                   address is a resolvable private address that cannot be resolved. */
#define BLE_SCAN_FP_WHITELIST_NOT_RESOLVED_DIRECTED 0x03  /**< Accept all advertising packets specified in @ref BLE_GAP_SCAN_FP_WHITELIST.
                                                                   In addition, accept directed advertising packets, where the advertiser's
                                                                   address is a resolvable private address that cannot be resolved. */

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
#define BLE_DEFAULT_MIN_CONN_INTERVAL               BLE_MSEC_TO_UNITS(30, BLE_UNIT_1_25_MS)     /* The minimal connection interval: 30ms (in units of 1.25ms). */
#define BLE_DEFAULT_MAX_CONN_INTERVAL               BLE_MSEC_TO_UNITS(45, BLE_UNIT_1_25_MS)     /* The minimal connection interval: 45ms (in units of 1.25ms). */
#define BLE_DEFAULT_SLAVE_LATENCY                   0                                           /* The slave latency. */
#define BLE_DEFAULT_CONN_SUP_TIMEOUT                BLE_MSEC_TO_UNITS(5000, BLE_UNIT_10_MS)     /* The connection supervision timeout: 5s (in units of 10ms). */

/* BLE Service type */
#define BLE_SERVICE_TYPE_INVALID                    BLE_GATTS_SRVC_TYPE_INVALID
#define BLE_SERVICE_TYPE_PRIMARY                    BLE_GATTS_SRVC_TYPE_PRIMARY
#define BLE_SERVICE_TYPE_SECONDARY                  BLE_GATTS_SRVC_TYPE_SECONDARY

/* BLE UUID type field in the hal_ble_uuid_t */
#define BLE_UUID_TYPE_16BIT                         0
#define BLE_UUID_TYPE_128BIT                        1
#define BLE_UUID_TYPE_128BIT_SHORTED                2

// Maximum supported size of an ATT packet in bytes (ATT_MTU)
#define BLE_MAX_ATT_MTU_SIZE                        NRF_SDH_BLE_GATT_MAX_MTU_SIZE

// Default maximum size of an ATT packet in bytes (ATT_MTU)
#define BLE_MIN_ATT_MTU_SIZE                        23

// Size of the ATT opcode field in bytes
#define BLE_ATT_OPCODE_SIZE                         1

// Size of the ATT handle field in bytes
#define BLE_ATT_HANDLE_SIZE                         2

// Minimum and maximum number of bytes that can be sent in a single write command, read response,
// notification or indication packet
#define BLE_MIN_ATTR_VALUE_PACKET_SIZE              (BLE_MIN_ATT_MTU_SIZE - BLE_ATT_OPCODE_SIZE - BLE_ATT_HANDLE_SIZE)
#define BLE_MAX_ATTR_VALUE_PACKET_SIZE              (BLE_MAX_ATT_MTU_SIZE - BLE_ATT_OPCODE_SIZE - BLE_ATT_HANDLE_SIZE)

#define BLE_MAX_SVC_COUNT                           5
#define BLE_MAX_CHAR_COUNT                          10
#define BLE_MAX_DESC_COUNT                          10

#define BLE_MAX_PERIPHERAL_COUNT                    NRF_SDH_BLE_PERIPHERAL_LINK_COUNT
#define BLE_MAX_CENTRAL_COUNT                       NRF_SDH_BLE_CENTRAL_LINK_COUNT


#endif //HAL_PLATFORM_BLE


#endif /* BLE_HAL_API_IMPL_H */

