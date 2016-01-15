/**
 ******************************************************************************
 * @file    wlan_hal.c
 * @author  Matthew McGowan
 * @version V1.0.0
 * @date    27-Sept-2014
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


#include "wlan_hal.h"
#include "delay_hal.h"
#include "core_msg.h"
#include <string.h>
#include "service_debug.h"

uint32_t HAL_NET_SetNetWatchDog(uint32_t timeOutInMS)
{
    return 0;
}

int wlan_clear_credentials()
{
    return 0;
}

int wlan_has_credentials()
{
    return 0;
}

int wlan_connect_init()
{
    INFO("Virtual WLAN connecting");
    return 0;
}

wlan_result_t wlan_activate() {
    INFO("Virtual WLAN on");
    return 0;
}

wlan_result_t wlan_deactivate() {
    INFO("Virtual WLAN off");
    return 0;
}

bool wlan_reset_credentials_store_required()
{
    return false;
}

wlan_result_t wlan_reset_credentials_store()
{
    return 0;
}

/**
 * Do what is needed to finalize the connection.
 * @return
 */
wlan_result_t wlan_connect_finalize()
{
    // enable connection from stored profiles
    HAL_Delay_Milliseconds(1000);
    HAL_NET_notify_connected();
    INFO("Virtual WLAN connected");
    HAL_NET_notify_dhcp(true);
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
    INFO("Virtual WLAN disconnected");
    return 0;
}

wlan_result_t wlan_connected_rssi()
{
    return 0;
}

int wlan_set_credentials(WLanCredentials* c)
{
  return -1;
}

void wlan_smart_config_init() {

}

bool wlan_smart_config_finalize() {
    return false;
}



void wlan_smart_config_cleanup()
{
}


void wlan_setup()
{
    INFO("Virtual WLAN init");
}


void wlan_set_error_count(uint32_t errorCount)
{
}

void wlan_fetch_ipconfig(WLanConfig* config)
{

/*
    memcpy(config->nw.aucIP, "\xC0\x0\x1\x68", 4);
    memcpy(config->nw.aucSubnetMask, "\xFF\xFF\xFF\x0", 4);
    memcpy(config->nw.aucDefaultGateway, "\xC0\x0\x1\x1", 4);
    memcpy(config->nw.aucDHCPServer, "\xC0\x0\x1\x1", 4);
    memcpy(config->nw.aucDNSServer, "\xC0\x0\x1\x1", 4);
    memcpy(config->nw.uaMacAddr, "\x08\x00\x27\x00\x7C\xAC", 6);
    memcpy(config->uaSSID, "WLAN", 5);
 */
}

void SPARK_WLAN_SmartConfigProcess()
{
}

void wlan_connect_cancel(bool called_from_isr)
{
}

int wlan_select_antenna(WLanSelectAntenna_TypeDef antenna)
{
    return 0;
}

bool fetch_or_generate_setup_ssid(void* SSID)
{
    return false;
}



/**
 * Sets the IP source - static or dynamic.
 */
void wlan_set_ipaddress_source(IPAddressSource source, bool persist, void* reserved)
{
}

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
        const HAL_IPAddress* gateway, const HAL_IPAddress* dns1, const HAL_IPAddress* dns2, void* reserved)
{
}

int wlan_scan(wlan_scan_result_t callback, void* cookie)
{
    return -1;
}