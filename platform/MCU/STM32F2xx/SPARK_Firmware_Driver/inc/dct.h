/**
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
#pragma once

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "platform_system_flags.h"
#include "platform_flash_modules.h"
#include "static_assert.h"
#include "stddef.h"     // for offsetof in C
#include "hw_config.h"  // for button_config_t
#include "rgbled_hal_impl.h" // for led_config_t
#include <stdio.h>

#define MAX_MODULES_SLOT    5 //Max modules
#define FAC_RESET_SLOT      0 //Factory reset module index
#define GEN_START_SLOT      1 //Generic module start index

typedef  uint32_t dct_ip_address_v4_t;

typedef struct _static_ip_config_t {
    uint8_t config_mode;            // how the IPv4 address is assigned
    uint8_t padding[3];             // use this for additional flags where possible
    dct_ip_address_v4_t host;       // addresses stored in network order
    dct_ip_address_v4_t netmask;
    dct_ip_address_v4_t gateway;
    dct_ip_address_v4_t dns1;
    dct_ip_address_v4_t dns2;
} static_ip_config_t;

typedef struct {
  uint8_t type;
  uint8_t inner_identity[64];
  uint8_t inner_identity_len;
  uint8_t outer_identity[64];
  uint8_t outer_identity_len;
  uint8_t security_key[64];
  uint8_t security_key_len;
  uint8_t ca_certificate[4*1024];
  uint16_t ca_certificate_len;
  uint8_t padding[2];
} eap_config_t;

STATIC_ASSERT(eap_config_size, sizeof(eap_config_t)==(196 + 4*1024 + 4));
STATIC_ASSERT(static_ip_config_size, sizeof(static_ip_config_t)==24);

#define DCT_SERVER_ADDRESS_SIZE  (128)

/**
 * Custom extensions to the DCT data.
 */
typedef struct __attribute__((packed)) application_dct {
    platform_system_flags_t system_flags;
    uint16_t version;
    uint8_t device_private_key[1216];   // sufficient for 2048 bits
    uint8_t device_public_key[384];     // sufficient for 2048 bits
    static_ip_config_t  ip_config;
    uint8_t unused[95];                 //
    uint8_t ota_update_flag;            // should be 0xA5 to trigger an update from data stored in the update region
    uint32_t feature_flags[1];          // Configurable feature flags (see HAL_Feature_Set()). Default uninitialized value is 0xffffffff
    uint8_t country_code[4];            // WICED country code. Stored as bit-endian format: CH1/CH2/0/rev (max 255)
    uint8_t claim_code[63];             // claim code. no terminating null.
    uint8_t claimed[1];                 // 0,0xFF, not claimed. 1 claimed.
    uint8_t ssid_prefix[26];            // SSID prefix (25 chars max). First byte is length.
    uint8_t device_code[6];             // 6 suffix characters (not null terminated))
    uint8_t version_string[32];         // version string including date
    uint8_t dns_resolve[128];           // DNS names to resolve.
    uint8_t reserved1[64];
    uint8_t server_public_key[768];     // 4096 bits
    uint8_t padding[2];                 // align to 4 byte boundary
    platform_flash_modules_t flash_modules[MAX_MODULES_SLOT]; //100 bytes
    uint16_t product_store[12];
    uint8_t antenna_selection;           // 0xFF is uninitialized
    uint8_t cloud_transport;             // 0xFF is uninitialized meaning platform default (TCP for Photon, UDP for Electron). 0 is TCP on Electron.
    uint8_t alt_device_public_key[128];  // alternative device public key
    uint8_t alt_device_private_key[192]; // alternative device private key
    uint8_t alt_server_public_key[192];
    uint8_t alt_server_address[DCT_SERVER_ADDRESS_SIZE]; // server address info
    uint8_t device_id[12];                               // the STM32 device ID
    uint8_t radio_flags;                 // xxxxxx10 means disable the wifi powersave testmode signal on P1. Any other values in the lower 2 bits means enabled.
    button_config_t mode_button_mirror;  // SETUP/MODE button mirror pin, to be used by bootloader
    led_config_t led_mirror[4];          // LED mirroring configuration, to be used by bootloader
    uint8_t led_theme[64];               // LED signaling theme
    eap_config_t eap_config;             // WLAN EAP settings
    uint8_t reserved2[272];
    // safe to add more data here or use up some of the reserved space to keep the end where it is
    uint8_t end[0];
} application_dct_t;

#define DCT_SYSTEM_FLAGS_OFFSET  (offsetof(application_dct_t, system_flags))
#define DCT_DEVICE_PRIVATE_KEY_OFFSET (offsetof(application_dct_t, device_private_key))
#define DCT_DEVICE_PUBLIC_KEY_OFFSET (offsetof(application_dct_t, device_public_key))
#define DCT_SERVER_PUBLIC_KEY_OFFSET (offsetof(application_dct_t, server_public_key))
#define DCT_SERVER_ADDRESS_OFFSET ((DCT_SERVER_PUBLIC_KEY_OFFSET)+384)
#define DCT_IP_CONFIG_OFFSET (offsetof(application_dct_t, ip_config))
#define DCT_OTA_UPDATE_FLAG_OFFSET (offsetof(application_dct_t, ota_update_flag))
#define DCT_FEATURE_FLAGS_OFFSET (offsetof(application_dct_t, feature_flags))
#define DCT_COUNTRY_CODE_OFFSET (offsetof(application_dct_t, country_code))
#define DCT_CLAIM_CODE_OFFSET (offsetof(application_dct_t, claim_code))
#define DCT_SSID_PREFIX_OFFSET (offsetof(application_dct_t, ssid_prefix))
#define DCT_DNS_RESOLVE_OFFSET (offsetof(application_dct_t, dns_resolve))
#define DCT_DEVICE_CODE_OFFSET (offsetof(application_dct_t, device_code))
#define DCT_DEVICE_CLAIMED_OFFSET (offsetof(application_dct_t, claimed))
#define DCT_FLASH_MODULES_OFFSET (offsetof(application_dct_t, flash_modules))
#define DCT_PRODUCT_STORE_OFFSET (offsetof(application_dct_t, product_store))
#define DCT_ANTENNA_SELECTION_OFFSET (offsetof(application_dct_t, antenna_selection))
#define DCT_CLOUD_TRANSPORT_OFFSET (offsetof(application_dct_t, cloud_transport))
#define DCT_ALT_DEVICE_PUBLIC_KEY_OFFSET (offsetof(application_dct_t, alt_device_public_key))
#define DCT_ALT_DEVICE_PRIVATE_KEY_OFFSET (offsetof(application_dct_t, alt_device_private_key))
#define DCT_ALT_SERVER_PUBLIC_KEY_OFFSET (offsetof(application_dct_t, alt_server_public_key))
#define DCT_ALT_SERVER_ADDRESS_OFFSET (offsetof(application_dct_t, alt_server_address))
#define DCT_DEVICE_ID_OFFSET (offsetof(application_dct_t, device_id))
#define DCT_RADIO_FLAGS_OFFSET (offsetof(application_dct_t, radio_flags))
#define DCT_MODE_BUTTON_MIRROR_OFFSET (offsetof(application_dct_t, mode_button_mirror))
#define DCT_LED_MIRROR_OFFSET (offsetof(application_dct_t, led_mirror))
#define DCT_LED_THEME_OFFSET (offsetof(application_dct_t, led_theme))
#define DCT_EAP_CONFIG_OFFSET (offsetof(application_dct_t, eap_config))

#define DCT_SYSTEM_FLAGS_SIZE  (sizeof(application_dct_t::system_flags))
#define DCT_DEVICE_PRIVATE_KEY_SIZE  (sizeof(application_dct_t::device_private_key))
#define DCT_DEVICE_PUBLIC_KEY_SIZE  (sizeof(application_dct_t::device_public_key))
#define DCT_SERVER_PUBLIC_KEY_SIZE  (sizeof(application_dct_t::server_public_key))
#define DCT_IP_CONFIG_SIZE (sizeof(application_dct_t::ip_config))
#define DCT_OTA_UPDATE_FLAG_SIZE  (sizeof(application_dct_t::ota_update_flag))
#define DCT_FEATURE_FLAGS_SIZE  (sizeof(application_dct_t::feature_flags))
#define DCT_COUNTRY_CODE_SIZE  (sizeof(application_dct_t::country_code))
#define DCT_CLAIM_CODE_SIZE  (sizeof(application_dct_t::claim_code))
#define DCT_SSID_PREFIX_SIZE  (sizeof(application_dct_t::ssid_prefix))
#define DCT_DNS_RESOLVE_SIZE  (sizeof(application_dct_t::dns_resolve))
#define DCT_DEVICE_CODE_SIZE  (sizeof(application_dct_t::device_code))
#define DCT_DEVICE_CLAIMED_SIZE  (sizeof(application_dct_t::claimed))
#define DCT_FLASH_MODULES_SIZE  (sizeof(application_dct_t::flash_modules))
#define DCT_PRODUCT_STORE_SIZE  (sizeof(application_dct_t::product_store))
#define DCT_ANTENNA_SELECTION_SIZE  (sizeof(application_dct_t::antenna_selection))
#define DCT_CLOUD_TRANSPORT_SIZE  (sizeof(application_dct_t::cloud_transport))
#define DCT_ALT_DEVICE_PUBLIC_KEY_SIZE  (sizeof(application_dct_t::alt_device_public_key))
#define DCT_ALT_DEVICE_PRIVATE_KEY_SIZE  (sizeof(application_dct_t::alt_device_private_key))
#define DCT_ALT_SERVER_PUBLIC_KEY_SIZE  (sizeof(application_dct_t::alt_server_public_key))
#define DCT_ALT_SERVER_ADDRESS_SIZE  (sizeof(application_dct_t::alt_server_address))
#define DCT_DEVICE_ID_SIZE  (sizeof(application_dct_t::device_id))
#define DCT_RADIO_FLAGS_SIZE  (sizeof(application_dct_t::radio_flags))
#define DCT_MODE_BUTTON_MIRROR_SIZE (sizeof(application_dct_t::mode_button_mirror))
#define DCT_LED_MIRROR_SIZE (sizeof(application_dct_t::led_mirror))
#define DCT_LED_THEME_SIZE (sizeof(application_dct_t::led_theme))
#define DCT_EAP_CONFIG_SIZE (sizeof(application_dct_t::eap_config))

#define STATIC_ASSERT_DCT_OFFSET(field, expected) STATIC_ASSERT( dct_##field, offsetof(application_dct_t, field)==expected)
#define STATIC_ASSERT_FLAGS_OFFSET(field, expected) STATIC_ASSERT( dct_sysflag_##field, offsetof(platform_system_flags_t, field)==expected)

/**
 * Assert offsets. These ensure that the layout in flash isn't inadvertently changed.
 */
STATIC_ASSERT_DCT_OFFSET(system_flags, 0);
STATIC_ASSERT_DCT_OFFSET(version, 32);
STATIC_ASSERT_DCT_OFFSET(device_private_key, 34);
STATIC_ASSERT_DCT_OFFSET(device_public_key, 1250 /*34+1216*/);
STATIC_ASSERT_DCT_OFFSET(ip_config, 1634 /* 1250 + 384 */);
STATIC_ASSERT_DCT_OFFSET(ota_update_flag, 1753);
STATIC_ASSERT_DCT_OFFSET(feature_flags, 1754 /* 1634 + 120 */);
STATIC_ASSERT_DCT_OFFSET(country_code, 1758 /* 1754 + 4 */);
STATIC_ASSERT_DCT_OFFSET(claim_code, 1762 /* 1758 + 4 */);
STATIC_ASSERT_DCT_OFFSET(claimed, 1825 /* 1762 + 63 */ );
STATIC_ASSERT_DCT_OFFSET(ssid_prefix, 1826 /* 1825 + 1 */);
STATIC_ASSERT_DCT_OFFSET(device_code, 1852 /* 1826 + 26 */);
STATIC_ASSERT_DCT_OFFSET(version_string, 1858 /* 1852 + 6 */);
STATIC_ASSERT_DCT_OFFSET(dns_resolve, 1890 /* 1868 + 32 */);
STATIC_ASSERT_DCT_OFFSET(reserved1, 2018 /* 1890 + 128 */);
STATIC_ASSERT_DCT_OFFSET(server_public_key, 2082 /* 2018 + 64 */);
STATIC_ASSERT_DCT_OFFSET(padding, 2850 /* 2082 + 768 */);
STATIC_ASSERT_DCT_OFFSET(flash_modules, 2852 /* 2850 + 2 */);
STATIC_ASSERT_DCT_OFFSET(product_store, 2952 /* 2852 + 100 */);
STATIC_ASSERT_DCT_OFFSET(antenna_selection, 2976 /* 2952 + 24 */);
STATIC_ASSERT_DCT_OFFSET(cloud_transport, 2977 /* 2976 + 1 */);
STATIC_ASSERT_DCT_OFFSET(alt_device_public_key, 2978 /* 2977 + 1 */);
STATIC_ASSERT_DCT_OFFSET(alt_device_private_key, 3106 /* 2978 + 128 */);
STATIC_ASSERT_DCT_OFFSET(alt_server_public_key, 3298 /* 3106 + 192 */);
STATIC_ASSERT_DCT_OFFSET(alt_server_address, 3490 /* 3298 + 192 */);
STATIC_ASSERT_DCT_OFFSET(device_id, 3618 /* 3490 + 128 */);
STATIC_ASSERT_DCT_OFFSET(radio_flags, 3630 /* 3618 + 12 */);
STATIC_ASSERT_DCT_OFFSET(mode_button_mirror, 3631 /* 3630 + 1 */);
STATIC_ASSERT_DCT_OFFSET(led_mirror, 3663 /* 3631 + 32 */);
STATIC_ASSERT_DCT_OFFSET(led_theme, 3759 /* 3663 + 24 * 4 */);
STATIC_ASSERT_DCT_OFFSET(eap_config, 3823 /* 3759 + 64 */);
STATIC_ASSERT_DCT_OFFSET(reserved2, 8119 /* 3823 + (196 + 4*1024 + 4) */);
STATIC_ASSERT_DCT_OFFSET(end, 8391 /* 8119 + 272 */);

STATIC_ASSERT_FLAGS_OFFSET(Bootloader_Version_SysFlag, 4);
STATIC_ASSERT_FLAGS_OFFSET(NVMEM_SPARK_Reset_SysFlag, 6);
STATIC_ASSERT_FLAGS_OFFSET(FLASH_OTA_Update_SysFlag, 8);
STATIC_ASSERT_FLAGS_OFFSET(OTA_FLASHED_Status_SysFlag, 10);
STATIC_ASSERT_FLAGS_OFFSET(Factory_Reset_SysFlag, 12);
STATIC_ASSERT_FLAGS_OFFSET(IWDG_Enable_SysFlag, 14);
STATIC_ASSERT_FLAGS_OFFSET(dfu_on_no_firmware, 16);
STATIC_ASSERT_FLAGS_OFFSET(Factory_Reset_Done_SysFlag, 17);
STATIC_ASSERT_FLAGS_OFFSET(StartupMode_SysFlag, 18);
STATIC_ASSERT_FLAGS_OFFSET(FeaturesEnabled_SysFlag, 19);
STATIC_ASSERT_FLAGS_OFFSET(RCC_CSR_SysFlag, 20);
STATIC_ASSERT_FLAGS_OFFSET(reserved, 24);

#define DCT_OTA_UPDATE_FLAG_SET (0xA5)
#define DCT_OTA_UPDATE_FLAG_CLEAR (0XFF)


// Note: This function is deprecated, use dct_read_app_data_copy() or dct_read_app_data_lock() instead
const void* dct_read_app_data(uint32_t offset);

int dct_read_app_data_copy(uint32_t offset, void* ptr, size_t size);
const void* dct_read_app_data_lock(uint32_t offset);
int dct_read_app_data_unlock(uint32_t offset);

int dct_write_app_data( const void* data, uint32_t offset, uint32_t size );


#ifdef	__cplusplus
}
#endif


