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
#include "dct.h"
#include "wwd_management.h"
#include "wiced_tls.h"
#include "wiced_utilities.h"
#include "wiced_supplicant.h"
#include "wiced_tls.h"
#include "rtc_hal.h"
#include "tls_callbacks.h"

uint64_t tls_host_get_time_ms_local() {
    uint64_t time_ms;
    wiced_time_get_utc_time_ms( (wiced_utc_time_ms_t*) &time_ms );
    return time_ms;
}

LOG_SOURCE_CATEGORY("hal.wlan");

// dns.h includes a class member, which doesn't compile in C++
#define class clazz
#include "dns.h"
#undef class

#ifndef EAP_DEFAULT_OUTER_IDENTITY
#define EAP_DEFAULT_OUTER_IDENTITY "anonymous"
#endif

int wlan_clear_enterprise_credentials();
int wlan_set_enterprise_credentials(WLanCredentials* c);

struct WPAEnterpriseContext {
    WPAEnterpriseContext() {
        tls_session = nullptr;
        tls_context = nullptr;
        tls_identity = nullptr;
        supplicant_workspace = nullptr;
        initialized = false;
        supplicant_running = false;
        supplicant_initialized = false;
    }

    ~WPAEnterpriseContext() {
        if (tls_session)
            free(tls_session);
        if (tls_context)
            free(tls_context);
        if (tls_identity)
            free(tls_identity);
        if (supplicant_workspace)
            free(supplicant_workspace);
    }

    void init() {
        if (!initialized) {
            TlsCallbacks cb;
            cb.tls_host_get_time_ms = tls_host_get_time_ms_local;
            tls_set_callbacks(cb);
            initialized = true;
            tls_session = (wiced_tls_session_t*)calloc(1, sizeof(wiced_tls_session_t));
            tls_context = (wiced_tls_context_t*)calloc(1, sizeof(wiced_tls_context_t));
            tls_identity = (wiced_tls_identity_t*)calloc(1, sizeof(wiced_tls_identity_t));
            supplicant_workspace = (supplicant_workspace_t*)calloc(1, sizeof(supplicant_workspace_t));
        }
    }

    wiced_tls_session_t* tls_session;
    wiced_tls_context_t* tls_context;
    wiced_tls_identity_t* tls_identity;
    supplicant_workspace_t* supplicant_workspace;
    bool initialized;
    bool supplicant_running;
    bool supplicant_initialized;
};

static WPAEnterpriseContext eap_context;

/**
 * Retrieves the country code from the DCT region.
 */
wiced_country_code_t fetch_country_code()
{
    const uint8_t* code = (const uint8_t*)dct_read_app_data_lock(DCT_COUNTRY_CODE_OFFSET);

    wiced_country_code_t result =
        wiced_country_code_t(MK_CNTRY(code[0], code[1], hex_nibble(code[2])));

    // if Japan explicitly configured, lower tx power for TELEC certification
    if (result == WICED_COUNTRY_JAPAN)
    {
        wwd_select_nvram_image_resource(1, nullptr);
    }

    // if no country configured, use Japan WiFi config for compatibility with older firmware
    if (code[0] == 0xFF || code[0] == 0)
    {
        result = WICED_COUNTRY_JAPAN;
    }

    dct_read_app_data_unlock(DCT_COUNTRY_CODE_OFFSET);

    return result;
}

bool isWiFiPowersaveClockDisabled() {
    uint8_t current = 0;
    dct_read_app_data_copy(DCT_RADIO_FLAGS_OFFSET, &current, sizeof(current));
    return ((current&3) == 0x2);
}

bool initialize_dct(platform_dct_wifi_config_t* wifi_config, bool force=false)
{
    bool changed = false;
    wiced_country_code_t country = fetch_country_code();
    if (force || wifi_config->device_configured != WICED_TRUE ||
        wifi_config->country_code != country)
    {
        if (!wifi_config->device_configured)
        {
            memset(wifi_config, 0, sizeof(*wifi_config));
        }
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

static_ip_config_t* wlan_fetch_saved_ip_config(static_ip_config_t* config)
{
    dct_read_app_data_copy(DCT_IP_CONFIG_OFFSET, config, sizeof(static_ip_config_t));
    return config;
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
    WARN("Clearing WiFi credentials");
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

    wlan_clear_enterprise_credentials();
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
    return result && result2!=0xFF;
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

bool check_enterprise_credentials(WLanCredentials* c)
{
    bool is_valid = true;
    if (c && c->eap_type == WLAN_EAP_TYPE_PEAP) {
        // Check if there is a username in inner_identity
        if (!c->inner_identity || c->inner_identity_len > 64 || c->inner_identity_len == 0)
            is_valid = false;
        // Check if there is a stored password in security_key
        if (!c->password || c->password_len > 64 || c->password_len == 0)
            is_valid = false;
    } else if (c && c->eap_type == WLAN_EAP_TYPE_TLS) {
        if (!c->client_certificate || c->client_certificate_len == 0 || !c->private_key || c->private_key_len == 0)
            is_valid = false;
        if (c->client_certificate_len > CERTIFICATE_SIZE || c->private_key_len > PRIVATE_KEY_SIZE)
            is_valid = false;
    } else {
        is_valid = false;
    }

    if (is_valid && c->ca_certificate && c->ca_certificate_len > CERTIFICATE_SIZE)
        is_valid = false;

    return is_valid;
}

bool is_eap_configuration_valid(const eap_config_t* eap_conf, platform_dct_security_t* sec = NULL)
{
    bool is_valid = true;
    if (eap_conf && eap_conf->type == WLAN_EAP_TYPE_PEAP) {
        // Check if there is a username in inner_identity
        if (eap_conf->inner_identity_len > 64 || eap_conf->inner_identity_len == 0)
            is_valid = false;
        // Check if there is a stored password in security_key
        if (eap_conf->security_key_len > 64 || eap_conf->security_key_len == 0)
            is_valid = false;
    } else if (eap_conf && eap_conf->type == WLAN_EAP_TYPE_TLS) {
        // if (eap_conf->inner_identity_len > 64 || eap_conf->inner_identity_len == 0)
        //     is_valid = false;

        // Check if there is a client certificate and private key
        wiced_result_t result = WICED_SUCCESS;
        bool locked = false;
        if (sec == NULL) {
            result = wiced_dct_read_lock((void**)&sec, WICED_FALSE,
                                         DCT_SECURITY_SECTION, 0, sizeof(*sec));
            locked = true;
        }
        if (sec && result == WICED_SUCCESS) {
            if (strnlen(sec->private_key, PRIVATE_KEY_SIZE) == 0)
                is_valid = false;
            if (strnlen(sec->certificate, CERTIFICATE_SIZE) == 0)
                is_valid = false;
        } else {
            is_valid = false;
        }
        if (locked) {
            wiced_dct_read_unlock(sec, WICED_FALSE);
        }
    } else {
        is_valid = false;
    }

    return is_valid;
}

int wlan_has_enterprise_credentials()
{
    int has_credentials = 0;

    // TODO: For now we are using only 1 global eap configuration for all access points
    const eap_config_t* eap_conf = (eap_config_t*)dct_read_app_data_lock(DCT_EAP_CONFIG_OFFSET);
    has_credentials = (int)is_eap_configuration_valid(eap_conf);
    dct_read_app_data_unlock(DCT_EAP_CONFIG_OFFSET);

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
    return (dct!=0);
}

void wlan_connect_timeout(os_timer_t t)
{
    wlan_connect_cancel(false);
}

int wlan_supplicant_start()
{
    LOG(TRACE, "Starting supplicant");
    // Fetch configuration
    uint8_t eap_type = WLAN_EAP_TYPE_NONE;

    const eap_config_t* eap_conf = (const eap_config_t*)dct_read_app_data_lock(DCT_EAP_CONFIG_OFFSET);
    if (!is_eap_configuration_valid(eap_conf)) {
        dct_read_app_data_unlock(DCT_EAP_CONFIG_OFFSET);
        return 1;
    }
    eap_type = eap_conf->type;
    dct_read_app_data_unlock(DCT_EAP_CONFIG_OFFSET);

    wiced_result_t result = WICED_SUCCESS;

    eap_context.init();

    platform_dct_security_t* sec = NULL;
    result = wiced_dct_read_lock((void**)&sec, WICED_FALSE,
                                 DCT_SECURITY_SECTION, 0, sizeof(*sec));

    if (result == WICED_SUCCESS) {
        if (eap_type == WLAN_EAP_TYPE_PEAP) {
            // result = wiced_tls_init_identity(eap_context.tls_identity, NULL, 0, NULL, 0 );
            memset(eap_context.tls_identity, 0, sizeof(wiced_tls_identity_t));
        } else if (eap_type == WLAN_EAP_TYPE_TLS) {
            result = wiced_tls_init_identity(eap_context.tls_identity,
                                             (const char*)sec->private_key,
                                             (uint32_t)strnlen((const char*)sec->private_key, PRIVATE_KEY_SIZE),
                                             (const uint8_t*)sec->certificate,
                                             (uint32_t)strnlen((const char*)sec->certificate, CERTIFICATE_SIZE));
        } else {
            result = WICED_ERROR;
        }
    }

    wiced_dct_read_unlock(sec, WICED_FALSE);

    if (result != WICED_SUCCESS)
        return result;

    eap_conf = (const eap_config_t*)dct_read_app_data_lock(DCT_EAP_CONFIG_OFFSET);

    wiced_tls_init_context(eap_context.tls_context, eap_context.tls_identity, NULL);
    if (eap_context.tls_session->length > 0) {
        memcpy(&eap_context.tls_context->session, eap_context.tls_session, sizeof(wiced_tls_session_t));
    } else {
        memset(&eap_context.tls_context->session, 0, sizeof(wiced_tls_session_t));
    }

    if (eap_conf->ca_certificate_len) {
        wiced_tls_init_root_ca_certificates((const char*)eap_conf->ca_certificate, eap_conf->ca_certificate_len);
    }

    result = (wiced_result_t)besl_supplicant_init(eap_context.supplicant_workspace, (eap_type_t)eap_conf->type, WWD_STA_INTERFACE);
    if ((besl_result_t)result == BESL_SUCCESS) {
        eap_context.supplicant_initialized = true;
        wiced_supplicant_enable_tls(eap_context.supplicant_workspace, eap_context.tls_context);
        if (eap_conf->outer_identity_len) {
            besl_supplicant_set_identity(eap_context.supplicant_workspace, (const char*)eap_conf->outer_identity, eap_conf->outer_identity_len);
        } else {
            // It's probably a good idea to set a default outer identity
            besl_supplicant_set_identity(eap_context.supplicant_workspace, EAP_DEFAULT_OUTER_IDENTITY, strlen(EAP_DEFAULT_OUTER_IDENTITY));
        }
        if (eap_conf->type == WLAN_EAP_TYPE_PEAP) {
            supplicant_mschapv2_identity_t mschap_identity;
            char mschap_password[32];

            uint8_t*  password = (uint8_t*)eap_conf->security_key;
            uint8_t*  unicode  = (uint8_t*)mschap_password;

            for (int i = 0; i <= eap_conf->security_key_len; i++) {
                *unicode++ = *password++;
                *unicode++ = '\0';
            }

            mschap_identity.identity = (uint8_t*)eap_conf->inner_identity;
            mschap_identity.identity_length = eap_conf->inner_identity_len;

            mschap_identity.password = (uint8_t*)mschap_password;
            mschap_identity.password_length = eap_conf->security_key_len * 2;

            besl_supplicant_set_inner_identity(eap_context.supplicant_workspace, (eap_type_t)eap_conf->type, &mschap_identity);
        }

        result = (wiced_result_t)besl_supplicant_start(eap_context.supplicant_workspace);
        LOG(TRACE, "Supplicant started %d", (int)result);
    }

    dct_read_app_data_unlock(DCT_EAP_CONFIG_OFFSET);

    if (result == 0) {
        eap_context.supplicant_running = true;
    }

    return result;
}

int wlan_supplicant_cancel(int isr)
{
    if (eap_context.supplicant_running) {
        besl_supplicant_cancel(eap_context.supplicant_workspace, isr);
    }
    return 0;
}

int wlan_supplicant_stop()
{
    LOG(TRACE, "Stopping supplicant");

    wiced_tls_deinit_context(eap_context.tls_context);
    wiced_tls_deinit_root_ca_certificates();
    wiced_tls_deinit_identity(eap_context.tls_identity);
    if (eap_context.supplicant_initialized) {
        besl_supplicant_deinit(eap_context.supplicant_workspace);
        eap_context.supplicant_initialized = false;
    }

    eap_context.supplicant_running = false;

    LOG(TRACE, "Supplicant stopped");

    return 0;
}

int wlan_restart() {
    wiced_wlan_connectivity_deinit();
    wiced_wlan_connectivity_init();

    return 0;
}

/*
 * This is almost an exact copy-paste of wiced_join_ap.
 * We need to manually loop through the APs here in order to know when to start
 * WPA Enterprise supplicant.
 */
static wiced_result_t wlan_join() {
    unsigned int             a;
    int                      retries;
    wiced_config_ap_entry_t* ap;
    wiced_result_t           result = WICED_NO_STORED_AP_IN_DCT;
    char                     ssid_name[SSID_NAME_SIZE + 1];

    for ( retries = WICED_JOIN_RETRY_ATTEMPTS; retries != 0 && !wiced_network_up_cancel; --retries )
    {
        /* Try all stored APs */
        for ( a = 0; a < CONFIG_AP_LIST_SIZE && !wiced_network_up_cancel; ++a )
        {
            /* Check if the entry is valid */
            if (wiced_dct_read_lock((void**)&ap, WICED_FALSE, DCT_WIFI_CONFIG_SECTION,
                                    (uint32_t) (OFFSETOF(platform_dct_wifi_config_t, stored_ap_list) +
                                     a * sizeof(wiced_config_ap_entry_t) ),
                                    sizeof(wiced_config_ap_entry_t)) != WICED_SUCCESS)
            {
                continue;
            }

            if ( ap->details.SSID.length != 0 )
            {
                bool suppl = false;
                bool join = true;
                if (ap->details.security & ENTERPRISE_ENABLED) {
                    if (!wlan_has_enterprise_credentials()) {
                        // Workaround. If connecting not for the first time, for some reason authentication fails.
                        // It works if we reset wireless module though
                        // ¯\_(ツ)_/¯
                        wlan_restart();
                        // We need to start supplicant
                        if (wlan_supplicant_start()) {
                            // Early error
                            wlan_restart();
                            wlan_supplicant_cancel(0);
                            wlan_supplicant_stop();
                            result = WICED_ERROR;
                            join = false;
                        } else {
                            suppl = true;
                        }
                    }
                }

                if (join) {
                    memset(ssid_name, 0, sizeof(ssid_name));
                    memcpy(ssid_name, ap->details.SSID.value, ap->details.SSID.length);
                    LOG(INFO, "Joining %s", ssid_name);
                    result = wiced_join_ap_specific( &ap->details, ap->security_key_length, ap->security_key );
                }

                if (suppl) {
                    if (!result) {
                        memcpy(eap_context.tls_session, &eap_context.tls_context->session, sizeof(wiced_tls_session_t));
                    } else {
                        // And another workaround here: in case of failed authentication we might get somewhat deadlocked
                        // while stopping supplicant
                        // ¯\_(ツ)_/¯
                        wlan_restart();
                        wlan_supplicant_cancel(0);
                    }
                    wlan_supplicant_stop();
                }
            }

            wiced_dct_read_unlock( (wiced_config_ap_entry_t*) ap, WICED_FALSE );

            if ( result == WICED_SUCCESS )
            {
                return result;
            }
        }
    }
    return result;
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
    wlan_result_t result = (wlan_result_t)wlan_join();
    if (!result)
    {
        HAL_NET_notify_connected();
        wiced_ip_setting_t settings;
        wiced_ip_address_t dns;
        static_ip_config_t ip_config;
        wlan_fetch_saved_ip_config(&ip_config);

        switch (IPAddressSource(ip_config.config_mode))
        {
            case STATIC_IP:
                INFO("Bringing WiFi interface up with static IP");
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
                break;
            default:
                INFO("Bringing WiFi interface up with DHCP");
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
    uint8_t result = 0;
    dct_read_app_data_copy(DCT_ANTENNA_SELECTION_OFFSET, &result, sizeof(result));
    if (result==0xFF)
        result = ANT_INTERNAL;  // default

    INFO("Using %s antenna", result == ANT_INTERNAL ? "internal" : (result == ANT_EXTERNAL ? "external" : "auto"));
    return WLanSelectAntenna_TypeDef(result);
}

STATIC_ASSERT(wlanselectantenna_typedef_is_size_1, sizeof(WLanSelectAntenna_TypeDef)==1);

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
#if PLATFORM_ID==PLATFORM_P1
	if (isWiFiPowersaveClockDisabled()) {
		wwd_set_wlan_sleep_clock_enabled(WICED_FALSE);
	}
#endif

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
    // FIXME:
    // This was required for PR #1125 to reduce Wi-Fi power completely for Sleep stop mode
    // however it introduced a new connectivity bug in LwIP that was unresolved in commit 87892a3837d4,
    // which was reverted in commit 6b78d7662f62.  WICED should be updated to pull in a newer version of LwIP
    // before this change is attempted again.
    // wiced_result_t result = wiced_wlan_connectivity_deinit();
    // return result;
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
    wlan_scan_result_t callback;
    void* callback_data;
    int count;
};

WLanSecurityType toSecurityType(wiced_security_t sec)
{
    if (sec==WICED_SECURITY_OPEN)
        return WLAN_SEC_UNSEC;
    if (sec & WEP_ENABLED)
        return WLAN_SEC_WEP;
    if ((sec & (WPA2_SECURITY | ENTERPRISE_ENABLED)) == (WPA2_SECURITY | ENTERPRISE_ENABLED))
        return WLAN_SEC_WPA2_ENTERPRISE;
    if ((sec & (WPA_SECURITY | ENTERPRISE_ENABLED)) == (WPA_SECURITY | ENTERPRISE_ENABLED))
        return WLAN_SEC_WPA_ENTERPRISE;
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
wiced_result_t sniffer( wiced_scan_handler_result_t* malloced_scan_result )
{
    if (malloced_scan_result != NULL)
    {
        malloc_transfer_to_curr_thread( malloced_scan_result );

        SnifferInfo* info = (SnifferInfo*)malloced_scan_result->user_data;
        if ( malloced_scan_result->status == WICED_SCAN_INCOMPLETE )
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
        free( malloced_scan_result );
    }
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
        case WLAN_SEC_WPA_ENTERPRISE:
            result = WPA_SECURITY | ENTERPRISE_ENABLED;
            break;
        case WLAN_SEC_WPA2_ENTERPRISE:
            result = WPA2_SECURITY | ENTERPRISE_ENABLED;
            break;
    }

    if (cipher & WLAN_CIPHER_AES)
        result |= AES_ENABLED;
    if (cipher & WLAN_CIPHER_TKIP)
        result |= TKIP_ENABLED;

    if (sec==WLAN_SEC_NOT_SET ||    // security not set, or WPA/WPA2 and cipher not set
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
    return (strlen(ssid)==current.length) && !memcmp(ssid, current.value, current.length);
}

static bool wifi_creds_changed;
wiced_result_t add_wiced_wifi_credentials(const char *ssid, uint16_t ssidLen, const char *password,
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

        // We can only store a single enterprise access point
        if (security & ENTERPRISE_ENABLED) {
            for (unsigned i = 0; i < CONFIG_AP_LIST_SIZE; i++)
            {
                if (wifi_config->stored_ap_list[i].details.security & ENTERPRISE_ENABLED)
                {
                    if (replace < 0) {
                        replace = i;
                    } else {
                        memset(&wifi_config->stored_ap_list[i], 0, sizeof(wiced_config_ap_entry_t));
                    }
                }
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

int wlan_set_credentials_internal(const char *ssid, uint16_t ssidLen, const char *password,
                                  uint16_t passwordLen, WLanSecurityType security,
                                  WLanSecurityCipher cipher, unsigned channel, unsigned flags)
{
    if (!ssid || !ssidLen || ssidLen > SSID_NAME_SIZE) {
        return WLAN_INVALID_SSID_LENGTH;
    }
    const int wicedSecurity = toSecurity(ssid, ssidLen, security, cipher);
    if (wicedSecurity == WLAN_SET_CREDENTIALS_CIPHER_REQUIRED) {
        return WLAN_SET_CREDENTIALS_CIPHER_REQUIRED;
    }
    // Check WEP password length
    if (wicedSecurity & WEP_ENABLED && (!password || passwordLen < 5 /* 40-bit key in printable characters */ ||
            passwordLen > 58 /* 232-bit key in hex characters */)) {
        return WLAN_INVALID_KEY_LENGTH;
    }
    // Check WPA/WPA2 password length
    if ((wicedSecurity & WPA_SECURITY || wicedSecurity & WPA2_SECURITY) && !(wicedSecurity & ENTERPRISE_ENABLED) &&
        (!password || passwordLen < WSEC_MIN_PSK_LEN || passwordLen > WSEC_MAX_PSK_LEN)) {
        return WLAN_INVALID_KEY_LENGTH;
    }
    if (flags & WLAN_SET_CREDENTIALS_FLAGS_DRY_RUN) {
        return 0;
    }
    return add_wiced_wifi_credentials(ssid, ssidLen, password, passwordLen, wiced_security_t(wicedSecurity), channel);
}

int wlan_clear_enterprise_credentials() {
    LOG(INFO, "Clearing enterprise credentials");

    return !wlan_set_enterprise_credentials(NULL);
}

int wlan_set_enterprise_credentials(WLanCredentials* c)
{
    LOG(TRACE, "Trying to set EAP credentials");
    if (c != NULL && !check_enterprise_credentials(c))
        return 1;

    platform_dct_security_t* sec = (platform_dct_security_t*)malloc(sizeof(platform_dct_security_t));
    if (sec == NULL) {
        return 1;
    }

    eap_config_t* eap_config = (eap_config_t*)malloc(sizeof(eap_config_t));
    if (eap_config == NULL) {
        free(sec);
        return 1;
    }
    memset(sec, 0, sizeof(*sec));
    memset(eap_config, 0, sizeof(eap_config_t));

    if (c) {
        if (c->client_certificate && c->client_certificate_len) {
            memcpy(sec->certificate, c->client_certificate, c->client_certificate_len);
        }
        if (c->private_key && c->private_key_len) {
            memcpy(sec->private_key, c->private_key, c->private_key_len);
        }
    }

    if (c) {
        eap_config->type = (uint8_t)c->eap_type;
        // FIXME length check
        if (c->inner_identity && c->inner_identity_len) {
            memcpy(eap_config->inner_identity, c->inner_identity, c->inner_identity_len);
            eap_config->inner_identity_len = c->inner_identity_len;
        }
        if (c->outer_identity && c->outer_identity_len) {
            memcpy(eap_config->outer_identity, c->outer_identity, c->outer_identity_len);
            eap_config->outer_identity_len = c->outer_identity_len;
        }
        if (c->password && c->password_len) {
            memcpy(eap_config->security_key, c->password, c->password_len);
            eap_config->security_key_len = c->password_len;
        }
        if (c->ca_certificate && c->ca_certificate_len) {
            memcpy(eap_config->ca_certificate, c->ca_certificate, c->ca_certificate_len);
            eap_config->ca_certificate_len = c->ca_certificate_len;
        }
    }

    int result = (int)(!is_eap_configuration_valid(eap_config, sec));
    LOG(INFO, "EAP config valid: %d", !result);

    if (!result || c == NULL) {
        // Write
        LOG(INFO, "Writing EAP configuration");
        wiced_dct_write(sec, DCT_SECURITY_SECTION, 0, sizeof(platform_dct_security_t));
        dct_write_app_data(eap_config, DCT_EAP_CONFIG_OFFSET, sizeof(eap_config_t));
    }
    free(sec);
    free(eap_config);

    return result;
}

int wlan_set_credentials(WLanCredentials* c)
{
    // size v1: 28
    // added flags: size 32
    // v3: wpa enterprise, version field, size 76
    int flags = 0;
    int version = 1;
    if (c->size >= 32)
    {
        version = 2;
        flags = c->flags;
    }

    if (c->size > 32) {
        version = c->version;
    }

    (void)version;

    STATIC_ASSERT(wlan_credentials_size, sizeof(WLanCredentials)==76);

    int result = wlan_set_credentials_internal(c->ssid, c->ssid_len, c->password, c->password_len,
                                               c->security, c->cipher, c->channel, flags | WLAN_SET_CREDENTIALS_FLAGS_DRY_RUN);
    if (result == WICED_SUCCESS && version > 2 && c->eap_type != WLAN_EAP_TYPE_NONE) {
        result = wlan_set_enterprise_credentials(c);
    }

    if (!result) {
        // Save
        LOG(INFO, "Saving credentials");
        result = wlan_set_credentials_internal(c->ssid, c->ssid_len, c->password, c->password_len,
                                               c->security, c->cipher, c->channel, flags);
    }

    return result;
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
        wlan_restart();
        current_softap_handle = softap_start(&config);
    }
}

bool wlan_smart_config_finalize()
{
    if (current_softap_handle)
    {
        softap_stop(current_softap_handle);
        wlan_disconnect_now();  // force a full refresh
        wlan_restart();
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
    LOG(TRACE, "connect cancel");
    wiced_network_up_cancel = 1;
    wwd_wifi_join_cancel(called_from_isr ? WICED_TRUE : WICED_FALSE);
    wlan_supplicant_cancel((int)called_from_isr);
}

/**
 * Sets the IP source - static or dynamic.
 */
void wlan_set_ipaddress_source(IPAddressSource source, bool persist, void* reserved)
{
    char c = source;
    dct_write_app_data(&c, DCT_IP_CONFIG_OFFSET+offsetof(static_ip_config_t, config_mode), 1);
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
    static_ip_config_t config;
    wlan_fetch_saved_ip_config(&config);
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
    int result =  sniff_security(&info);
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
            const wiced_config_ap_entry_t &ap = wifi_config->stored_ap_list[i];

            if (!is_ap_config_set(ap))
            {
                continue;
            }
            count++;

            if (!callback)
            {
                continue;
            }

            const wiced_ap_info_t *record = &ap.details;

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
