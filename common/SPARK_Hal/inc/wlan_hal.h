/* 
 * File:   wlan.h
 * Author: mat
 *
 * Created on 18 September 2014, 17:43
 */

#ifndef WLAN_H
#define	WLAN_H

#include <stdint.h>
#include "debug.h"

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
    
    
    
struct WlanConfig {    
    uint8_t aucIP[4];             // byte 0 is MSB, byte 3 is LSB
    uint8_t aucSubnetMask[4];     // byte 0 is MSB, byte 3 is LSB
    uint8_t aucDefaultGateway[4]; // byte 0 is MSB, byte 3 is LSB
    uint8_t aucDHCPServer[4];     // byte 0 is MSB, byte 3 is LSB
    uint8_t aucDNSServer[4];      // byte 0 is MSB, byte 3 is LSB
    uint8_t uaMacAddr[6];
    uint8_t uaSSID[32];
};

int wlan_connect_init();
int wlan_connect_finalize();

bool wlan_reset_credentials_store_required();
void wlan_reset_credentials_store();

void Set_NetApp_Timeout();
void Clear_NetApp_Dhcp();

void wlan_disconnect_now();

void wlan_activate();
void wlan_deactivate();



/**
 * 
 * @return <0 for a valid signal strength, in db. 
 *         0 for rssi not found (caller could retry)
 *         >0 for an error
 */
int wlan_connected_rssi();

int wlan_clear_credentials();
bool wlan_has_credentials();

enum WLanSecurityType {
    WLAN_SEC_UNSEC,
    WLAN_SEC_WEP,
    WLAN_SEC_WPA,
    WLAN_SEC_WPA2    
};

int wlan_set_credentials(const char* ssid, uint16_t ssid_len, const char* password,
        uint16_t passwordLen, WLanSecurityType security);

/**
 * Initialize smart config mode.
 */
void wlan_smart_config_init();

/**
 * Finalize after profiles established.
 */
void wlan_smart_config_finalize();

extern WlanConfig ip_config;

/**
 * Called once at startup to initialize the wlan hardware.
 */
void wlan_setup();

void wlan_clear_spark_error_count();
void welan_set_error_count();

void SPARK_WLAN_SmartConfigProcess();

#ifdef	__cplusplus
}
#endif

#endif	/* WLAN_H */

