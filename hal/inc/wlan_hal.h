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
#include "timer_hal.h"
#include "net_hal.h"


#ifdef	__cplusplus
extern "C" {

#include <string.h> // for memset
#endif

//#define DEBUG_WIFI    // Define to show all the flags in debug output
//#define DEBUG_WAN_WD  // Define to show all SW WD activity in debug output

#if defined(DEBUG_WIFI)
extern uint32_t lastEvent;

#define SET_LAST_EVENT(x) do {lastEvent = (x);} while(0)
#define GET_LAST_EVENT(x) do { x = lastEvent; lastEvent = 0;} while(0)
#define DUMP_STATE() do { \
    DEBUG("\r\nSPARK_WLAN_RESET=%d\r\nSPARK_WLAN_SLEEP=%d\r\nSPARK_WLAN_STARTED=%d\r\nSPARK_CLOUD_CONNECT=%d", \
           SPARK_WLAN_RESET,SPARK_WLAN_SLEEP,SPARK_WLAN_STARTED,SPARK_CLOUD_CONNECT); \
    DEBUG("\r\nSPARK_CLOUD_SOCKETED=%d\r\nSPARK_CLOUD_CONNECTED=%d\r\nSPARK_FLASH_UPDATE=%d\r\n", \
           SPARK_CLOUD_SOCKETED,SPARK_CLOUD_CONNECTED,SPARK_FLASH_UPDATE); \
 } while(0)

#define ON_EVENT_DELTA()  do { if (lastEvent != 0) { uint32_t l; GET_LAST_EVENT(l); DEBUG("\r\nAsyncEvent 0x%04x", l); DUMP_STATE();}} while(0)
#else
#define SET_LAST_EVENT(x)
#define GET_LAST_EVENT(x)
#define DUMP_STATE()
#define ON_EVENT_DELTA()
#endif

/**
 * Describes the antenna selection to be used with the primary radio for devices with multiple antennae.
 */
typedef enum
{
  ANT_INTERNAL = 0,
  ANT_EXTERNAL = 1,
  ANT_AUTO = 3,
  // Error
  ANT_NONE = 0xff
} WLanSelectAntenna_TypeDef;

typedef int wlan_result_t;

typedef struct __attribute__((__packed__))  _WLanConfig_t {
    uint16_t size;
    NetworkConfig nw;
    uint8_t uaSSID[33];
    uint8_t BSSID[6];			// since V2
} WLanConfig;

#define WLanConfig_Size_V1   (sizeof(NetworkConfig)+2+33)
#define WLanConfig_Size_V2   (WLanConfig_Size_V1+6)

PARTICLE_STATIC_ASSERT(WLanConfigSize, sizeof(WLanConfig)==WLanConfig_Size_V2);



/**
 * Begin establishing the wireless connection. This is provided as a convenience for platforms to do any steps required
 * prior to connected to a configured AP.  wlan_connect_finalize() will be called to finalize the connection.
 *
 */
wlan_result_t  wlan_connect_init();

/**
 * Finalize the connection by connecting to an AP previously stored with wlan_set_credentials().
 * The notification handlers HAL_NET_notify_connected, HAL_NET_notify_disconnected must be set up to receive
 * asynchronous events from the wifi hardware when a connection to an AP is established and lost respectively.
 */
wlan_result_t  wlan_connect_finalize();

/**
 * Determines if a request has been made to reset the wifi credentials. On platforms that use the Particle bootloader,
 * this is done by issuing a "full reset" command.
 */
bool wlan_reset_credentials_store_required();

/**
 * Removes all stored AP credentials by calling wlan_clear_credentials() and clears the wlan_reset_credentials_store_required() flag.
 */
wlan_result_t  wlan_reset_credentials_store();

/**
 * This function exists primarily as a throwback to early system firmware.
 * It is called between wlan_connect_init() and wlan_connect_finalize()
 * and gives the platform an opportunity to set any network timeouts required.
 * This will most likely be phased out since it is subsumed by the wlan_connect_init() function.
 */
void Set_NetApp_Timeout();

/**
 * Brings down the primary networking interface, while maintaining power to the WiFi hardware.
 * All open sockets are closed and when the operation is complete, the HAL_NET_notify_disconnected() function is called,
 * either directly, as a result of bringing down the connection.
 */
wlan_result_t wlan_disconnect_now();

/**
 * Enable the wifi hardware without connecting.
 */
wlan_result_t wlan_activate();

/**
 * Disable wifi. The wifi hardware should be powered down to save power.
 * This may be called when the AP is still connected, so implementations should call wlan_disconnect_now() to take down
 * the network first, before shutting off the wifi hardware.
 */
wlan_result_t wlan_deactivate();



/**
 * Determines the strength of the signal of the connected AP.
 *
 * @return <0 for a valid signal strength, in db.
 *         0 for rssi not found (caller could retry)
 *         >0 for an error
 */
int wlan_connected_rssi();

typedef struct {
    uint16_t size;
    uint16_t version;

    int32_t rssi;
    // In % mapped to [0, 65535]
    int32_t strength;

    int32_t snr;
    int32_t noise;
    // In % mapped to [0, 65535]
    int32_t quality;
} wlan_connected_info_t;

int wlan_connected_info(void* reserved, wlan_connected_info_t* inf, void* reserved1);

int wlan_clear_credentials();
int wlan_has_credentials();

// Provide compatibility with the original cc3000 headers.
#ifdef WLAN_SEC_UNSEC
#undef WLAN_SEC_UNSEC
#undef WLAN_SEC_WEP
#undef WLAN_SEC_WPA
#undef WLAN_SEC_WPA2
#undef WLAN_SEC_WPA_ENTERPRISE
#undef WLAN_SEC_WPA2_ENTERPRISE
#endif
typedef enum {
    WLAN_SEC_UNSEC = 0,
    WLAN_SEC_WEP,
    WLAN_SEC_WPA,
    WLAN_SEC_WPA2,
    WLAN_SEC_WPA_ENTERPRISE,
    WLAN_SEC_WPA2_ENTERPRISE,
    WLAN_SEC_NOT_SET = 0xFF
} WLanSecurityType;


typedef enum {
    WLAN_CIPHER_NOT_SET = 0,
    WLAN_CIPHER_AES = 1,
    WLAN_CIPHER_TKIP = 2,
    WLAN_CIPHER_AES_TKIP = 3   // OR of AES and TKIP
} WLanSecurityCipher;

typedef enum {
    WLAN_EAP_TYPE_NONE         = 0,
    WLAN_EAP_TYPE_IDENTITY     = 1   /* RFC 3748 */,
    WLAN_EAP_TYPE_NOTIFICATION = 2   /* RFC 3748 */,
    WLAN_EAP_TYPE_NAK          = 3   /* Response only, RFC 3748 */,
    WLAN_EAP_TYPE_MD5          = 4,  /* RFC 3748 */
    WLAN_EAP_TYPE_OTP          = 5   /* RFC 3748 */,
    WLAN_EAP_TYPE_GTC          = 6,  /* RFC 3748 */
    WLAN_EAP_TYPE_TLS          = 13  /* RFC 2716 */,
    WLAN_EAP_TYPE_LEAP         = 17  /* Cisco proprietary */,
    WLAN_EAP_TYPE_SIM          = 18  /* draft-haverinen-pppext-eap-sim-12.txt */,
    WLAN_EAP_TYPE_TTLS         = 21  /* draft-ietf-pppext-eap-ttls-02.txt */,
    WLAN_EAP_TYPE_AKA          = 23  /* draft-arkko-pppext-eap-aka-12.txt */,
    WLAN_EAP_TYPE_PEAP         = 25  /* draft-josefsson-pppext-eap-tls-eap-06.txt */,
    WLAN_EAP_TYPE_MSCHAPV2     = 26  /* draft-kamath-pppext-eap-mschapv2-00.txt */,
    WLAN_EAP_TYPE_TLV          = 33  /* draft-josefsson-pppext-eap-tls-eap-07.txt */,
    WLAN_EAP_TYPE_FAST         = 43  /* draft-cam-winget-eap-fast-00.txt */,
    WLAN_EAP_TYPE_PAX          = 46, /* draft-clancy-eap-pax-04.txt */
    WLAN_EAP_TYPE_EXPANDED_NAK = 253 /* RFC 3748 */,
    WLAN_EAP_TYPE_WPS          = 254 /* Wireless Simple Config */,
    WLAN_EAP_TYPE_PSK          = 255 /* EXPERIMENTAL - type not yet allocated draft-bersani-eap-psk-09 */
} WLanEapType;

typedef struct {
    unsigned size;           // the size of this structure. allows older clients to work with newer HAL.
    const char* ssid;
    unsigned ssid_len;
    const char* password;
    unsigned password_len;
    WLanSecurityType security;
    WLanSecurityCipher cipher;
    unsigned channel;
    unsigned flags;
    // v2
    uint8_t version;
    // Additional parameters used with WLAN_SEC_WPA_ENTERPRISE or WLAN_SEC_WPA2_ENTERPRISE
    // EAP type
    WLanEapType eap_type;
    // EAP inner identity
    const char* inner_identity;
    uint16_t inner_identity_len;
    // EAP outer identity
    const char* outer_identity;
    uint16_t outer_identity_len;
    // Private key (PEM or DER)
    const uint8_t* private_key;
    uint16_t private_key_len;
    // Client certificate (PEM or DER)
    const uint8_t* client_certificate;
    uint16_t client_certificate_len;
    // CA certificate (PEM or DER)
    const uint8_t* ca_certificate;
    uint16_t ca_certificate_len;
} WLanCredentials;

#define WLAN_CREDENTIALS_VERSION_3        (3)
#define WLAN_CREDENTIALS_CURRENT_VERSION  (WLAN_CREDENTIALS_VERSION_3)

#define WLAN_SET_CREDENTIALS_FLAGS_DRY_RUN  (1<<0)

#define WLAN_SET_CREDENTIALS_UNKNOWN_SECURITY_TYPE (-1)
#define WLAN_SET_CREDENTIALS_CIPHER_REQUIRED (-2)
#define WLAN_INVALID_SSID_LENGTH (-3)
#define WLAN_INVALID_KEY_LENGTH (-4)

/**
 * Adds credentials for an AP that this device will connect to. The credentials are stored persistently.
 * If the storage for WiFi credentials is full, these credentials
 * should take precedence can replace previously set credentials, typically the oldest.
 * @param credentials	The credentials to store persistently.
 * @return 0 on success.
 *
 * Note that if the device is already connected to an AP, that connection remains, even if the credentials for that AP were removed.
 */
int wlan_set_credentials(WLanCredentials* credentials);

/**
 * Initialize smart config/SoftAP mode. This is used to put the device in setup mode so that it can pair with the mobile
 * app to be provided with credentials.
 */
void wlan_smart_config_init();

/**
 * This is invoked after setup is done.
 */
void wlan_smart_config_cleanup();

/**
 * This is obsolete and is not used.
 */
void wlan_set_error_count(uint32_t error_count);

/**
 * Finalize and exit setup mode after profiles established.
 * @return true the wifi profiles were changed
 */
bool wlan_smart_config_finalize();

/**
 * Retrieve IP address info. Available after HAL_WLAN_notify_dhcp() has been callted.
 */
int wlan_fetch_ipconfig(WLanConfig* config);

/**
 * Called once at startup to initialize the wlan hardware.
 */
void wlan_setup();

/**
 *
 */
void SPARK_WLAN_SmartConfigProcess();

/**
 *
 */
void HAL_WLAN_notify_simple_config_done();


/**
 * Select the Wi-Fi antenna.
 */
int wlan_select_antenna(WLanSelectAntenna_TypeDef antenna);

WLanSelectAntenna_TypeDef wlan_get_antenna(void* reserved);

/**
 * Cancel a previous call to any blocking wifi connect method.
 * @param called_from_isr - set to true if this is being called from an ISR.
 */
void wlan_connect_cancel(bool called_from_isr);

typedef enum {
    DYNAMIC_IP,
    STATIC_IP
} IPAddressSource;

/**
 * Sets the IP source - static or dynamic.
 */
void wlan_set_ipaddress_source(IPAddressSource source, bool persist, void* reserved);

IPAddressSource wlan_get_ipaddress_source(void* reserved);

/**
 * Sets the IP Addresses to use when the device is in static IP mode.
 * @param device
 * @param netmask
 * @param gateway
 * @param dns1
 * @param dns2
 * @param reserved
 */
void wlan_set_ipaddress(const HAL_IPAddress* device, const HAL_IPAddress* netmask,
        const HAL_IPAddress* gateway, const HAL_IPAddress* dns1, const HAL_IPAddress* dns2, void* reserved);


int wlan_get_ipaddress(IPConfig* conf, void* reserved);


typedef struct WiFiAccessPoint {
   size_t size;
   char ssid[33];
   uint8_t ssidLength;
   uint8_t bssid[6];
   WLanSecurityType security;
   WLanSecurityCipher cipher;
   uint8_t channel;
   int maxDataRate;   // the mdr in bits/s
   int rssi;        // when scanning

#ifdef __cplusplus

   WiFiAccessPoint()
   {
       memset(this, 0, sizeof(*this));
       size = sizeof(*this);
   }
#endif
} WiFiAccessPoint;

typedef void (*wlan_scan_result_t)(WiFiAccessPoint* ap, void* cookie);


/**
 * @param callback  The callback that receives each scanned AP
 * @param cookie    An opaque handle that is passed to the callback.
 * @return negative on error.
 */
int wlan_scan(wlan_scan_result_t callback, void* cookie);

/**
 * Lists all WLAN credentials currently stored on the device
 * @param callback  The callback that receives each stored AP
 * @param callback_data An opaque handle that is passed to the callback.
 * @return count of stored credentials, negative on error.
 */
int wlan_get_credentials(wlan_scan_result_t callback, void* callback_data);

/**
 * @return true if wi-fi powersave clock is enabled, false if disabled.
 */
bool isWiFiPowersaveClockDisabled(void);

/**
 * Disables and then enables WLAN connectivity
 */
int wlan_restart(void* reserved);

int wlan_set_hostname(const char* hostname, void* reserved);
int wlan_get_hostname(char* buf, size_t buf_size, void* reserved);

#ifdef	__cplusplus
}
#endif

#endif	/* WLAN_H */

