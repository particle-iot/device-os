
#include "wlan_ap_hal.h"
#include "wlan_internal.h"
#include "service_debug.h"
#include "wiced.h"
#include "softap.h"
#include "device_code.h"
#include <string.h>


wiced_result_t ap_get_credentials(wiced_config_soft_ap_t& creds);


wiced_result_t wlan_ap_up(wiced_config_soft_ap_t& creds, const wiced_ip_setting_t* ip_settings)
{
    wiced_result_t result = wiced_network_down(WICED_AP_INTERFACE);
    if (result) return result;

    result = (wiced_result_t) wwd_wifi_start_ap( &creds.SSID, creds.security, (uint8_t*) creds.security_key,
            creds.security_key_length, creds.channel );
    if (result) return result;

    result=wiced_network_up( WICED_AP_INTERFACE, WICED_USE_INTERNAL_DHCP_SERVER, ip_settings);
    if (result) return result;

    return WICED_SUCCESS;
}

/**
 * Indicate if the application wants the AP mode active.
 * If the device is in listening mode, that takes precedence - the system will reconfigure the AP to the application specifications when
 * listening mode exits.
 */
int wlan_ap_enabled(uint8_t enabled, void* reserved)
{
    wiced_result_t result;
    if (enabled)
    {
        wiced_config_soft_ap_t creds;
        result = ap_get_credentials(creds);
        if (result) return result;
        result = wlan_ap_up(creds, &device_init_ip_settings);
    }
    else
    {
        result = wiced_network_down(WICED_AP_INTERFACE);
    }
    return result;
}

#define AP_CREDS_OFFSET (OFFSETOF(platform_dct_wifi_config_t, soft_ap_settings))

/**
 * Fetches the AP credentials from the DCT.
 * After calling this, `unread_ap_credentials()` should be called to free up the memory.
 */
static wiced_result_t read_ap_credentials(wiced_config_soft_ap_t** dct_creds)
{
    return wiced_dct_read_lock((void**) dct_creds, WICED_FALSE, DCT_WIFI_CONFIG_SECTION, AP_CREDS_OFFSET, sizeof(wiced_config_soft_ap_t));
}

/**
 * Cleans up resources from `read_ap_credentials()`.
 */
static wiced_result_t unread_ap_credentials(wiced_config_soft_ap_t* dct_creds)
{
    return wiced_dct_read_unlock( dct_creds, WICED_FALSE );
}

/**
 * Writes ap credentials to the DCT.
 */
static wiced_result_t write_ap_credentials(const wiced_config_soft_ap_t* dct_creds)
{
    return wiced_dct_write(dct_creds, DCT_WIFI_CONFIG_SECTION, AP_CREDS_OFFSET, sizeof(wiced_config_soft_ap_t));
}

/**
 * Sets the AP credentials that are used to conenct to the AP interface.
 * The credentials are stored in the DCT.
 * If the new credentials are the same as the existing ones, no change is made to the flash memory.
 * If SSID is length 0, the default SoftAP SSID is used.
 */
static wiced_result_t set_ap_credentials(const wiced_config_soft_ap_t& creds)
{
    wiced_config_soft_ap_t* dct_creds = nullptr;

    wiced_result_t result = read_ap_credentials(&dct_creds);
    if (result == WICED_SUCCESS)
    {
        // check if there is a change and only write then.
        if (memcmp(&creds, dct_creds, sizeof(creds)))
        {
            unread_ap_credentials(dct_creds);
            result = write_ap_credentials(&creds);
        } else {
            unread_ap_credentials(dct_creds);
        }
    }
    else
    {
        WARN("Error obtaining existing AP credentials: %x", result);
    }
    return result;
}

static void copy_from_wlan_credentials(wiced_config_soft_ap_t& ap_creds, WLanCredentials& wlan_creds)
{
    ap_creds.SSID.length = wlan_creds.ssid_len;
    memcpy(ap_creds.SSID.value, wlan_creds.ssid, wlan_creds.ssid_len);

    ap_creds.security_key_length = wlan_creds.password_len;
    memcpy(ap_creds.security_key, wlan_creds.password, wlan_creds.password_len);

    ap_creds.channel = wlan_creds.channel;

    WLanSecurityType security_type = wlan_creds.security;
    WLanSecurityCipher cipher = wlan_creds.cipher;
    if (security_type==WLAN_SEC_NOT_SET)
        security_type = ap_creds.security_key_length ? WLAN_SEC_WPA2 : WLAN_SEC_UNSEC;

    if (cipher==WLAN_CIPHER_NOT_SET && (security_type==WLAN_SEC_WPA || security_type==WLAN_SEC_WPA2))
        cipher = WLAN_CIPHER_AES;

    ap_creds.security = wlan_to_wiced_security(security_type, cipher);
    ap_creds.details_valid = CONFIG_VALIDITY_VALUE;
}

static void copy_to_wifi_access_point(WiFiAccessPoint& dest, wiced_config_soft_ap_t& source)
{
    memcpy(dest.ssid, source.SSID.value, source.SSID.length);
    dest.ssid[source.SSID.length] = 0;
    dest.ssidLength = source.SSID.length;

    dest.security = wlan_to_security_type(source.security);
    dest.cipher = wlan_to_cipherer_type(source.security);
    dest.channel = source.channel;
    dest.maxDataRate = 0;
}

/**
 * Gets, sets or clears the credentials.
 * When update is false, then the credentials are read into the result parameter (if it is not null.)
 * When update is true, the credentials stored in result are set. If result is null, then the AP credentials are cleared.
 */
int wlan_ap_set_credentials(WLanCredentials* wlan_creds, void* reserved)
{
    wiced_config_soft_ap_t ap_creds;
    memset(&ap_creds, 0, sizeof(ap_creds));
    wiced_result_t result = WICED_SUCCESS;
    if (wlan_creds) {
        copy_from_wlan_credentials(ap_creds, *wlan_creds);
        result = set_ap_credentials(ap_creds);
        INFO("Set AP credentials. SSID='%s'", wlan_creds->ssid);
    }
    else {
        result = set_ap_credentials(ap_creds);
        if (result)
            WARN("Error clearing AP credentials: %x", result);
        else
            INFO("Cleared AP credentials.");
    }
    return result;
}

/**
 * Fetches the credentials from the dct and fills in defaults.
 */
wiced_result_t ap_get_stored_credentials(wiced_config_soft_ap_t& creds)
{
    wiced_config_soft_ap_t* dct_creds;
    wiced_result_t result = read_ap_credentials(&dct_creds);
    if (result == WICED_SUCCESS)
    {
        memcpy(&creds, dct_creds, sizeof(creds));
        unread_ap_credentials(dct_creds);
    }
    return result;
}

/**
 * Fetches the credentials from the dct and fills in defaults.
 */
wiced_result_t ap_get_credentials(wiced_config_soft_ap_t& creds)
{
    wiced_result_t result = ap_get_stored_credentials(creds);
    if (!creds.SSID.length) {
        fetch_or_generate_setup_ssid((device_code_t*)&creds.SSID);
    }
    if (!creds.channel)
        creds.channel = 11;
    return result;
}

/**
 * Retrieves the AP credentials.
 */
int wlan_ap_get_credentials(WiFiAccessPoint* wap, void* reserved)
{
    wiced_config_soft_ap_t creds;
    wiced_result_t result = ap_get_credentials(creds);
    if (result == WICED_SUCCESS)
    {
        copy_to_wifi_access_point(*wap, creds);
        if (wiced_network_is_up(WICED_AP_INTERFACE)) {
            wiced_mac_t mac;
            wwd_wifi_get_mac_address(&mac, WWD_AP_INTERFACE);
            memcpy(wap->bssid, mac.octet, sizeof(wap->bssid));
        }
    }
    return result;
}

/**
 * Determines if the AP credentials have been set.
 */
int wlan_ap_has_credentials(void* reserved)
{
    wiced_config_soft_ap_t* dct_creds = nullptr;
    wiced_result_t result = read_ap_credentials(&dct_creds);
    if (result == WICED_SUCCESS)
    {
        if (dct_creds->details_valid != CONFIG_VALIDITY_VALUE) {
            // Invalid settings
            result = WICED_ERROR;
        }

        unread_ap_credentials(dct_creds);
    }
    return result;
}

int wlan_ap_get_state(uint8_t* state, void* reserved)
{
    *state = wiced_network_is_up(WICED_AP_INTERFACE);
    return 0;
}


/**
 * Begin AP listening mode.
 */
int wlan_ap_listen(void* reserved)
{
    return 0;
}
