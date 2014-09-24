/* 
 * File:   wlan.h
 * Author: mat
 *
 * Created on 18 September 2014, 17:43
 */

#ifndef WLAN_H
#define	WLAN_H

#include <stdint.h>
#include <stdbool.h>
#include "debug.h"
#include "socket_hal.h"

#ifdef	__cplusplus
extern "C" {
#endif

#if defined(DEBUG_WAN_WD)
#define WAN_WD_DEBUG(x,...) DEBUG(x,__VA_ARGS__)
#else
#define WAN_WD_DEBUG(x,...)
#endif
extern uint32_t wlan_watchdog;
#define ARM_WLAN_WD(x) do { wlan_watchdog = millis()+(x); WAN_WD_DEBUG("WD Set "#x" %d",(x));}while(0)
#define WLAN_WD_TO() (wlan_watchdog && (millis() >= wlan_watchdog))
#define CLR_WLAN_WD() do { wlan_watchdog = 0; WAN_WD_DEBUG("WD Cleared, was %d",wlan_watchdog);;}while(0)
    
#if defined(DEBUG_WIFI)
extern uint32_t lastEvent = 0;

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


        
typedef struct _WLanConfig_t {    
    uint8_t aucIP[4];             // byte 0 is MSB, byte 3 is LSB
    uint8_t aucSubnetMask[4];     // byte 0 is MSB, byte 3 is LSB
    uint8_t aucDefaultGateway[4]; // byte 0 is MSB, byte 3 is LSB
    uint8_t aucDHCPServer[4];     // byte 0 is MSB, byte 3 is LSB
    uint8_t aucDNSServer[4];      // byte 0 is MSB, byte 3 is LSB
    uint8_t uaMacAddr[6];
    uint8_t uaSSID[32];
} WLanConfig;

typedef int wlan_result_t;

wlan_result_t  wlan_connect_init();
wlan_result_t  wlan_connect_finalize();
wlan_result_t  wlan_manual_connect();

bool wlan_reset_credentials_store_required();
wlan_result_t  wlan_reset_credentials_store();

void Set_NetApp_Timeout();
void Clear_NetApp_Dhcp();

wlan_result_t wlan_disconnect_now();

wlan_result_t wlan_activate();
wlan_result_t wlan_deactivate();



/**
 * 
 * @return <0 for a valid signal strength, in db. 
 *         0 for rssi not found (caller could retry)
 *         >0 for an error
 */
int wlan_connected_rssi();

int wlan_clear_credentials();
int wlan_has_credentials();

typedef enum {
    WLAN_SEC_UNSEC,
    WLAN_SEC_WEP,
    WLAN_SEC_WPA,
    WLAN_SEC_WPA2    
} WLanSecurityType;

int wlan_set_credentials(const char* ssid, uint16_t ssid_len, const char* password,
        uint16_t passwordLen, WLanSecurityType security);

/**
 * Initialize smart config mode.
 */
void wlan_smart_config_init();
void wlan_smart_config_cleanup();

void wlan_clear_error_count();
void wlan_set_error_count(uint32_t error_count);


/**
 * Finalize after profiles established.
 */
void wlan_smart_config_finalize();

void wlan_fetch_ipconfig(WLanConfig* config);


/**
 * Called once at startup to initialize the wlan hardware.
 */
void wlan_setup();

void wlan_clear_spark_error_count();
void welan_set_error_count();

void SPARK_WLAN_SmartConfigProcess();

void HAL_WLAN_notify_simple_config_done();
void HAL_WLAN_notify_connected();   
void HAL_WLAN_notify_disconnected();
void HAL_WLAN_notify_dhcp(bool dhcp);
void HAL_WLAN_notify_can_shutdown();
void HAL_WLAN_notify_socket_closed(sock_handle_t socket);


#ifdef	__cplusplus
}
#endif

#endif	/* WLAN_H */

