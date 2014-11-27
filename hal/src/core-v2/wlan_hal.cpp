/**
 ******************************************************************************
 * @file    wlan_hal.c
 * @author  Matthew McGowan
 * @version V1.0.0
 * @date    03-Nov-2014
 * @brief
 ******************************************************************************
  Copyright (c) 2014 Spark Labs, Inc.  All rights reserved.

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
#include "wlan_hal.h"
#include "hw_config.h"
#include <string.h>
#include <algorithm>

void initialize_dct(platform_dct_wifi_config_t* wifi_config, bool force=false)
{
    if (force || wifi_config->device_configured!=WICED_TRUE) {
        memset(wifi_config, 0, sizeof(*wifi_config));            
    }
    wifi_config->country_code = WICED_COUNTRY_UNITED_STATES;
    wifi_config->device_configured = WICED_TRUE;    
}

uint32_t SPARK_WLAN_SetNetWatchDog(uint32_t timeOutInMS)
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
        wiced_network_down(WICED_STA_INTERFACE);
        // bring network up again?
    }    
    return result;
}

int is_set(const unsigned char* pv, unsigned length) {
    int result = 0;
    while (length-->0) {
        result |= *pv;
    }
    return result;
}

bool is_ap_config_set(const wiced_config_ap_entry_t& ap_entry)
{
    return ap_entry.details.SSID.length>0
      || is_set(ap_entry.details.BSSID.octet, sizeof(ap_entry.details.BSSID));
}

/**
 * Determine if the DCT contains wifi credentials.
 * @return 
 */
int wlan_has_credentials()
{
    int has_credentials = 0;
    platform_dct_wifi_config_t* wifi_config = NULL;
    wiced_result_t result = wiced_dct_read_lock( (void**) &wifi_config, WICED_FALSE, DCT_WIFI_CONFIG_SECTION, 0, sizeof(*wifi_config));
    if (result==WICED_SUCCESS) {
        has_credentials = wifi_config->device_configured!=WICED_TRUE || !is_ap_config_set(wifi_config->stored_ap_list[0]);
    }
    wiced_dct_read_unlock(wifi_config, WICED_FALSE);
    return has_credentials;
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
    // DHCP happens synchronously
    HAL_WLAN_notify_dhcp(!result);
    return result;
}

wlan_result_t wlan_activate() 
{    
    return wiced_wlan_connectivity_init();    
}

wlan_result_t wlan_deactivate() {
    // turn off wifi - power save?    
    wiced_network_down(WICED_STA_INTERFACE);
    return wiced_wlan_connectivity_deinit();
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

wlan_result_t wlan_disconnect_now() 
{
    return wiced_network_down(WICED_STA_INTERFACE);
}

wlan_result_t wlan_connected_rssi(char* ssid) 
{        
    // todo;
    return 0;
}

int find_empty_slot(platform_dct_wifi_config_t* wifi_config) {
    int empty = -1;      // 0 if all full
    for (int i=0; i<CONFIG_AP_LIST_SIZE; i++) {
        if (!is_ap_config_set(wifi_config->stored_ap_list[i])) {
            empty = i;
            break;
        }
    }        
    // if empty == -1, write to last one, and shuffle all from index N to index N-1.
    if (empty<0) {
        empty = CONFIG_AP_LIST_SIZE-1;
        memcpy(wifi_config->stored_ap_list, wifi_config->stored_ap_list+1, sizeof(wifi_config->stored_ap_list[0])*(empty));
    }        
    return empty;
}

wiced_security_t toSecurity(WLanSecurityType sec)
{
    switch (sec) {
        default:
        case WLAN_SEC_UNSEC:
            return WICED_SECURITY_OPEN;
        case WLAN_SEC_WEP:
            return WICED_SECURITY_WEP_PSK;
        case WLAN_SEC_WPA:
            return wiced_security_t(WPA_SECURITY | AES_ENABLED | TKIP_ENABLED);
        case WLAN_SEC_WPA2:
            return wiced_security_t(WPA2_SECURITY | AES_ENABLED | TKIP_ENABLED);
    }
}

int wlan_set_credentials(const char *ssid, uint16_t ssidLen, const char *password, 
    uint16_t passwordLen, WLanSecurityType security)
{
    // find the next available slot, or use the first
    platform_dct_wifi_config_t* wifi_config = NULL;
    wiced_result_t result = wiced_dct_read_lock( (void**) &wifi_config, WICED_TRUE, DCT_WIFI_CONFIG_SECTION, 0, sizeof(*wifi_config));    
    if (!result) {        
        // the storage may not have been initialized, so device_configured will be 0xFF
        initialize_dct(wifi_config);
        int empty = 0; //find_empty_slot(wifi_config);
        
        wiced_config_ap_entry_t& entry = wifi_config->stored_ap_list[empty];
        memset(&entry, 0, sizeof(entry));        
        passwordLen = std::min(passwordLen, uint16_t(64));
        ssidLen = std::min(ssidLen, uint16_t(32));
        memcpy(entry.details.SSID.value, ssid, ssidLen);
        entry.details.SSID.length = ssidLen;
        memcpy(entry.security_key, password, passwordLen);
        entry.security_key_length = passwordLen;
        entry.details.security = toSecurity(security);
        result = wiced_dct_write( (const void*) wifi_config, DCT_WIFI_CONFIG_SECTION, 0, sizeof(*wifi_config) );
        wiced_dct_read_unlock(wifi_config, WICED_TRUE);
    }    
    return result;    
}

void wlan_smart_config_init() {    
    // todo - launch our own soft-ap daemon
    // todo - when the user has completed the soft ap setup process,
    // call HAL_WLAN_notify_simple_config_done() to exit the calling process.
}

void wlan_smart_config_finalize() 
{    
    // dispose the soft AP daemon
}

void wlan_smart_config_cleanup() 
{    
    // todo - mDNS broadcast device IP? Not sure that is needed for soft-ap.
}

void wlan_setup()
{    
    if (!wiced_wlan_connectivity_init())
        wiced_network_register_link_callback(HAL_WLAN_notify_connected, HAL_WLAN_notify_disconnected);
}

wlan_result_t wlan_manual_connect() 
{
    return 0;
}

void wlan_clear_error_count() 
{
}

void wlan_set_error_count(uint32_t errorCount) 
{
}

void setAddress(wiced_ip_address_t* addr, uint8_t* target) {
    memcpy(target, (void*)&addr->ip.v4, 4);
}

void wlan_fetch_ipconfig(WLanConfig* config) 
{
    wiced_ip_address_t addr;
    //wl_join_params_t* join_params;
    wiced_interface_t ifup = WICED_STA_INTERFACE;
    
    memset(config, 0, sizeof(*config));
    if (wiced_network_is_up(ifup)) {
    
        if (wiced_ip_get_ipv4_address(ifup, &addr)==WICED_SUCCESS)
            setAddress(&addr, config->aucIP);

        if (wiced_ip_get_netmask(ifup, &addr)==WICED_SUCCESS)
            setAddress(&addr, config->aucSubnetMask);

        if (wiced_ip_get_gateway_address(ifup, &addr)==WICED_SUCCESS)
            setAddress(&addr, config->aucDefaultGateway);

        wiced_mac_t my_mac_address;
        if (wiced_wifi_get_mac_address( &my_mac_address)==WICED_SUCCESS) 
            memcpy(config->uaMacAddr, &my_mac_address, 6);

        /*join_params = (wl_join_params_t*) wwd_sdpcm_get_ioctl_buffer( &buffer, sizeof(wl_join_params_t) );        
        if (join_params && join_params->ssid.SSID_Len) {
            memcpy(config->uaSSID, join_params->ssid.SSID, sizeof(config->uaSSID));
        }
         * */
    }    
    // todo DNS and DHCP servers
}

void SPARK_WLAN_SmartConfigProcess()
{
}
