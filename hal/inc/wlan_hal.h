/**
 ******************************************************************************
 * @file    wlan_hal.h
 * @author  Matthew McGowan
 * @version V1.0.0
 * @date    25-Sept-2014
 * @brief
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

#ifndef WLAN_H
#define	WLAN_H

#include <stdint.h>
#include <stdbool.h>
#include "debug.h"
#include "inet_hal.h"
#include "socket_hal.h"


#ifdef	__cplusplus
extern "C" {
#endif

//#define DEBUG_WIFI    // Define to show all the flags in debug output
//#define DEBUG_WAN_WD  // Define to show all SW WD activity in debug output
    
#if defined(DEBUG_WAN_WD)
#define WAN_WD_DEBUG(x,...) DEBUG(x,__VA_ARGS__)
#else
#define WAN_WD_DEBUG(x,...)
#endif
extern uint32_t wlan_watchdog;
#define ARM_WLAN_WD(x) do { wlan_watchdog = HAL_Timer_Get_Milli_Seconds()+(x); WAN_WD_DEBUG("WD Set "#x" %d",(x));}while(0)
#define WLAN_WD_TO() (wlan_watchdog && (HAL_Timer_Get_Milli_Seconds() >= wlan_watchdog))
#define CLR_WLAN_WD() do { wlan_watchdog = 0; WAN_WD_DEBUG("WD Cleared, was %d",wlan_watchdog);;}while(0)
    
#if defined(DEBUG_WIFI)
extern uint32_t lastEvent;

#define SET_LAST_EVENT(x) do {lastEvent = (x);} while(0)
#define GET_LAST_EVENT(x) do { x = lastEvent; lastEvent = 0;} while(0)
#define DUMP_STATE() do { \
    DEBUG("\r\nWLAN_MANUAL_CONNECT=%d\r\nWLAN_DELETE_PROFILES=%d\r\nWLAN_SMART_CONFIG_START=%d\r\nWLAN_SMART_CONFIG_STOP=%d", \
          WLAN_MANUAL_CONNECT,WLAN_DELETE_PROFILES,WLAN_SMART_CONFIG_START, WLAN_SMART_CONFIG_STOP); \
    DEBUG("\r\nWLAN_SMART_CONFIG_FINISHED=%d\r\nWLAN_SERIAL_CONFIG_DONE=%d\r\nWLAN_CONNECTED=%d\r\nWLAN_DHCP=%d\r\nWLAN_CAN_SHUTDOWN=%d", \
          WLAN_SMART_CONFIG_FINISHED,WLAN_SERIAL_CONFIG_DONE,WLAN_CONNECTED,WLAN_DHCP,WLAN_CAN_SHUTDOWN); \
    DEBUG("\r\nSPARK_WLAN_RESET=%d\r\nSPARK_WLAN_SLEEP=%d\r\nSPARK_WLAN_STARTED=%d\r\nSPARK_CLOUD_CONNECT=%d", \
           SPARK_WLAN_RESET,SPARK_WLAN_SLEEP,SPARK_WLAN_STARTED,SPARK_CLOUD_CONNECT); \
    DEBUG("\r\nSPARK_CLOUD_SOCKETED=%d\r\nSPARK_CLOUD_CONNECTED=%d\r\nSPARK_FLASH_UPDATE=%d\r\nSPARK_LED_FADE=%d\r\n", \
           SPARK_CLOUD_SOCKETED,SPARK_CLOUD_CONNECTED,SPARK_FLASH_UPDATE,SPARK_LED_FADE); \
 } while(0)

#define ON_EVENT_DELTA()  do { if (lastEvent != 0) { uint32_t l; GET_LAST_EVENT(l); DEBUG("\r\nAsyncEvent 0x%04x", l); DUMP_STATE();}} while(0)
#else
#define SET_LAST_EVENT(x)
#define GET_LAST_EVENT(x)
#define DUMP_STATE()
#define ON_EVENT_DELTA()
#endif

typedef enum
{
  ANT_INTERNAL = 0, ANT_EXTERNAL = 1, ANT_AUTO = 3
} WLanSelectAntenna_TypeDef;

typedef struct __attribute__((__packed__))  _WLanConfig_t {    
    uint16_t size;
    NetworkConfig nw;
    uint8_t uaSSID[33];
} WLanConfig;

typedef int wlan_result_t;

/**
 * Connect start the wireless connection.
 */
wlan_result_t  wlan_connect_init();

/**
 * Finalize the connection by connecting to stored profiles.
 */
wlan_result_t  wlan_connect_finalize();


bool wlan_reset_credentials_store_required();
wlan_result_t  wlan_reset_credentials_store();

void Set_NetApp_Timeout();
void Clear_NetApp_Dhcp();

wlan_result_t wlan_disconnect_now();

/**
 * Enable wifi without connecting.
 */
wlan_result_t wlan_activate();

/**
 * Disable wifi.
 */
wlan_result_t wlan_deactivate();



/**
 * @return <0 for a valid signal strength, in db. 
 *         0 for rssi not found (caller could retry)
 *         >0 for an error
 */
int wlan_connected_rssi();

int wlan_clear_credentials();
int wlan_has_credentials();

// Provide compatibility with the original cc3000 headers.
#ifdef WLAN_SEC_UNSEC
#undef WLAN_SEC_UNSEC
#undef WLAN_SEC_WEP
#undef WLAN_SEC_WPA
#undef WLAN_SEC_WPA2
#endif
typedef enum {
    WLAN_SEC_UNSEC,
    WLAN_SEC_WEP,
    WLAN_SEC_WPA,
    WLAN_SEC_WPA2,
    WLAN_SEC_NOT_SET
} WLanSecurityType;


typedef enum {
    WLAN_CIPHER_NOT_SET = 0,
    WLAN_CIPHER_AES = 1,
    WLAN_CIPHER_TKIP = 2,
    WLAN_CIPHER_AES_TKIP = 3   // OR of AES and TKIP
} WLanSecurityCipher;

typedef struct {
    unsigned len;           // the size of this structure. allows older clients to work with newer HAL. 
    const char* ssid;
    unsigned ssid_len;
    const char* password;
    unsigned password_len;
    WLanSecurityType security;
    WLanSecurityCipher cipher;
    unsigned channel;
} WLanCredentials;

int wlan_set_credentials(WLanCredentials* credentials);

/**
 * Initialize smart config mode.
 */
void wlan_smart_config_init();
void wlan_smart_config_cleanup();

void wlan_set_error_count(uint32_t error_count);


/**
 * Finalize after profiles established.
 * @return true the wifi profiles were changed
 */
bool wlan_smart_config_finalize();

/**
 * Retrieve IP address info. Available after HAL_WLAN_notify_dhcp() has been callted.
 */
void wlan_fetch_ipconfig(WLanConfig* config);

/**
 * Called once at startup to initialize the wlan hardware.
 */
void wlan_setup();

void welan_set_error_count();

void SPARK_WLAN_SmartConfigProcess();

uint32_t HAL_WLAN_SetNetWatchDog(uint32_t timeOutInuS);

void HAL_WLAN_notify_simple_config_done();

/**
 * Notification that the wifi network has been connected to. 
 */
void HAL_WLAN_notify_connected();   
void HAL_WLAN_notify_disconnected();

/**
 * Notification that an IP address has been received via DHCP.
 * todo - what with the case of static IP config?
 */
void HAL_WLAN_notify_dhcp(bool dhcp);

void HAL_WLAN_notify_can_shutdown();

/**
 * Notification that an open socket has been closed.
 */
void HAL_WLAN_notify_socket_closed(sock_handle_t socket);

/**
 * Select the Wi-Fi antenna.
 */
int wlan_select_antenna(WLanSelectAntenna_TypeDef antenna);

/**
 * Cancel a previous call to any blocking wifi connect method.
 * @param called_from_isr - set to true if this is being called from an ISR.
 */
void wlan_connect_cancel(bool called_from_isr);

#ifdef	__cplusplus
}
#endif

#endif	/* WLAN_H */

