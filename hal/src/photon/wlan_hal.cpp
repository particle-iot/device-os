/**
 ******************************************************************************
 * @file    wlan_hal.c
 * @author  Matthew McGowan
 * @version V1.0.0
 * @date    03-Nov-2014
 * @brief
 ******************************************************************************
  Copyright (c) 2014-2015 Particle Industries, Inc.  All rights reserved.

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

#include "wiced.h"
#include "wiced_easy_setup.h"
#include "delay_hal.h"
#include "wlan_hal.h"
#include "hw_config.h"
#include "softap.h"
#include <string.h>
#include <algorithm>
#include "wlan_internal.h"
#include "socket_internal.h"
#include "wwd_sdpcm.h"
#include "delay_hal.h"
#include "wlan_scan.h"

bool initialize_dct(platform_dct_wifi_config_t* wifi_config, bool force=false)
{
    bool changed = false;
    wiced_country_code_t country = WICED_COUNTRY_JAPAN;
    if (force || wifi_config->device_configured!=WICED_TRUE || wifi_config->country_code!=country) {
        memset(wifi_config, 0, sizeof(*wifi_config));            
        wifi_config->country_code = country;
        wifi_config->device_configured = WICED_TRUE;    
        changed = true;
    }
    return changed;
}

/**
 * Initializes the DCT area if required.
 * @return 
 */
wiced_result_t wlan_initialize_dct()
{
    // find the next available slot, or use the first
    platform_dct_wifi_config_t* wifi_config = NULL;
    wiced_result_t result = wiced_dct_read_lock( (void**) &wifi_config, WICED_TRUE, DCT_WIFI_CONFIG_SECTION, 0, sizeof(*wifi_config));
    if (result==WICED_SUCCESS) {
        // the storage may not have been initialized, so device_configured will be 0xFF
        if (initialize_dct(wifi_config))
            result = wiced_dct_write( (const void*) wifi_config, DCT_WIFI_CONFIG_SECTION, 0, sizeof(*wifi_config) );
        wiced_dct_read_unlock(wifi_config, WICED_TRUE);
    }    
    return result;    
}


uint32_t HAL_WLAN_SetNetWatchDog(uint32_t timeOutInMS)
{
    wiced_watchdog_kick();
    return 0;
}

/**
 * Clears the WLAN credentials by erasing the DCT data and taking down the STA
 * network interface.
 * @return 
 */
int wlan_clear_credentials() 
{
    // write to DCT credentials
    // clear current IP
    platform_dct_wifi_config_t* wifi_config = NULL;
    wiced_result_t result = wiced_dct_read_lock( (void**) &wifi_config, WICED_TRUE, DCT_WIFI_CONFIG_SECTION, 0, sizeof(*wifi_config));
    if (!result) {        
        memset(wifi_config->stored_ap_list, 0, sizeof(wifi_config->stored_ap_list));
        result = wiced_dct_write( (const void*) wifi_config, DCT_WIFI_CONFIG_SECTION, 0, sizeof(*wifi_config) );
        wiced_dct_read_unlock(wifi_config, WICED_TRUE);
    }    
    return result;
}

int is_set(const unsigned char* pv, unsigned length) {
    int result = 0;
    int result2 = 0xFF;
    while (length-->0) {
        result |= *pv;
        result2 &= *pv;
    }
    return result && result2!=0xFF;
}

bool is_ap_config_set(const wiced_config_ap_entry_t& ap_entry)
{
    bool set = ((ap_entry.details.SSID.length>0 && ap_entry.details.SSID.length<33))
      || is_set(ap_entry.details.BSSID.octet, sizeof(ap_entry.details.BSSID));
    return set;
}

/**
 * Determine if the DCT contains wifi credentials.
 * @return 0 if the device has credentials. 1 otherwise. (yes, it's backwards! The intent is that negative values indicate some kind of error.)
 */
int wlan_has_credentials()
{
    int has_credentials = 0;
    platform_dct_wifi_config_t* wifi_config = NULL;
    wiced_result_t result = wiced_dct_read_lock( (void**) &wifi_config, WICED_FALSE, DCT_WIFI_CONFIG_SECTION, 0, sizeof(*wifi_config));
    if (result==WICED_SUCCESS) {
        has_credentials = wifi_config->device_configured==WICED_TRUE && is_ap_config_set(wifi_config->stored_ap_list[0]);
    }
    wiced_dct_read_unlock(wifi_config, WICED_FALSE);
    return !has_credentials;
}

/**
 * Enable wlan and connect to a network.
 * @return 
 */
int wlan_connect_init() 
{   
    return wiced_wlan_connectivity_init();    
}

/**
 * Do what is needed to finalize the connection. 
 * @return 
 */
wlan_result_t wlan_connect_finalize() 
{
    // enable connection from stored profiles
    wlan_result_t result = wiced_network_up(WICED_STA_INTERFACE, WICED_USE_EXTERNAL_DHCP_SERVER, NULL);        
    if (!result)
        HAL_WLAN_notify_connected();
    // DHCP happens synchronously
    HAL_WLAN_notify_dhcp(!result);
    return result;
}

wlan_result_t wlan_activate() 
{    
    return wiced_network_resume();
}

wlan_result_t wlan_deactivate() {
    wlan_disconnect_now();
    return wiced_network_suspend();
}

wlan_result_t wlan_disconnect_now() 
{
    socket_close_all();    
    wiced_result_t result = wiced_network_down(WICED_STA_INTERFACE);
    HAL_WLAN_notify_disconnected();    
    return result;
}


bool wlan_reset_credentials_store_required() 
{
    return system_flags.NVMEM_SPARK_Reset_SysFlag == 0x0001;
}

wlan_result_t wlan_reset_credentials_store()
{
    wlan_clear_credentials();
    system_flags.NVMEM_SPARK_Reset_SysFlag = 0x0000;
    Save_SystemFlags();    
    return 0;
}

void Set_NetApp_Timeout(void)
{
}

int wlan_connected_rssi() 
{        
    int32_t rssi = 0;
    if (wwd_wifi_get_rssi( &rssi ))
        rssi = 0;
    return rssi;
}

struct SnifferInfo
{
    const char* ssid;
    unsigned ssid_len;    
    wiced_security_t security;
    int16_t rssi;
    wiced_semaphore_t complete;
    scan_ap_callback callback;
    void* callback_data;
};

/*
 * Callback function to handle scan results
 */
wiced_result_t sniffer( wiced_scan_handler_result_t* malloced_scan_result )
{
    malloc_transfer_to_curr_thread( malloced_scan_result );
    
    SnifferInfo* info = (SnifferInfo*)malloced_scan_result->user_data;
    if ( malloced_scan_result->status == WICED_SCAN_INCOMPLETE )
    {
        wiced_scan_result_t* record = &malloced_scan_result->ap_details;
        if (!info->callback) {
            if (record->SSID.length==info->ssid_len && !memcmp(record->SSID.value, info->ssid, info->ssid_len)) {
                info->security = record->security;
                info->rssi = record->signal_strength;
            }
        }
        else {
            info->callback(info->callback_data, record->SSID.value, record->SSID.length, record->signal_strength);
        }
    }
    else {
        wiced_rtos_set_semaphore(&info->complete);
    }
    free( malloced_scan_result );
    return WICED_SUCCESS;
}

wiced_result_t sniff_security(SnifferInfo* info) {
    
    wiced_rtos_init_semaphore(&info->complete);
    wiced_result_t result = wiced_wifi_scan_networks(sniffer, info);
    wiced_rtos_get_semaphore(&info->complete, 30000);
    wiced_rtos_deinit_semaphore(&info->complete);
    if (!info->rssi)
        result = WICED_NOT_FOUND;
    return result;
}


void wlan_scan_aps(scan_ap_callback callback, void *data) {    
    SnifferInfo info;
    info.callback = callback;
    info.callback_data = data;
    sniff_security(&info);
}

wiced_security_t toSecurity(const char* ssid, unsigned ssid_len, WLanSecurityType sec, WLanSecurityCipher cipher)
{
    unsigned result = 0;
    switch (sec) {
        case WLAN_SEC_UNSEC:
            result = WICED_SECURITY_OPEN;
            break;
        case WLAN_SEC_WEP:
            result = WICED_SECURITY_WEP_PSK;
            break;
        case WLAN_SEC_WPA:
            result = WPA_SECURITY;
            break;
        case WLAN_SEC_WPA2:
            result = WPA2_SECURITY;
            break;
    }        

    if (cipher & WLAN_CIPHER_AES)
        result |= AES_ENABLED;
    if (cipher & WLAN_CIPHER_TKIP)
        result |= TKIP_ENABLED;

    if (sec==WLAN_SEC_NOT_SET ||    // security not set, or WPA/WPA2 and cipher not set
            ((result & (WPA_SECURITY | WPA2_SECURITY) && (cipher==WLAN_CIPHER_NOT_SET)))) {
        SnifferInfo info;
        info.ssid = ssid;
        info.callback = NULL;
        info.ssid_len = ssid_len;
        if (!sniff_security(&info)) {
            result = info.security;
        }
    }
    return wiced_security_t(result);
}

static bool wifi_creds_changed;
wiced_result_t add_wiced_wifi_credentials(const char *ssid, uint16_t ssidLen, const char *password, 
    uint16_t passwordLen, wiced_security_t security, unsigned channel)
{    
    platform_dct_wifi_config_t* wifi_config = NULL;
    wiced_result_t result = wiced_dct_read_lock( (void**) &wifi_config, WICED_TRUE, DCT_WIFI_CONFIG_SECTION, 0, sizeof(*wifi_config));    
    if (!result) {        
        // the storage may not have been initialized, so device_configured will be 0xFF
        initialize_dct(wifi_config);
        
        // shuffle all slots along
        memcpy(wifi_config->stored_ap_list+1, wifi_config->stored_ap_list, sizeof(wiced_config_ap_entry_t)*(CONFIG_AP_LIST_SIZE-1));
        
        const int empty = 0; 
        wiced_config_ap_entry_t& entry = wifi_config->stored_ap_list[empty];
        memset(&entry, 0, sizeof(entry));        
        passwordLen = std::min(passwordLen, uint16_t(64));
        ssidLen = std::min(ssidLen, uint16_t(32));
        memcpy(entry.details.SSID.value, ssid, ssidLen);
        entry.details.SSID.length = ssidLen;
        if (security==WICED_SECURITY_WEP_PSK && passwordLen>1 && password[0]>4) {
            // convert from hex to binary
            entry.security_key_length = hex_decode((uint8_t*)entry.security_key, sizeof(entry.security_key), password);
        }
        else {
            memcpy(entry.security_key, password, passwordLen);        
            entry.security_key_length = passwordLen;
        }
        entry.details.security = security;
        entry.details.channel = channel;        
        result = wiced_dct_write( (const void*) wifi_config, DCT_WIFI_CONFIG_SECTION, 0, sizeof(*wifi_config) );
        if (!result)
            wifi_creds_changed = true;
        wiced_dct_read_unlock(wifi_config, WICED_TRUE);        
    }    
    return result;
}
    
int wlan_set_credentials_internal(const char *ssid, uint16_t ssidLen, const char *password, 
    uint16_t passwordLen, WLanSecurityType security, WLanSecurityCipher cipher, unsigned channel)
{
    wiced_result_t result = WICED_ERROR;
    if (ssidLen>0 && ssid) {
        wiced_security_t wiced_security = toSecurity(ssid, ssidLen, security, cipher);
        result = add_wiced_wifi_credentials(ssid, ssidLen, password, passwordLen, wiced_security, channel);        
    }
    return result;    
}

int wlan_set_credentials(WLanCredentials* c)
{
    return wlan_set_credentials_internal(c->ssid, c->ssid_len, c->password, c->password_len, c->security, c->cipher, c->channel);
}

softap_handle current_softap_handle;

void wlan_smart_config_init() {    
    
    wifi_creds_changed = false;
    if (!current_softap_handle) {
        softap_config config;
        config.softap_complete = HAL_WLAN_notify_simple_config_done;
        wlan_disconnect_now();
        current_softap_handle = softap_start(&config);        
    }    
}

bool wlan_smart_config_finalize() 
{    
    if (current_softap_handle) {
        softap_stop(current_softap_handle);
        wlan_disconnect_now();  // force a full refresh
        HAL_Delay_Milliseconds(5);
        wlan_activate();
        current_softap_handle = NULL;
    }
    // if wifi creds changed, then indicate the system should enter listening mode on failed connect
    return wifi_creds_changed;
}

void wlan_smart_config_cleanup() 
{    
    // todo - mDNS broadcast device IP? Not sure that is needed for soft-ap.
}

void wlan_setup()
{    
    if (!wiced_wlan_connectivity_init()) {
        wiced_network_register_link_callback(HAL_WLAN_notify_connected, HAL_WLAN_notify_disconnected, WICED_STA_INTERFACE);
        wiced_network_suspend();
}
}

void wlan_set_error_count(uint32_t errorCount) 
{
}

inline void setAddress(wiced_ip_address_t* addr, HAL_IPAddress& target) {
    target.ipv4 = GET_IPV4_ADDRESS(*addr);    
}

void wlan_fetch_ipconfig(WLanConfig* config) 
{
    wiced_ip_address_t addr;
    wiced_interface_t ifup = WICED_STA_INTERFACE;
    
    memset(config, 0, sizeof(*config));
    if (wiced_network_is_up(ifup)) {
    
        if (wiced_ip_get_ipv4_address(ifup, &addr)==WICED_SUCCESS)
            setAddress(&addr, config->nw.aucIP);

        if (wiced_ip_get_netmask(ifup, &addr)==WICED_SUCCESS)
            setAddress(&addr, config->nw.aucSubnetMask);

        if (wiced_ip_get_gateway_address(ifup, &addr)==WICED_SUCCESS)
            setAddress(&addr, config->nw.aucDefaultGateway);
    }
    
    wiced_mac_t my_mac_address;
    if (wiced_wifi_get_mac_address( &my_mac_address)==WICED_SUCCESS) 
        memcpy(config->nw.uaMacAddr, &my_mac_address, 6);

    wl_bss_info_t ap_info;
    wiced_security_t sec;

    if ( wwd_wifi_get_ap_info( &ap_info, &sec ) == WWD_SUCCESS )
    {
        uint8_t len = std::min(ap_info.SSID_len, uint8_t(32));
        memcpy(config->uaSSID, ap_info.SSID, len);
        config->uaSSID[len] = 0;
    }   
    // todo DNS and DHCP servers
}

void SPARK_WLAN_SmartConfigProcess()
{
}

/** Select the Wi-Fi antenna
 * WICED_ANTENNA_1    = 0, selects Chip Antenna
 * WICED_ANTENNA_2    = 1, selects u.FL Antenna
 * WICED_ANTENNA_AUTO = 3, enables auto antenna selection ie. automatic diversity
 *
 * @param antenna: The antenna configuration to use
 *
 * @return   0 : if the antenna selection was successfully set
 *          -1 : if the antenna selection was not set
 *          
 */
int wlan_select_antenna(WLanSelectAntenna_TypeDef antenna) {
    
    wwd_result_t result;
    switch(antenna) {
        case ANT_INTERNAL: result = wwd_wifi_select_antenna(WICED_ANTENNA_1); break;
        case ANT_EXTERNAL: result = wwd_wifi_select_antenna(WICED_ANTENNA_2); break;
        case ANT_AUTO: result = wwd_wifi_select_antenna(WICED_ANTENNA_AUTO); break;
        default: result = WWD_DOES_NOT_EXIST; break;
    }
    if (result == WWD_SUCCESS)
        return 0;
    else
        return -1;
}


void wlan_connect_cancel(bool called_from_isr)
{
    wwd_wifi_join_cancel(called_from_isr ? WICED_TRUE : WICED_FALSE);
}
