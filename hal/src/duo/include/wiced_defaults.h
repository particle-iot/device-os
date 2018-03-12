/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

/************************************************************************
 * ** WARNING for PRINTING **
 *  If printing is enabled, the stack of each thread that uses printing
 *  must be increased to at least 4 kBytes since the printf function uses
 *  a lot of memory (including dynamic memory)
 */

/* Select which group of functions are allowed to print */
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
//#define WPRINT_ENABLE_APP_INFO           /* Application prints */
//#define WPRINT_ENABLE_APP_DEBUG
//#define WPRINT_ENABLE_APP_ERROR
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
//#define WPRINT_ENABLE_LIB_INFO           /* General library prints */
//#define WPRINT_ENABLE_LIB_DEBUG
//#define WPRINT_ENABLE_LIB_ERROR
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
//#define WPRINT_ENABLE_WEBSERVER_INFO     /* Webserver prints */
//#define WPRINT_ENABLE_WEBSERVER_DEBUG
//#define WPRINT_ENABLE_WEBSERVER_ERROR

#ifndef DISABLE_LOGGING
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
//#define WPRINT_ENABLE_NETWORK_INFO       /* Network stack prints */
//#define WPRINT_ENABLE_NETWORK_DEBUG
//#define WPRINT_ENABLE_NETWORK_ERROR
#endif
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
//#define WPRINT_ENABLE_RTOS_INFO          /* RTOS prints */
//#define WPRINT_ENABLE_RTOS_DEBUG
//#define WPRINT_ENABLE_RTOS_ERROR

//#define WPRINT_ENABLE_SECURITY_INFO    /* Security stack prints */
//#define WPRINT_ENABLE_SECURITY_DEBUG
//#define WPRINT_ENABLE_SECURITY_ERROR
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
//#define WPRINT_ENABLE_WPS_INFO           /* WPS stack prints */
//#define WPRINT_ENABLE_WPS_DEBUG
//#define WPRINT_ENABLE_WPS_ERROR
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
// #define WPRINT_ENABLE_SUPPLICANT_INFO    /* Supplicant stack prints */
// #define WPRINT_ENABLE_SUPPLICANT_DEBUG
// #define WPRINT_ENABLE_SUPPLICANT_ERROR

#ifndef DISABLE_LOGGING
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
//#define WPRINT_ENABLE_WICED_INFO         /* Wiced internal prints */
//#define WPRINT_ENABLE_WICED_DEBUG
//#define WPRINT_ENABLE_WICED_ERROR
#endif
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
//#define WPRINT_ENABLE_WWD_INFO         /* Wiced Wi-Fi Driver prints */
//#define WPRINT_ENABLE_WWD_DEBUG
//#define WPRINT_ENABLE_WWD_ERROR
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
//#define WPRINT_ENABLE_PLATFORM_INFO      /* Platform prints */
//#define WPRINT_ENABLE_PLATFORM_DEBUG
//#define WPRINT_ENABLE_PLATFORM_ERROR


/************************************************************************
 * Default WICED networking timeouts in milliseconds */
#define WICED_ALLOCATE_PACKET_TIMEOUT             (2000)
#define WICED_TCP_DISCONNECT_TIMEOUT              (3000)
#define WICED_TCP_BIND_TIMEOUT                    (3000)
#define WICED_TCP_SEND_TIMEOUT                    (3000)
#define WICED_TCP_ACCEPT_TIMEOUT                  (3000)
#define WICED_UDP_BIND_TIMEOUT                    (3000)
#define WICED_NTP_REPLY_TIMEOUT                   (5000)
#define WICED_TLS_RECEIVE_TIMEOUT                 (5000)
#define WICED_TLS_TRANSMIT_TIMEOUT                (5000)
#define WICED_DHCP_IP_ADDRESS_RESOLUTION_TIMEOUT (15000)
#define WICED_AUTO_IP_ADDRESS_RESOLUTION_TIMEOUT (15000)

/************************************************************************
 * Default WICED IOCTL/IOVAR timeouts in milliseconds */
#define WICED_DEFAULT_IOCTL_PACKET_TIMEOUT       ( WICED_NEVER_TIMEOUT )

/************************************************************************
 *   WICED thread priority table
 *
 * +----------+-----------------+
 * | Priority |      Thread     |
 * |----------|-----------------|
 * |     0    |      Wiced      |   Highest priority
 * |     1    |     Network     |
 * |     2    |                 |
 * |     3    | Network worker  |
 * |     4    |                 |
 * |     5    | Default Library |
 * |          | Default worker  |
 * |     6    |                 |
 * |     7    |   Application   |
 * |     8    |                 |
 * |     9    |      Idle       |   Lowest priority
 * +----------+-----------------+
 */
#define WICED_NETWORK_WORKER_PRIORITY      (3)
#define WICED_DEFAULT_WORKER_PRIORITY      (5)
#define WICED_DEFAULT_LIBRARY_PRIORITY     (5)
#define WICED_APPLICATION_PRIORITY         (7)

/************************************************************************
 * WICED TCP Options */
#define WICED_TCP_WINDOW_SIZE                 (7 * 1024)
#define WICED_DEFAULT_TCP_LISTEN_QUEUE_SIZE   (5)
#define WICED_DEFAULT_TCP_TX_DEPTH_QUEUE      (5)
#define WICED_DEFAULT_TCP_RX_DEPTH_QUEUE      (5)
#define WICED_DEFAULT_TCP_TX_RETRIES          (10)

/************************************************************************
 * WICED UDP Options */
#define WICED_DEFAULT_UDP_QUEUE_SIZE          (5)

/************************************************************************
 * WICED Join Options */
#define WICED_JOIN_RETRY_ATTEMPTS             (3)

/************************************************************************
 * WICED TLS Options */
#define WICED_TLS_MAX_RESUMABLE_SESSIONS      (4)
#define WICED_TLS_DEFAULT_VERIFICATION        (TLS_VERIFICATION_REQUIRED)

/************************************************************************
 * Country code */
#define WICED_DEFAULT_COUNTRY_CODE            ( WICED_COUNTRY_UNITED_STATES )

/************************************************************************
 * Uncomment to enable Auto country support */
//#define WICED_ENABLE_AUTO_COUNTRY

/************************************************************************
 * Application thread stack size */
#define WICED_DEFAULT_APPLICATION_STACK_SIZE    (6144)

/************************************************************************
 * Soft AP Options */
#define WICED_DEFAULT_SOFT_AP_DTIM_PERIOD       (1)

/************************************************************************
 * WICED Wi-Fi Direct Options */
#define P2P_MAX_ASSOCIATED_DEVICES              (5)

/************************************************************************
 * WICED Connectivity Options */
#define WICED_USE_WIFI_STA_INTERFACE
#define WICED_USE_WIFI_AP_INTERFACE
//#define WICED_USE_WIFI_P2P_INTERFACE
//#define WICED_USE_ETHERNET_INTERFACE

/************************************************************************
 * WICED WiFi Roaming related options (for STA interface)
 * See wiced_wifi_set_roam_trigger() for details */
#define WICED_WIFI_ROAMING_TRIGGER_MODE             ( WICED_WIFI_OPTIMIZE_BANDWIDTH_ROAMING_TRIGGER )
#define WICED_WIFI_ROAMING_TRIGGER_DELTA_IN_DBM     ( 5 )
#define WICED_WIFI_ROAMING_SCAN_PERIOD_IN_SECONDS   ( 10 )

/************************************************************************
 * Uncomment to "hide" the soft AP */
//#define WICED_DISABLE_SSID_BROADCAST

/************************************************************************
 * Uncomment to prevent soft AP clients from communicating with each other */
//#define WICED_WIFI_ISOLATE_AP_CLIENTS

/************************************************************************
 * Uncomment to disable AMPDU transmission */
//#define WICED_DISABLE_AMPDU_TX

/************************************************************************
 * Uncomment to disable watchdog. For debugging only */
//#define WICED_DISABLE_WATCHDOG

/************************************************************************
 * Uncomment to disable standard IO, i.e. printf(), etc. */
//#define WICED_DISABLE_STDIO

/************************************************************************
 * Uncomment to disable MCU powersave API functions */
//#define WICED_DISABLE_MCU_POWERSAVE

/************************************************************************
 * Uncomment to enable MCU real time clock */
//#define WICED_ENABLE_MCU_RTC

/************************************************************************
 * Uncomment both to disable TLS completely*/
#define WICED_DISABLE_TLS

#ifdef __cplusplus
} /*extern "C" */
#endif
