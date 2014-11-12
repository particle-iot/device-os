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
#include "wlan_hal.h"
#include "hw_config.h"
#include <string.h>

uint32_t SPARK_WLAN_SetNetWatchDog(uint32_t timeOutInMS)
{
    return 0;
}


int wlan_clear_credentials() 
{
    // write to DCT credentials
    // clear current IP
    Clear_NetApp_Dhcp();
    return 0;
}

int wlan_has_credentials()
{
    // fetch dct wifi credentials and check for non null
    return 0;
}

/**
 * Enable wlan and connect to a network.
 * @return 
 */
int wlan_connect_init() 
{   
    // bring wlan online but don't connect        
    return 0;
}

wlan_result_t wlan_activate() {
    //wiced_network_up();
    // turn on wlan but do not connect
    return 0;
}

wlan_result_t wlan_deactivate() {
    // turn off wifi - power save?
    //wiced_network_down();
    return 0;
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


/**
 * Do what is needed to finalize the connection. 
 * @return 
 */
wlan_result_t wlan_connect_finalize() 
{
    // enable connection from stored profiles
    //wiced_network_up();
    return 0;
}


void Set_NetApp_Timeout(void)
{
}

void Clear_NetApp_Dhcp(void)
{
}

wlan_result_t wlan_disconnect_now() 
{
    //wiced_network_down();
    return 0;
}

wlan_result_t wlan_connected_rssi(char* ssid) 
{        
    return -1;
}



int wlan_set_credentials(const char *ssid, uint16_t ssidLen, const char *password, 
    uint16_t passwordLen, WLanSecurityType security)
{
    return 0;
}

void wlan_smart_config_init() {
    
}

void wlan_smart_config_finalize() {    
}



void wlan_smart_config_cleanup() 
{
}


void wlan_setup()
{    
    //wiced_wlan_connectivity_init();
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