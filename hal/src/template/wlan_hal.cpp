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

uint32_t HAL_NET_SetNetWatchDog(uint32_t timeOutInMS)
{
    return 0;
}

int wlan_clear_credentials()
{
    return 1;
}

int wlan_has_credentials()
{
    return 1;
}

int wlan_connect_init()
{
    return 0;
}

wlan_result_t wlan_activate()
{
    return 0;
}

wlan_result_t wlan_deactivate()
{
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
    return 0;
}

void Set_NetApp_Timeout(void)
{
}

wlan_result_t wlan_disconnect_now()
{
    return 0;
}

wlan_result_t wlan_connected_rssi(char* ssid)
{
    return 0;
}

int wlan_connected_info(void* reserved, wlan_connected_info_t* inf, void* reserved1)
{
    return -1;
}

int wlan_set_credentials(WLanCredentials* c)
{
    return -1;
}

void wlan_smart_config_init()
{
}

bool wlan_smart_config_finalize()
{
    return false;
}

void wlan_smart_config_cleanup()
{
}

void wlan_setup()
{
}

void wlan_set_error_count(uint32_t errorCount)
{
}

int wlan_fetch_ipconfig(WLanConfig* config)
{
    return -1;
}

void SPARK_WLAN_SmartConfigProcess()
{
}

void wlan_connect_cancel(bool called_from_isr)
{
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

IPAddressSource wlan_get_ipaddress_source(void* reserved)
{
    return DYNAMIC_IP;
}

int wlan_get_ipaddress(IPConfig* conf, void* reserved)
{
    return -1;
}

int wlan_scan(wlan_scan_result_t callback, void* cookie)
{
    return -1;
}

int wlan_restart(void* reserved)
{
    return -1;
}

int wlan_get_hostname(char* buf, size_t len, void* reserved)
{
    // Unsupported
    if (buf) {
        buf[0] = '\0';
    }
    return -1;
}

int wlan_set_hostname(const char* hostname, void* reserved)
{
    // Unsupported
    return -1;
}
