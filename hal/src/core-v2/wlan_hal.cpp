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

#if USE_WICED_SDK==1

#include "wiced.h"
#include "wiced_easy_setup.h"
#include "wlan_hal.h"
#include "hw_config.h"
#include <string.h>
#include <algorithm>

uint32_t SPARK_WLAN_SetNetWatchDog(uint32_t timeOutInMS)
{
    return 0;
}


int wlan_clear_credentials() 
{
    // write to DCT credentials
    // clear current IP
    platform_dct_wifi_config_t* wifi_config = NULL;
    wiced_result_t result = wiced_dct_read_lock( (void**) &wifi_config, WICED_TRUE, DCT_WIFI_CONFIG_SECTION, 0, sizeof(*wifi_config));
    if (!result) {
        //int is_up = wiced_network_is_up(WICED_STA_INTERFACE);
        wifi_config->device_configured = WICED_FALSE;
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

int wlan_has_credentials()
{
    int has_credentials = 0;
    platform_dct_wifi_config_t* wifi_config = NULL;
    wiced_result_t result = wiced_dct_read_lock( (void**) &wifi_config, WICED_FALSE, DCT_WIFI_CONFIG_SECTION, 0, sizeof(*wifi_config));
    if (result==WICED_SUCCESS) {
        has_credentials = wifi_config->device_configured && is_ap_config_set(wifi_config->stored_ap_list[0]);                
    }
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
    return wiced_network_up(WICED_STA_INTERFACE, WICED_USE_EXTERNAL_DHCP_SERVER, NULL);    
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
    return -1;
}

int wlan_set_credentials(const char *ssid, uint16_t ssidLen, const char *password, 
    uint16_t passwordLen, WLanSecurityType security)
{
    // find the next available slot, or use the first
    platform_dct_wifi_config_t* wifi_config = NULL;
    wiced_result_t result = wiced_dct_read_lock( (void**) &wifi_config, WICED_TRUE, DCT_WIFI_CONFIG_SECTION, 0, sizeof(*wifi_config));    
    if (!result) {        
        wifi_config->device_configured = WICED_TRUE;
        int empty = 0;      // 0 if all full
        for (int i=0; i<CONFIG_AP_LIST_SIZE; i++) {
            if (!is_ap_config_set(wifi_config->stored_ap_list[i])) {
                empty = i;
                break;
            }
        }        
        wiced_config_ap_entry_t& entry = wifi_config->stored_ap_list[empty];
        memset(&entry, 0, sizeof(entry));
        passwordLen = std::min(passwordLen, uint16_t(64));
        ssidLen = std::min(ssidLen, uint16_t(32));
        memcpy(entry.details.SSID.value, ssid, ssidLen);
        entry.details.SSID.length = ssidLen;
        memcpy(entry.security_key, password, passwordLen);
        entry.security_key_length = passwordLen;
        result = wiced_dct_write( (const void*) wifi_config, DCT_WIFI_CONFIG_SECTION, 0, sizeof(*wifi_config) );
        wiced_dct_read_unlock(wifi_config, WICED_TRUE);
    }    
    return result;    
}

softap_setup_t* soft_ap_setup = NULL;
configuration_entry_t configuration[] = { {0,0,0,CONFIG_STRING_DATA} };

void wlan_smart_config_init() {
#if 0    
    if (soft_ap_setup==NULL) {        
        soft_ap_setup = new softap_setup_t();
        soft_ap_setup->force = WICED_TRUE;        
        soft_ap_setup->result = wiced_easy_setup_start_softap( soft_ap_setup );
    }
#endif
}

void wlan_smart_config_finalize() 
{    
#if 0
    if (soft_ap_setup!=NULL && !soft_ap_setup->result) {
        wiced_easy_setup_stop_softap(soft_ap_setup);
    }
    delete soft_ap_setup;
    soft_ap_setup = NULL;
#endif
}

void wlan_smart_config_cleanup() 
{    
}


void wlan_setup()
{        
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
    memcpy(target, addr, 4);
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

#else

#include "../template/wlan_hal.c"

#endif