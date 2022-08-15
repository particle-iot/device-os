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



/**< Number of microseconds in 0.625 milliseconds. */
#define BLE_UNIT_0_625_MS                           625
/**< Number of microseconds in 1.25 milliseconds. */
#define BLE_UNIT_1_25_MS                            1250
/**< Number of microseconds in 10 milliseconds. */
#define BLE_UNIT_10_MS                              10000

#define BLE_MSEC_TO_UNITS(TIME, RESOLUTION)         (((TIME) * 1000) / (RESOLUTION))

#define BLE_MAX_LINK_COUNT                          4

/* Maximum allowed BLE event callback that can be registered. */
#define BLE_MAX_EVENT_CALLBACK_COUNT                10

/* BLE event queue depth */
#define BLE_EVENT_QUEUE_ITEM_COUNT                  30

/* Maximum length of device name, non null-terminated */
#define BLE_MAX_DEV_NAME_LEN                        20

/* Maximum length of characteristic description */
#define BLE_MAX_DESC_LEN                            20

/* BLE event thread stack size */
#define BLE_EVENT_THREAD_STACK_SIZE                 (10 * 1024)

/* BLE invalid connection handle. */
#define BLE_INVALID_CONN_HANDLE                     0xFFFF

/* BLE invalid attribute handle. */
#define BLE_INVALID_ATTR_HANDLE                     0x0000

/* Maximum number of device address in the whitelist. */
#define BLE_MAX_WHITELIST_ADDR_COUNT                10

/* Default advertising parameters */
#define BLE_DEFAULT_ADVERTISING_INTERVAL            BLE_MSEC_TO_UNITS(100, BLE_UNIT_0_625_MS)   /* The advertising interval: 100ms (in units of 0.625 ms). */
#define BLE_DEFAULT_ADVERTISING_TIMEOUT             BLE_MSEC_TO_UNITS(0, BLE_UNIT_10_MS)        /* The advertising duration: infinite (in units of 10 milliseconds). */

#define BLE_MAX_TX_POWER                            (0)

#define BLE_SCAN_INTERVAL_MIN                       0x0004
#define BLE_SCAN_INTERVAL_MAX                       0x4000
#define BLE_SCAN_WINDOW_MIN                         0x0004
#define BLE_SCAN_WINDOW_MAX                         0x4000
#define BLE_SCAN_TIMEOUT_UNLIMITED                  0

/* Default scanning parameters */
#define BLE_DEFAULT_SCANNING_TIMEOUT_MS             5000
#define BLE_DEFAULT_SCANNING_INTERVAL               BLE_MSEC_TO_UNITS(100, BLE_UNIT_0_625_MS)   /* The scan interval: 100ms (in units of 0.625 ms). */
#define BLE_DEFAULT_SCANNING_WINDOW                 BLE_MSEC_TO_UNITS(50, BLE_UNIT_0_625_MS)    /* The scan window: 50ms (in units of 0.625 ms). */
#define BLE_DEFAULT_SCANNING_TIMEOUT                BLE_MSEC_TO_UNITS(BLE_DEFAULT_SCANNING_TIMEOUT_MS, BLE_UNIT_10_MS)     /* The timeout: 5000ms (in units of 10 ms. 0 for scanning forever). */
// Extended timeout for the scanning timeout guard timer
#define BLE_SCANNING_TIMEOUT_EXT_MS                 1000

/* Maximum length of advertising and scan response data */
#define BLE_MAX_ADV_DATA_LEN                        31

/* Maximum length of the buffer to store scan report data */
#define BLE_MAX_SCAN_REPORT_BUF_LEN                 255  /* Must support extended length for CODED_PHY scanning */

/* Maximum length for CODED_PHY */
#define BLE_MAX_ADV_DATA_LEN_EXT                    255
#define BLE_MAX_ADV_DATA_LEN_EXT_CONNECTABLE        238

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

// Maximum supported size of an ATT packet in bytes (ATT_MTU)
#define BLE_MAX_ATT_MTU_SIZE                        247

// Minimum supported size of an ATT packet in bytes (ATT_MTU)
#define BLE_MIN_ATT_MTU_SIZE                        23

#define BLE_DEFAULT_ATT_MTU_SIZE                    BLE_MIN_ATT_MTU_SIZE

// Size of the ATT opcode field in bytes
#define BLE_ATT_OPCODE_SIZE                         1

// Size of the ATT handle field in bytes
#define BLE_ATT_HANDLE_SIZE                         2

// Minimum and maximum number of bytes that can be sent in a single write command, read response,
// notification or indication packet
#define BLE_MIN_ATTR_VALUE_PACKET_SIZE              (BLE_MIN_ATT_MTU_SIZE - BLE_ATT_OPCODE_SIZE - BLE_ATT_HANDLE_SIZE)
#define BLE_MAX_ATTR_VALUE_PACKET_SIZE              (BLE_MAX_ATT_MTU_SIZE - BLE_ATT_OPCODE_SIZE - BLE_ATT_HANDLE_SIZE)
#define BLE_ATTR_VALUE_PACKET_SIZE(ATT_MTU)         (ATT_MTU - BLE_ATT_OPCODE_SIZE - BLE_ATT_HANDLE_SIZE)

#define BLE_MAX_SVC_COUNT                           21
#define BLE_MAX_CHAR_COUNT                          23
#define BLE_MAX_DESC_COUNT                          10

#define BLE_MAX_PERIPHERAL_COUNT                    1
#define BLE_MAX_CENTRAL_COUNT                       3


typedef uint16_t hal_ble_attr_handle_t;
typedef uint16_t hal_ble_conn_handle_t;


#endif //HAL_PLATFORM_BLE


#endif /* BLE_HAL_API_IMPL_H */

