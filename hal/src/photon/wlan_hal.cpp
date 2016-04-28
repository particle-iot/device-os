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
#include "wiced_internal_api.h"
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
#include "dct_hal.h"
#include "concurrent_hal.h"
#include "wwd_resources.h"

// dns.h includes a class member, which doesn't compile in C++
#define class clazz
#include "dns.h"
#undef class

/**
 * Retrieves the country code from the DCT region.
 */
wiced_country_code_t fetch_country_code()
{
    const uint8_t* code = (const uint8_t*)dct_read_app_data(DCT_COUNTRY_CODE_OFFSET);

    wiced_country_code_t result =
        wiced_country_code_t(MK_CNTRY(code[0], code[1], hex_nibble(code[2])));
    if (code[0] == 0xFF || code[0] == 0)
    {
        result = WICED_COUNTRY_UNITED_KINGDOM; // default is UK, so channels 1-13 are available by default.
    }
    if (result == WICED_COUNTRY_JAPAN)
    {
        wwd_select_nvram_image_resource(1, nullptr); // lower tx power for TELEC certification
    }
    return result;
}

bool initialize_dct(platform_dct_wifi_config_t* wifi_config, bool force = false)
{
    bool changed = false;
    wiced_country_code_t country = fetch_country_code();
    if (force || wifi_config->device_configured != WICED_TRUE ||
        wifi_config->country_code != country)
    {
        if (!wifi_config->device_configured)
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
    wiced_result_t result = wiced_dct_read_lock((void**)&wifi_config, WICED_TRUE,
                                                DCT_WIFI_CONFIG_SECTION, 0, sizeof(*wifi_config));
    if (result == WICED_SUCCESS)
    {
        // the storage may not have been initialized, so device_configured will be 0xFF
        if (initialize_dct(wifi_config))
            result = wiced_dct_write((const void*)wifi_config, DCT_WIFI_CONFIG_SECTION, 0,
                                     sizeof(*wifi_config));
        wiced_dct_read_unlock(wifi_config, WICED_TRUE);
    }
    return result;
}

const static_ip_config_t* wlan_fetch_saved_ip_config()
{
    return (const static_ip_config_t*)dct_read_app_data(DCT_IP_CONFIG_OFFSET);
}

uint32_t HAL_NET_SetNetWatchDog(uint32_t timeOutInMS)
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
    wiced_result_t result = wiced_dct_read_lock((void**)&wifi_config, WICED_TRUE,
                                                DCT_WIFI_CONFIG_SECTION, 0, sizeof(*wifi_config));
    if (!result)
    {
        memset(wifi_config->stored_ap_list, 0, sizeof(wifi_config->stored_ap_list));
        result = wiced_dct_write((const void*)wifi_config, DCT_WIFI_CONFIG_SECTION, 0,
                                 sizeof(*wifi_config));
        wiced_dct_read_unlock(wifi_config, WICED_TRUE);
    }
    return result;
}

int is_set(const unsigned char* pv, unsigned length)
{
    int result = 0;
    int result2 = 0xFF;
    while (length-- > 0)
    {
        result |= *pv;
        result2 &= *pv;
    }
    return result && result2 != 0xFF;
}

bool is_ap_config_set(const wiced_config_ap_entry_t& ap_entry)
{
    bool set = ((ap_entry.details.SSID.length > 0 && ap_entry.details.SSID.length < 33)) ||
               is_set(ap_entry.details.BSSID.octet, sizeof(ap_entry.details.BSSID));
    return set;
}

/**
 * Determine if the DCT contains wifi credentials.
 * @return 0 if the device has credentials. 1 otherwise. (yes, it's backwards! The intent is that
 * negative values indicate some kind of error.)
 */
int wlan_has_credentials()
{
    int has_credentials = 0;
    platform_dct_wifi_config_t* wifi_config = NULL;
    wiced_result_t result = wiced_dct_read_lock((void**)&wifi_config, WICED_FALSE,
                                                DCT_WIFI_CONFIG_SECTION, 0, sizeof(*wifi_config));
    if (result == WICED_SUCCESS)
    {
        has_credentials = wifi_config->device_configured == WICED_TRUE &&
                          is_ap_config_set(wifi_config->stored_ap_list[0]);
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
    wiced_network_up_cancel = 0;
    return 0;
}

bool to_wiced_ip_address(wiced_ip_address_t& wiced, const dct_ip_address_v4_t& dct)
{
    if (dct != 0)
    {
        wiced.ip.v4 = dct;
        wiced.version = WICED_IPV4;
    }
    return (dct != 0);
}

void wlan_connect_timeout(os_timer_t t)
{
    wlan_connect_cancel(false);
}

/**
 * Do what is needed to finalize the connection.
 * @return
 */
wlan_result_t wlan_connect_finalize()
{
    os_timer_t cancel_timer = 0;
    os_timer_create(&cancel_timer, 60000, &wlan_connect_timeout, nullptr, false /* oneshot */,
                    nullptr);

    // enable connection from stored profiles
    wlan_result_t result = wiced_interface_up(WICED_STA_INTERFACE);
    if (!result)
    {
        HAL_NET_notify_connected();
        wiced_ip_setting_t settings;
        wiced_ip_address_t dns;
        const static_ip_config_t& ip_config = *wlan_fetch_saved_ip_config();

        switch (IPAddressSource(ip_config.config_mode))
        {
        case STATIC_IP:
            to_wiced_ip_address(settings.ip_address, ip_config.host);
            to_wiced_ip_address(settings.netmask, ip_config.netmask);
            to_wiced_ip_address(settings.gateway, ip_config.gateway);
            result = wiced_network_up(WICED_STA_INTERFACE, WICED_USE_STATIC_IP, &settings);
            if (!result)
            {
                if (to_wiced_ip_address(dns, ip_config.dns1))
                    dns_client_add_server_address(dns);
                if (to_wiced_ip_address(dns, ip_config.dns2))
                    dns_client_add_server_address(dns);
            }
        default:
            result = wiced_network_up(WICED_STA_INTERFACE, WICED_USE_EXTERNAL_DHCP_SERVER, NULL);
            break;
        }
    }
    else
    {
        wiced_network_down(WICED_STA_INTERFACE);
    }
    // DHCP happens synchronously
    HAL_NET_notify_dhcp(!result);
    wiced_network_up_cancel = 0;

    if (cancel_timer)
    {
        os_timer_destroy(cancel_timer, nullptr);
    }
    return result;
}

int wlan_select_antenna_impl(WLanSelectAntenna_TypeDef antenna);

WLanSelectAntenna_TypeDef fetch_antenna_selection()
{
    uint8_t result = *(const uint8_t*)dct_read_app_data(DCT_ANTENNA_SELECTION_OFFSET);
    if (result == 0xFF)
        result = ANT_INTERNAL; // default
    return WLanSelectAntenna_TypeDef(result);
}

STATIC_ASSERT(wlanselectantenna_typedef_is_size_1, sizeof(WLanSelectAntenna_TypeDef) == 1);

void save_antenna_selection(WLanSelectAntenna_TypeDef selection)
{
    dct_write_app_data(&selection, DCT_ANTENNA_SELECTION_OFFSET, DCT_ANTENNA_SELECTION_SIZE);
}

inline int wlan_refresh_antenna()
{
    return wlan_select_antenna_impl(fetch_antenna_selection());
}

int wlan_select_antenna(WLanSelectAntenna_TypeDef antenna)
{
    save_antenna_selection(antenna);
    return wiced_wlan_connectivity_initialized() ? wlan_refresh_antenna() : 0;
}

wlan_result_t wlan_activate()
{
    wlan_initialize_dct();
    wlan_result_t result = wiced_wlan_connectivity_init();
    if (!result)
        wiced_network_register_link_callback(HAL_NET_notify_connected, HAL_NET_notify_disconnected,
                                             WICED_STA_INTERFACE);
    wlan_refresh_antenna();
    return result;
}

wlan_result_t wlan_deactivate()
{
    wlan_disconnect_now();
    return 0;
}

wlan_result_t wlan_disconnect_now()
{
    socket_close_all();
    wlan_connect_cancel(false);
    wiced_result_t result = wiced_network_down(WICED_STA_INTERFACE);
    HAL_NET_notify_disconnected();
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
    if (wwd_wifi_get_rssi(&rssi))
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
    wlan_scan_result_t callback;
    void* callback_data;
    int count;
};

WLanSecurityType toSecurityType(wiced_security_t sec)
{
    if (sec == WICED_SECURITY_OPEN)
        return WLAN_SEC_UNSEC;
    if (sec & WEP_ENABLED)
        return WLAN_SEC_WEP;
    if (sec & WPA_SECURITY)
        return WLAN_SEC_WPA;
    if (sec & WPA2_SECURITY)
        return WLAN_SEC_WPA2;
    return WLAN_SEC_NOT_SET;
}

WLanSecurityCipher toCipherType(wiced_security_t sec)
{
    if (sec & AES_ENABLED)
        return WLAN_CIPHER_AES;
    if (sec & TKIP_ENABLED)
        return WLAN_CIPHER_TKIP;
    return WLAN_CIPHER_NOT_SET;
}

/*
 * Callback function to handle scan results
 */
wiced_result_t sniffer(wiced_scan_handler_result_t* malloced_scan_result)
{
    malloc_transfer_to_curr_thread(malloced_scan_result);

    SnifferInfo* info = (SnifferInfo*)malloced_scan_result->user_data;
    if (malloced_scan_result->status == WICED_SCAN_INCOMPLETE)
    {
        wiced_scan_result_t* record = &malloced_scan_result->ap_details;
        info->count++;
        if (!info->callback)
        {
            if (record->SSID.length == info->ssid_len &&
                !memcmp(record->SSID.value, info->ssid, info->ssid_len))
            {
                info->security = record->security;
                info->rssi = record->signal_strength;
            }
        }
        else
        {
            WiFiAccessPoint data;
            memcpy(data.ssid, record->SSID.value, record->SSID.length);
            memcpy(data.bssid, (uint8_t*)&record->BSSID, 6);
            data.ssidLength = record->SSID.length;
            data.ssid[data.ssidLength] = 0;
            data.security = toSecurityType(record->security);
            data.cipher = toCipherType(record->security);
            data.rssi = record->signal_strength;
            data.channel = record->channel;
            data.maxDataRate = record->max_data_rate;
            info->callback(&data, info->callback_data);
        }
    }
    else
    {
        wiced_rtos_set_semaphore(&info->complete);
    }
    free(malloced_scan_result);
    return WICED_SUCCESS;
}

wiced_result_t sniff_security(SnifferInfo* info)
{

    wiced_result_t result = wiced_rtos_init_semaphore(&info->complete);
    if (result != WICED_SUCCESS)
        return result;
    result = wiced_wifi_scan_networks(sniffer, info);
    if (result == WICED_SUCCESS)
    {
        wiced_rtos_get_semaphore(&info->complete, 30000);
    }
    wiced_rtos_deinit_semaphore(&info->complete);
    if (!info->rssi)
        result = WICED_NOT_FOUND;
    return result;
}

/**
 * Converts the given the current security credentials.
 */
wiced_security_t toSecurity(const char* ssid, unsigned ssid_len, WLanSecurityType sec,
                            WLanSecurityCipher cipher)
{
    int result = 0;
    switch (sec)
    {
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

    if (sec == WLAN_SEC_NOT_SET || // security not set, or WPA/WPA2 and cipher not set
        ((result & (WPA_SECURITY | WPA2_SECURITY) && (cipher == WLAN_CIPHER_NOT_SET))))
    {
        SnifferInfo info;
        memset(&info, 0, sizeof(info));
        info.ssid = ssid;
        info.ssid_len = ssid_len;
        if (!sniff_security(&info))
        {
            result = info.security;
        }
        else
        {
            result = WLAN_SET_CREDENTIALS_CIPHER_REQUIRED;
        }
    }
    return wiced_security_t(result);
}

bool equals_ssid(const char* ssid, wiced_ssid_t& current)
{
    return (strlen(ssid) == current.length) && !memcmp(ssid, current.value, current.length);
}

static bool wifi_creds_changed;
wiced_result_t add_wiced_wifi_credentials(const char* ssid, uint16_t ssidLen, const char* password,
                                          uint16_t passwordLen, wiced_security_t security,
                                          unsigned channel)
{
    platform_dct_wifi_config_t* wifi_config = NULL;
    wiced_result_t result = wiced_dct_read_lock((void**)&wifi_config, WICED_TRUE,
                                                DCT_WIFI_CONFIG_SECTION, 0, sizeof(*wifi_config));
    if (!result)
    {
        // the storage may not have been initialized, so device_configured will be 0xFF
        initialize_dct(wifi_config);

        int replace = -1;

        // find a slot with the same ssid
        for (unsigned i = 0; i < CONFIG_AP_LIST_SIZE; i++)
        {
            if (equals_ssid(ssid, wifi_config->stored_ap_list[i].details.SSID))
            {
                replace = i;
                break;
            }
        }

        if (replace < 0)
        {
            // shuffle all slots along
            memmove(wifi_config->stored_ap_list + 1, wifi_config->stored_ap_list,
                    sizeof(wiced_config_ap_entry_t) * (CONFIG_AP_LIST_SIZE - 1));
            replace = 0;
        }
        wiced_config_ap_entry_t& entry = wifi_config->stored_ap_list[replace];
        memset(&entry, 0, sizeof(entry));
        passwordLen = std::min(passwordLen, uint16_t(64));
        ssidLen = std::min(ssidLen, uint16_t(32));
        memcpy(entry.details.SSID.value, ssid, ssidLen);
        entry.details.SSID.length = ssidLen;
        if (security == WICED_SECURITY_WEP_PSK && passwordLen > 1 && password[0] > 4)
        {
            // convert from hex to binary
            entry.security_key_length =
                hex_decode((uint8_t*)entry.security_key, sizeof(entry.security_key), password);
        }
        else
        {
            memcpy(entry.security_key, password, passwordLen);
            entry.security_key_length = passwordLen;
        }
        entry.details.security = security;
        entry.details.channel = channel;
        result = wiced_dct_write((const void*)wifi_config, DCT_WIFI_CONFIG_SECTION, 0,
                                 sizeof(*wifi_config));
        if (!result)
            wifi_creds_changed = true;
        wiced_dct_read_unlock(wifi_config, WICED_TRUE);
    }
    return result;
}

int wlan_set_credentials_internal(const char* ssid, uint16_t ssidLen, const char* password,
                                  uint16_t passwordLen, WLanSecurityType security,
                                  WLanSecurityCipher cipher, unsigned channel, unsigned flags)
{
    int result = WICED_ERROR;
    if (ssidLen > 0 && ssid)
    {
        int security_result = toSecurity(ssid, ssidLen, security, cipher);
        if (security_result == WLAN_SET_CREDENTIALS_CIPHER_REQUIRED)
        {
            result = WLAN_SET_CREDENTIALS_CIPHER_REQUIRED;
        }
        else if (flags & WLAN_SET_CREDENTIALS_FLAGS_DRY_RUN)
        {
            result = WICED_SUCCESS;
        }
        else
        {
            result = add_wiced_wifi_credentials(ssid, ssidLen, password, passwordLen,
                                                wiced_security_t(security_result), channel);
        }
    }
    return result;
}

int wlan_set_credentials(WLanCredentials* c)
{
    // size v1: 28
    // added flags: size 32
    int flags = 0;
    if (c->size >= 32)
    {
        flags = c->flags;
    }
    STATIC_ASSERT(wlan_credentials_size, sizeof(WLanCredentials) == 32);
    return wlan_set_credentials_internal(c->ssid, c->ssid_len, c->password, c->password_len,
                                         c->security, c->cipher, c->channel, flags);
}

softap_handle current_softap_handle;

void wlan_smart_config_init()
{

    wifi_creds_changed = false;
    if (!current_softap_handle)
    {
        softap_config config;
        config.softap_complete = HAL_WLAN_notify_simple_config_done;
        wlan_disconnect_now();
        current_softap_handle = softap_start(&config);
    }
}

bool wlan_smart_config_finalize()
{
    if (current_softap_handle)
    {
        softap_stop(current_softap_handle);
        wlan_disconnect_now(); // force a full refresh
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
}

void wlan_set_error_count(uint32_t errorCount)
{
}

inline void setAddress(wiced_ip_address_t* addr, HAL_IPAddress& target)
{
    HAL_IPV4_SET(&target, GET_IPV4_ADDRESS(*addr));
}

void wlan_fetch_ipconfig(WLanConfig* config)
{
    wiced_ip_address_t addr;
    wiced_interface_t ifup = WICED_STA_INTERFACE;

    if (wiced_network_is_up(ifup))
    {

        if (wiced_ip_get_ipv4_address(ifup, &addr) == WICED_SUCCESS)
            setAddress(&addr, config->nw.aucIP);

        if (wiced_ip_get_netmask(ifup, &addr) == WICED_SUCCESS)
            setAddress(&addr, config->nw.aucSubnetMask);

        if (wiced_ip_get_gateway_address(ifup, &addr) == WICED_SUCCESS)
            setAddress(&addr, config->nw.aucDefaultGateway);
    }

    wiced_mac_t my_mac_address;
    if (wiced_wifi_get_mac_address(&my_mac_address) == WICED_SUCCESS)
        memcpy(config->nw.uaMacAddr, &my_mac_address, 6);

    wl_bss_info_t ap_info;
    wiced_security_t sec;

    if (wwd_wifi_get_ap_info(&ap_info, &sec) == WWD_SUCCESS)
    {
        uint8_t len = std::min(ap_info.SSID_len, uint8_t(32));
        memcpy(config->uaSSID, ap_info.SSID, len);
        config->uaSSID[len] = 0;

        if (config->size >= WLanConfig_Size_V2)
        {
            memcpy(config->BSSID, ap_info.BSSID.octet, sizeof(config->BSSID));
        }
    }
    // todo DNS and DHCP servers
}

void SPARK_WLAN_SmartConfigProcess()
{
}

/** Select the Wi-Fi antenna
 * WICED_ANTENNA_1    = 0, selects u.FL Antenna
 * WICED_ANTENNA_2    = 1, selects Chip Antenna
 * WICED_ANTENNA_AUTO = 3, enables auto antenna selection ie. automatic diversity
 *
 * @param antenna: The antenna configuration to use
 *
 * @return   0 : if the antenna selection was successfully set
 *          -1 : if the antenna selection was not set
 *
 */
int wlan_select_antenna_impl(WLanSelectAntenna_TypeDef antenna)
{

    wwd_result_t result;
    switch (antenna)
    {
#if PLATFORM_ID == 6 // Photon
    case ANT_EXTERNAL:
        result = wwd_wifi_select_antenna(WICED_ANTENNA_1);
        break;
    case ANT_INTERNAL:
        result = wwd_wifi_select_antenna(WICED_ANTENNA_2);
        break;
#else
    case ANT_INTERNAL:
        result = wwd_wifi_select_antenna(WICED_ANTENNA_1);
        break;
    case ANT_EXTERNAL:
        result = wwd_wifi_select_antenna(WICED_ANTENNA_2);
        break;
#endif
    case ANT_AUTO:
        result = wwd_wifi_select_antenna(WICED_ANTENNA_AUTO);
        break;
    default:
        result = WWD_DOES_NOT_EXIST;
        break;
    }
    if (result == WWD_SUCCESS)
        return 0;
    else
        return -1;
}

void wlan_connect_cancel(bool called_from_isr)
{
    wiced_network_up_cancel = 1;
    wwd_wifi_join_cancel(called_from_isr ? WICED_TRUE : WICED_FALSE);
}

/**
 * Sets the IP source - static or dynamic.
 */
void wlan_set_ipaddress_source(IPAddressSource source, bool persist, void* reserved)
{
    char c = source;
    dct_write_app_data(&c, DCT_IP_CONFIG_OFFSET + offsetof(static_ip_config_t, config_mode), 1);
}

void assign_if_set(dct_ip_address_v4_t& dct_address, const HAL_IPAddress* address)
{
    if (address && is_ipv4(address))
    {
        dct_address = address->ipv4;
    }
}

/**
 * Sets the IP Addresses to use when the device is in static IP mode.
 * @param host
 * @param netmask
 * @param gateway
 * @param dns1
 * @param dns2
 * @param reserved
 */
void wlan_set_ipaddress(const HAL_IPAddress* host, const HAL_IPAddress* netmask,
                        const HAL_IPAddress* gateway, const HAL_IPAddress* dns1,
                        const HAL_IPAddress* dns2, void* reserved)
{
    const static_ip_config_t* pconfig = wlan_fetch_saved_ip_config();
    static_ip_config_t config;
    memcpy(&config, pconfig, sizeof(config));
    assign_if_set(config.host, host);
    assign_if_set(config.netmask, netmask);
    assign_if_set(config.gateway, gateway);
    assign_if_set(config.dns1, dns1);
    assign_if_set(config.dns2, dns2);
    dct_write_app_data(&config, DCT_IP_CONFIG_OFFSET, sizeof(config));
}

int wlan_scan(wlan_scan_result_t callback, void* cookie)
{
    SnifferInfo info;
    memset(&info, 0, sizeof(info));
    info.callback = callback;
    info.callback_data = cookie;
    int result = sniff_security(&info);
    return result < 0 ? result : info.count;
}

/**
 * Lists all WLAN credentials currently stored on the device
 */
int wlan_get_credentials(wlan_scan_result_t callback, void* callback_data)
{
    int count = 0;
    platform_dct_wifi_config_t* wifi_config = NULL;
    wiced_result_t result = wiced_dct_read_lock((void**)&wifi_config, WICED_FALSE,
                                                DCT_WIFI_CONFIG_SECTION, 0, sizeof(*wifi_config));
    if (!result)
    {
        // the storage may not have been initialized, so device_configured will be 0xFF
        initialize_dct(wifi_config);

        // iterate through each stored ap
        for (int i = 0; i < CONFIG_AP_LIST_SIZE; i++)
        {
            const wiced_config_ap_entry_t& ap = wifi_config->stored_ap_list[i];

            if (!is_ap_config_set(ap))
            {
                continue;
            }
            count++;

            if (!callback)
            {
                continue;
            }

            const wiced_ap_info_t* record = &ap.details;

            WiFiAccessPoint data;
            memcpy(data.ssid, record->SSID.value, record->SSID.length);
            memcpy(data.bssid, (uint8_t*)&record->BSSID, 6);
            data.ssidLength = record->SSID.length;
            data.ssid[data.ssidLength] = 0;
            data.security = toSecurityType(record->security);
            data.cipher = toCipherType(record->security);
            data.rssi = record->signal_strength;
            data.channel = record->channel;
            data.maxDataRate = record->max_data_rate;

            callback(&data, callback_data);
        }
        wiced_dct_read_unlock(wifi_config, WICED_FALSE);
    }
    return result < 0 ? result : count;
}
