/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#pragma once

#include "besl_structures.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define ETHERNET_ADDRESS_LENGTH    (6)

#define WPS_OUI         "\x00\x50\xF2"  /* WPS OUI */
#define WPS_OUI_TYPE        4

/* Data Element Definitions */
#define WPS_ID_AP_CHANNEL         0x1001
#define WPS_ID_ASSOC_STATE        0x1002
#define WPS_ID_AUTH_TYPE          0x1003
#define WPS_ID_AUTH_TYPE_FLAGS    0x1004
#define WPS_ID_AUTHENTICATOR      0x1005
#define WPS_ID_CONFIG_METHODS     0x1008
#define WPS_ID_CONFIG_ERROR       0x1009
#define WPS_ID_CONF_URL4          0x100A
#define WPS_ID_CONF_URL6          0x100B
#define WPS_ID_CONN_TYPE          0x100C
#define WPS_ID_CONN_TYPE_FLAGS    0x100D
#define WPS_ID_CREDENTIAL         0x100E
#define WPS_ID_DEVICE_NAME        0x1011
#define WPS_ID_DEVICE_PWD_ID      0x1012
#define WPS_ID_E_HASH1            0x1014
#define WPS_ID_E_HASH2            0x1015
#define WPS_ID_E_SNONCE1          0x1016
#define WPS_ID_E_SNONCE2          0x1017
#define WPS_ID_ENCR_SETTINGS      0x1018
#define WPS_ID_ENCR_TYPE          0x100F
#define WPS_ID_ENCR_TYPE_FLAGS    0x1010
#define WPS_ID_ENROLLEE_NONCE     0x101A
#define WPS_ID_FEATURE_ID         0x101B
#define WPS_ID_IDENTITY           0x101C
#define WPS_ID_IDENTITY_PROOF     0x101D
#define WPS_ID_KEY_WRAP_AUTH      0x101E
#define WPS_ID_KEY_IDENTIFIER     0x101F
#define WPS_ID_MAC_ADDR           0x1020
#define WPS_ID_MANUFACTURER       0x1021
#define WPS_ID_MSG_TYPE           0x1022
#define WPS_ID_MODEL_NAME         0x1023
#define WPS_ID_MODEL_NUMBER       0x1024
#define WPS_ID_NW_INDEX           0x1026
#define WPS_ID_NW_KEY             0x1027
#define WPS_ID_NW_KEY_INDEX       0x1028
#define WPS_ID_NEW_DEVICE_NAME    0x1029
#define WPS_ID_NEW_PWD            0x102A
#define WPS_ID_OOB_DEV_PWD        0x102C
#define WPS_ID_OS_VERSION         0x102D
#define WPS_ID_POWER_LEVEL        0x102F
#define WPS_ID_PSK_CURRENT        0x1030
#define WPS_ID_PSK_MAX            0x1031
#define WPS_ID_PUBLIC_KEY         0x1032
#define WPS_ID_RADIO_ENABLED      0x1033
#define WPS_ID_REBOOT             0x1034
#define WPS_ID_REGISTRAR_CURRENT  0x1035
#define WPS_ID_REGISTRAR_ESTBLSHD 0x1036
#define WPS_ID_REGISTRAR_LIST     0x1037
#define WPS_ID_REGISTRAR_MAX      0x1038
#define WPS_ID_REGISTRAR_NONCE    0x1039
#define WPS_ID_REQ_TYPE           0x103A
#define WPS_ID_RESP_TYPE          0x103B
#define WPS_ID_RF_BAND            0x103C
#define WPS_ID_R_HASH1            0x103D
#define WPS_ID_R_HASH2            0x103E
#define WPS_ID_R_SNONCE1          0x103F
#define WPS_ID_R_SNONCE2          0x1040
#define WPS_ID_SEL_REGISTRAR      0x1041
#define WPS_ID_SERIAL_NUM         0x1042
#define WPS_ID_SC_STATE           0x1044
#define WPS_ID_SSID               0x1045
#define WPS_ID_TOT_NETWORKS       0x1046
#define WPS_ID_UUID_E             0x1047
#define WPS_ID_UUID_R             0x1048
#define WPS_ID_VENDOR_EXT         0x1049
#define WPS_ID_VERSION            0x104A
#define WPS_ID_X509_CERT_REQ      0x104B
#define WPS_ID_X509_CERT          0x104C
#define WPS_ID_EAP_IDENTITY       0x104D
#define WPS_ID_MSG_COUNTER        0x104E
#define WPS_ID_PUBKEY_HASH        0x104F
#define WPS_ID_REKEY_KEY          0x1050
#define WPS_ID_KEY_LIFETIME       0x1051
#define WPS_ID_PERM_CFG_METHODS   0x1052
#define WPS_ID_SEL_REG_CFG_METHODS 0x1053
#define WPS_ID_PRIM_DEV_TYPE      0x1054
#define WPS_ID_SEC_DEV_TYPE_LIST  0x1055
#define WPS_ID_PORTABLE_DEVICE    0x1056
#define WPS_ID_AP_SETUP_LOCKED    0x1057
#define WPS_ID_APP_LIST           0x1058
#define WPS_ID_EAP_TYPE           0x1059
#define WPS_ID_INIT_VECTOR        0x1060
#define WPS_ID_KEY_PROVIDED_AUTO  0x1061
#define WPS_ID_8021X_ENABLED      0x1062
#define WPS_ID_WEP_TRANSMIT_KEY   0x1064
#define WPS_ID_REQ_DEV_TYPE       0x106A

/* Fixed size elements sizes */
#define WPS_ID_AP_CHANNEL_S             2
#define WPS_ID_ASSOC_STATE_S            2
#define WPS_ID_AUTH_TYPE_S              2
#define WPS_ID_AUTH_TYPE_FLAGS_S        2
#define WPS_ID_AUTHENTICATOR_S          8
#define WPS_ID_CONFIG_METHODS_S         2
#define WPS_ID_CONFIG_ERROR_S           2
#define WPS_ID_CONN_TYPE_S              1
#define WPS_ID_CONN_TYPE_FLAGS_S        1
#define WPS_ID_DEVICE_PWD_ID_S          2
#define WPS_ID_ENCR_TYPE_S              2
#define WPS_ID_ENCR_TYPE_FLAGS_S        2
#define WPS_ID_FEATURE_ID_S             4
#define WPS_ID_MAC_ADDR_S               6
#define WPS_ID_MSG_TYPE_S               1
#define WPS_ID_SC_STATE_S               1
#define WPS_ID_RF_BAND_S                1
#define WPS_ID_OS_VERSION_S             4
#define WPS_ID_VERSION_S                1
#define WPS_ID_SEL_REGISTRAR_S          1
#define WPS_ID_SEL_REG_CFG_METHODS_S    2
#define WPS_ID_REQ_TYPE_S               1
#define WPS_ID_RESP_TYPE_S              1
#define WPS_ID_AP_SETUP_LOCKED_S        1
#define WPS_ID_PRIM_DEV_TYPE_S          8
#define WPS_ID_UUID_S                   16
#define WPS_ID_PUBLIC_KEY_S             192

/* WPS Message Types */
#define WPS_ID_BEACON            0x01
#define WPS_ID_PROBE_REQ         0x02
#define WPS_ID_PROBE_RESP        0x03
#define WPS_ID_MESSAGE_M1        0x04
#define WPS_ID_MESSAGE_M2        0x05
#define WPS_ID_MESSAGE_M2D       0x06
#define WPS_ID_MESSAGE_M3        0x07
#define WPS_ID_MESSAGE_M4        0x08
#define WPS_ID_MESSAGE_M5        0x09
#define WPS_ID_MESSAGE_M6        0x0A
#define WPS_ID_MESSAGE_M7        0x0B
#define WPS_ID_MESSAGE_M8        0x0C
#define WPS_ID_MESSAGE_ACK       0x0D
#define WPS_ID_MESSAGE_NACK      0x0E
#define WPS_ID_MESSAGE_DONE      0x0F
/* WSP private ID for local use */
#define WPS_PRIVATE_ID_FRAG_ACK     (WPS_ID_MESSAGE_ACK | 0x80)

/* WSC 2.0, WFA Vendor Extension Subelements */
#define WFA_VENDOR_EXT_ID                 "\x00\x37\x2A"
#define WPS_WFA_SUBID_VERSION2            0x00
#define WPS_WFA_SUBID_AUTHORIZED_MACS     0x01
#define WPS_WFA_SUBID_NW_KEY_SHAREABLE    0x02
#define WPS_WFA_SUBID_REQ_TO_ENROLL       0x03
#define WPS_WFA_SUBID_SETTINGS_DELAY_TIME 0x04

/* Device request/response type */
#define WPS_MSGTYPE_ENROLLEE_INFO_ONLY    0x00
#define WPS_MSGTYPE_ENROLLEE_OPEN_8021X   0x01
#define WPS_MSGTYPE_REGISTRAR             0x02
#define WPS_MSGTYPE_AP_WLAN_MGR           0x03

/* RF Band */
#define WPS_RFBAND_24GHZ    0x01
#define WPS_RFBAND_50GHZ    0x02

/* Simple Config state */
#define WPS_SCSTATE_UNCONFIGURED    0x01
#define WPS_SCSTATE_CONFIGURED      0x02


/* Device Type sub categories for primary and secondary device types */
#define WPS_DEVICE_TYPE_SUB_CAT_COMP_PC         1
#define WPS_DEVICE_TYPE_SUB_CAT_COMP_SERVER     2
#define WPS_DEVICE_TYPE_SUB_CAT_COMP_MEDIA_CTR  3
#define WPS_DEVICE_TYPE_SUB_CAT_COMP_UM_PC      4   /* WSC 2.0 */
#define WPS_DEVICE_TYPE_SUB_CAT_COMP_NOTEBOOK   5   /* WSC 2.0 */
#define WPS_DEVICE_TYPE_SUB_CAT_COMP_DESKTOP    6   /* WSC 2.0 */
#define WPS_DEVICE_TYPE_SUB_CAT_COMP_MID        7   /* WSC 2.0 */
#define WPS_DEVICE_TYPE_SUB_CAT_COMP_NETBOOK    8   /* WSC 2.0 */
#define WPS_DEVICE_TYPE_SUB_CAT_INP_Keyboard    1   /* WSC 2.0 */
#define WPS_DEVICE_TYPE_SUB_CAT_INP_MOUSE       2   /* WSC 2.0 */
#define WPS_DEVICE_TYPE_SUB_CAT_INP_JOYSTICK    3   /* WSC 2.0 */
#define WPS_DEVICE_TYPE_SUB_CAT_INP_TRACKBALL   4   /* WSC 2.0 */
#define WPS_DEVICE_TYPE_SUB_CAT_INP_GAM_CTRL    5   /* WSC 2.0 */
#define WPS_DEVICE_TYPE_SUB_CAT_INP_REMOTE      6   /* WSC 2.0 */
#define WPS_DEVICE_TYPE_SUB_CAT_INP_TOUCHSCREEN 7   /* WSC 2.0 */
#define WPS_DEVICE_TYPE_SUB_CAT_INP_BIO_READER  8   /* WSC 2.0 */
#define WPS_DEVICE_TYPE_SUB_CAT_INP_BAR_READER  9   /* WSC 2.0 */
#define WPS_DEVICE_TYPE_SUB_CAT_PRTR_PRINTER    1
#define WPS_DEVICE_TYPE_SUB_CAT_PRTR_SCANNER    2
#define WPS_DEVICE_TYPE_SUB_CAT_PRTR_FAX        3   /* WSC 2.0 */
#define WPS_DEVICE_TYPE_SUB_CAT_PRTR_COPIER     4   /* WSC 2.0 */
#define WPS_DEVICE_TYPE_SUB_CAT_PRTR_ALLINONE   5   /* WSC 2.0 */
#define WPS_DEVICE_TYPE_SUB_CAT_CAM_DGTL_STILL  1
#define WPS_DEVICE_TYPE_SUB_CAT_CAM_VIDEO_CAM   2   /* WSC 2.0 */
#define WPS_DEVICE_TYPE_SUB_CAT_CAM_WEB_CAM     3   /* WSC 2.0 */
#define WPS_DEVICE_TYPE_SUB_CAT_CAM_SECU_CAM    4   /* WSC 2.0 */
#define WPS_DEVICE_TYPE_SUB_CAT_STOR_NAS        1
#define WPS_DEVICE_TYPE_SUB_CAT_NW_AP           1
#define WPS_DEVICE_TYPE_SUB_CAT_NW_ROUTER       2
#define WPS_DEVICE_TYPE_SUB_CAT_NW_SWITCH       3
#define WPS_DEVICE_TYPE_SUB_CAT_NW_Gateway      4   /* WSC 2.0 */
#define WPS_DEVICE_TYPE_SUB_CAT_NW_BRIDGE       5   /* WSC 2.0 */
#define WPS_DEVICE_TYPE_SUB_CAT_DISP_TV         1
#define WPS_DEVICE_TYPE_SUB_CAT_DISP_PIC_FRAME  2
#define WPS_DEVICE_TYPE_SUB_CAT_DISP_PROJECTOR  3
#define WPS_DEVICE_TYPE_SUB_CAT_DISP_MONITOR    4   /* WSC 2.0 */
#define WPS_DEVICE_TYPE_SUB_CAT_MM_DAR          1
#define WPS_DEVICE_TYPE_SUB_CAT_MM_PVR          2
#define WPS_DEVICE_TYPE_SUB_CAT_MM_MCX          3
#define WPS_DEVICE_TYPE_SUB_CAT_MM_STB          4   /* WSC 2.0 */
#define WPS_DEVICE_TYPE_SUB_CAT_MM_MS_ME        5   /* WSC 2.0 */
#define WPS_DEVICE_TYPE_SUB_CAT_MM_PVP          6   /* WSC 2.0 */
#define WPS_DEVICE_TYPE_SUB_CAT_GAM_XBOX        1
#define WPS_DEVICE_TYPE_SUB_CAT_GAM_XBOX_360    2
#define WPS_DEVICE_TYPE_SUB_CAT_GAM_PS          3
#define WPS_DEVICE_TYPE_SUB_CAT_GAM_GC          4   /* WSC 2.0 */
#define WPS_DEVICE_TYPE_SUB_CAT_GAM_PGD         5   /* WSC 2.0 */
#define WPS_DEVICE_TYPE_SUB_CAT_PHONE_WM        1
#define WPS_DEVICE_TYPE_SUB_CAT_PHONE_PSM       2   /* WSC 2.0 */
#define WPS_DEVICE_TYPE_SUB_CAT_PHONE_PDM       3   /* WSC 2.0 */
#define WPS_DEVICE_TYPE_SUB_CAT_PHONE_SSM       4   /* WSC 2.0 */
#define WPS_DEVICE_TYPE_SUB_CAT_PHONE_SDM       5   /* WSC 2.0 */
#define WPS_DEVICE_TYPE_SUB_CAT_AUDIO_TUNER     1   /* WSC 2.0 */
#define WPS_DEVICE_TYPE_SUB_CAT_AUDIO_SPEAKERS  2   /* WSC 2.0 */
#define WPS_DEVICE_TYPE_SUB_CAT_AUDIO_PMP       3   /* WSC 2.0 */
#define WPS_DEVICE_TYPE_SUB_CAT_AUDIO_HEADSET   4   /* WSC 2.0 */
#define WPS_DEVICE_TYPE_SUB_CAT_AUDIO_HPHONE    5   /* WSC 2.0 */
#define WPS_DEVICE_TYPE_SUB_CAT_AUDIO_MPHONE    6   /* WSC 2.0 */
#define WPS_DEVICE_TYPE_SUB_CAT_AUDIO_HTS       7   /* WSC 2.0 */

/* WSC 2.0, WFA Vendor Extension Subelements */
#define WPS_WFA_SUBID_VERSION2_S            1
#define WPS_WFA_SUBID_NW_KEY_SHAREABLE_S    1
#define WPS_WFA_SUBID_REQ_TO_ENROLL_S       1
#define WPS_WFA_SUBID_SETTINGS_DELAY_TIME_S 1

/* Association states */
#define WPS_ASSOC_NOT_ASSOCIATED  0
#define WPS_ASSOC_CONN_SUCCESS    1
#define WPS_ASSOC_CONFIG_FAIL     2
#define WPS_ASSOC_ASSOC_FAIL      3
#define WPS_ASSOC_IP_FAIL         4

/* Authentication types */
#define WPS_AUTHTYPE_OPEN        0x0001
#define WPS_AUTHTYPE_WPAPSK      0x0002 /* Deprecated in WSC 2.0 */
#define WPS_AUTHTYPE_SHARED      0x0004 /* Deprecated in WSC 2.0 */
#define WPS_AUTHTYPE_WPA         0x0008 /* Deprecated in WSC 2.0 */
#define WPS_AUTHTYPE_WPA2        0x0010
#define WPS_AUTHTYPE_WPA2PSK     0x0020

/* Config methods */
#define WPS_CONFMET_USBA            0x0001  /* Deprecated in WSC 2.0 */
#define WPS_CONFMET_ETHERNET        0x0002  /* Deprecated in WSC 2.0 */
#define WPS_CONFMET_LABEL           0x0004
#define WPS_CONFMET_DISPLAY         0x0008
#define WPS_CONFMET_EXT_NFC_TOK     0x0010
#define WPS_CONFMET_INT_NFC_TOK     0x0020
#define WPS_CONFMET_NFC_INTF        0x0040
#define WPS_CONFMET_PBC             0x0080
#define WPS_CONFMET_KEYPAD          0x0100
/* WSC 2.0 */
#define WPS_CONFMET_VIRT_PBC        0x0280
#define WPS_CONFMET_PHY_PBC         0x0480
#define WPS_CONFMET_VIRT_DISPLAY    0x2008
#define WPS_CONFMET_PHY_DISPLAY     0x4008

/* Connection types */
#define WPS_CONNTYPE_ESS    0x01
#define WPS_CONNTYPE_IBSS   0x02

/* WSC 2.0, deprecated and set to hardcode 0x10 for backwords compatibility resons */
#define WPS_VERSION                0x10 /* Do not change it anymore */

/* WSC 2.0 */
#define WPS_VERSION2               0x20

/* Size constants */
#define SIZE_SSID_LENGTH       33
#define WPS_UUID_LENGTH        16

#define SIZE_64_BITS        8
#define SIZE_128_BITS       16
#define SIZE_160_BITS       20
#define SIZE_256_BITS       32
#define SIZE_1536_BITS      192

#define SIZE_ETHERNET_ADDRESS    6

#define PRF_DIGEST_SIZE         SIZE_256_BITS

#define WPS_PADDED_AES_ROUND_UP(x,y) ((x) + (y)-((x)%(y)))

/* OUI constants */
#define WIFI_ALLIANCE_OUI       0x0050F204

/* String constants */
#define REGISTRAR_ID_STRING         "WFA-SimpleConfig-Registrar-1-0"
#define ENROLLEE_ID_STRING          "WFA-SimpleConfig-Enrollee-1-0"
#define KDK_PERSONALIZATION_STRING  (char*) "Wi-Fi Easy and Secure Key Derivation"

/* Timeout and delay constants */
#define WPS_EAPOL_PACKET_TIMEOUT       (5000) /* 5 second timeout when waiting for WPS EAPOL packets, except M2 */
#define WPS_PUBLIC_KEY_MESSAGE_TIMEOUT (8000) /* 8 second timeout for the enrollee when it has sent M1 and is waiting for the registrar's public key in M2 */
#define WPS_PBC_MODE_ASSERTION_DELAY   (5000) /* 5 second delay to account for enrollees that keep asserting PBC mode for some time after the handshake has completed */
#define WPS_PBC_NOTIFICATION_DELAY     (3000) /* 3 second delay between notifications sent to the application layer that an enrollee is trying to join using PBC mode */

/******************************************************
 *                   Enumerations
 ******************************************************/

/* WPS Message types */
typedef enum
{
    WPS_MESSAGE_TYPE_START    = 0x01,
    WPS_MESSAGE_TYPE_ACK      = 0x02,
    WPS_MESSAGE_TYPE_NACK     = 0x03,
    WPS_MESSAGE_TYPE_MSG      = 0x04,
    WPS_MESSAGE_TYPE_DONE     = 0x05,
    WPS_MESSAGE_TYPE_FRAG_ACK = 0x06,
} wps_message_type_t;

/* Device Type categories for primary and secondary device types */
typedef enum
{
    WPS_DEVICE_TYPE_CAT_COMPUTER      = 1,
    WPS_DEVICE_TYPE_CAT_INPUT_DEVICE  = 2,
    WPS_DEVICE_TYPE_CAT_PRINTER       = 3,
    WPS_DEVICE_TYPE_CAT_CAMERA        = 4,
    WPS_DEVICE_TYPE_CAT_STORAGE       = 5,
    WPS_DEVICE_TYPE_CAT_NW_INFRA      = 6,
    WPS_DEVICE_TYPE_CAT_DISPLAYS      = 7,
    WPS_DEVICE_TYPE_CAT_MM_DEVICES    = 8,
    WPS_DEVICE_TYPE_CAT_GAME_DEVICES  = 9,
    WPS_DEVICE_TYPE_CAT_TELEPHONE     = 10,
    WPS_DEVICE_TYPE_CAT_AUDIO_DEVICES = 11, /* WSC 2.0 */
} wps_device_category_t;


typedef enum
{
    WPS_NO_MODE  = 0,
    WPS_PBC_MODE = 1,
    WPS_PIN_MODE = 2,
    WPS_NFC_MODE = 3,
} wps_mode_t;


typedef enum
{
    WPS_BESL_RESULT_LIST(      WPS_     )
} wps_result_t;

typedef enum
{
    PRIMARY_DEVICE_COMPUTER               = 1,
    PRIMARY_DEVICE_INPUT                  = 2,
    PRIMARY_DEVICE_PRINT_SCAN_FAX_COPY    = 3,
    PRIMARY_DEVICE_CAMERA                 = 4,
    PRIMARY_DEVICE_STORAGE                = 5,
    PRIMARY_DEVICE_NETWORK_INFRASTRUCTURE = 6,
    PRIMARY_DEVICE_DISPLAY                = 7,
    PRIMARY_DEVICE_MULTIMEDIA             = 8,
    PRIMARY_DEVICE_GAMING                 = 9,
    PRIMARY_DEVICE_TELEPHONE              = 10,
    PRIMARY_DEVICE_AUDIO                  = 11,
    PRIMARY_DEVICE_OTHER                  = 0xFF,
} primary_device_type_category_t;

typedef enum
{
    USBA                  = 0x0001,
    ETHERNET              = 0x0002,
    LABEL                 = 0x0004,
    DISPLAY               = 0x0008,
    EXTERNAL_NFC_TOKEN    = 0x0010,
    INTEGRATED_NFC_TOKEN  = 0x0020,
    NFC_INTERFACE         = 0x0040,
    PUSH_BUTTON           = 0x0080,
    KEYPAD                = 0x0100,
    VIRTUAL_PUSH_BUTTON   = 0x0280,
    PHYSICAL_PUSH_BUTTON  = 0x0480,
    VIRTUAL_DISPLAY_PIN   = 0x2008,
    PHYSICAL_DISPLAY_PIN  = 0x4008
} wps_configuration_method_t;

typedef enum
{
    WPS_ENROLLEE_INFO_ONLY      = 0x00,
    WPS_ENROLLEE_OPEN_8021X     = 0x01,
    WPS_REGISTRAR               = 0x02,
    WPS_WLAN_MANAGER_REGISTRAR  = 0x03
} wps_request_type_t;

/* Device password ID */
typedef enum
{
    WPS_DEFAULT_DEVICEPWDID       = 0x0000,
    WPS_USER_SPEC_DEVICEPWDID     = 0x0001,
    WPS_MACHINE_SPEC_DEVICEPWDID  = 0x0002,
    WPS_REKEY_DEVICEPWDID         = 0x0003,
    WPS_PUSH_BTN_DEVICEPWDID      = 0x0004,
    WPS_DEVICEPWDID_REG_SPEC      = 0x0005,
} wps_device_password_id_t;

/* WPS encryption types */
typedef enum
{
    WPS_MIXED_ENCRYPTION = 0x000c,
    WPS_AES_ENCRYPTION   = 0x0008,
    WPS_TKIP_ENCRYPTION  = 0x0004, /* Deprecated in WSC 2.0 */
    WPS_WEP_ENCRYPTION   = 0x0002, /* Deprecated in WSC 2.0 */
    WPS_NO_ENCRYPTION    = 0x0001,
    WPS_NO_UNDEFINED     = 0x0000,
} wps_encryption_type_t;


/* WPS authentication types */
typedef enum
{
    WPS_OPEN_AUTHENTICATION               = 0x0001,
    WPS_WPA_PSK_AUTHENTICATION            = 0x0002, /* Deprecated in version 2.0 */
    WPS_SHARED_AUTHENTICATION             = 0x0004, /* Deprecated in version 2.0 */
    WPS_WPA_ENTERPRISE_AUTHENTICATION     = 0x0008, /* Deprecated in version 2.0 */
    WPS_WPA2_ENTERPRISE_AUTHENTICATION    = 0x0010,
    WPS_WPA2_PSK_AUTHENTICATION           = 0x0020,
    WPS_WPA2_WPA_PSK_MIXED_AUTHENTICATION = 0x0022,
} wps_authentication_type_t;

/* WPS authentication types */
typedef enum
{
    WPS_TLV_VERSION             = (1 << 0),
    WPS_TLV_ENROLLEE_NONCE      = (1 << 1),
    WPS_TLV_E_HASH1             = (1 << 2),
    WPS_TLV_E_HASH2             = (1 << 3),
    WPS_TLV_ENCRYPTION_SETTINGS = (1 << 4),
    WPS_TLV_AUTHENTICATOR       = (1 << 5),
    WPS_TLV_REGISTRAR_NONCE     = (1 << 6),
    WPS_TLV_AUTH_TYPE_FLAGS     = (1 << 7),
    WPS_TLV_ENCR_TYPE_FLAGS     = (1 << 8),
    WPS_TLV_UUID_R              = (1 << 9),
    WPS_TLV_PUBLIC_KEY          = (1 << 10),
    WPS_TLV_MSG_TYPE            = (1 << 11),
    WPS_TLV_X509_CERT           = (1 << 12),
    WPS_TLV_VENDOR_EXT          = (1 << 13),
    WPS_TLV_R_HASH1             = (1 << 14),
    WPS_TLV_R_HASH2             = (1 << 15),
    WPS_TLV_SSID                = (1 << 16),
    WPS_TLV_AUTH_TYPE           = (1 << 17),
    WPS_TLV_ENCR_TYPE           = (1 << 18),
    WPS_TLV_NW_KEY              = (1 << 19),
    WPS_TLV_MAC_ADDR            = (1 << 20),
    WPS_TLV_E_SNONCE1           = (1 << 21),
    WPS_TLV_E_SNONCE2           = (1 << 22),
    WPS_TLV_R_SNONCE1           = (1 << 23),
    WPS_TLV_R_SNONCE2           = (1 << 24),
    WPS_TLV_CREDENTIAL          = (1 << 25),
} wps_tlv_mask_value_t;

typedef enum
{
    WPS_CRYPTO_MATERIAL_ENROLLEE_NONCE       = (1 <<  1), // 0x002
    WPS_CRYPTO_MATERIAL_ENROLLEE_HASH1       = (1 <<  2), // 0x004
    WPS_CRYPTO_MATERIAL_ENROLLEE_HASH2       = (1 <<  3), // 0x008
    WPS_CRYPTO_MATERIAL_ENROLLEE_PUBLIC_KEY  = (1 <<  4), // 0x010
    WPS_CRYPTO_MATERIAL_ENROLLEE_MAC_ADDRESS = (1 <<  5), // 0x020
    WPS_CRYPTO_MATERIAL_REGISTRAR_NONCE      = (1 <<  6), // 0x040
    WPS_CRYPTO_MATERIAL_REGISTRAR_HASH1      = (1 <<  7), // 0x080
    WPS_CRYPTO_MATERIAL_REGISTRAR_HASH2      = (1 <<  8), // 0x100
    WPS_CRYPTO_MATERIAL_REGISTRAR_PUBLIC_KEY = (1 <<  9), // 0x200
    WPS_CRYPTO_MATERIAL_AUTH_KEY             = (1 << 10), // 0x400
    WPS_CRYPTO_MATERIAL_KEY_WRAP_KEY         = (1 << 11), // 0x800
} wps_crypto_material_value_t;

typedef enum
{
    WPS_ENROLLEE_AGENT  = 0,
    WPS_REGISTRAR_AGENT = 1,
} wps_agent_type_t;


/* High level states
 * INITIALISING ( scan, join )
 * EAP_HANDSHAKE ( go through EAP state machine )
 * WPS_HANDSHAKE ( go through WPS state machine )
 */
typedef enum
{
    WPS_INITIALISING,
    WPS_IN_WPS_HANDSHAKE,
    WPS_CLOSING_EAP,
} wps_main_stage_t;

typedef enum
{
    WPS_EAP_START         = 0,    /* (EAP start ) */
    WPS_EAP_IDENTITY      = 1,    /* (EAP identity request, EAP identity response) */
    WPS_WSC_START         = 2,    /* (WSC start) */
    WPS_ENROLLEE_DISCOVER = 3,    /* Special state machine just for Enrollees*/
} wps_eap_state_machine_stage_t;

typedef enum
{
    WPS_SENDING_PUBLIC_KEYS,
    WPS_SENDING_SECRET_HASHES,
    WPS_SENDING_SECRET_NONCE1,
    WPS_SENDING_SECRET_NONCE2,
    WPS_SENDING_CREDENTIALS = WPS_SENDING_SECRET_NONCE2,
} wps_state_machine_stage_t;


/******************************************************
 *                    Variables
 ******************************************************/

extern const uint32_t wps_m2_timeout; /* Settable timeout for the enrollee when it is waiting for WPS message M2 from the registrar */

#ifdef __cplusplus
} /*extern "C" */
#endif
