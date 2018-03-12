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

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define EAP_MTU_SIZE  ( 1020 )

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    EAP_CODE_REQUEST  = 1,
    EAP_CODE_RESPONSE = 2,
    EAP_CODE_SUCCESS  = 3,
    EAP_CODE_FAILURE  = 4
} eap_code_t;

/*
 * EAP Request and Response data begins with one octet Type. Success and
 * Failure do not have additional data.
 */
typedef enum {
    EAP_TYPE_NONE         = 0,
    EAP_TYPE_IDENTITY     = 1   /* RFC 3748 */,
    EAP_TYPE_NOTIFICATION = 2   /* RFC 3748 */,
    EAP_TYPE_NAK          = 3   /* Response only, RFC 3748 */,
    EAP_TYPE_MD5          = 4,  /* RFC 3748 */
    EAP_TYPE_OTP          = 5   /* RFC 3748 */,
    EAP_TYPE_GTC          = 6,  /* RFC 3748 */
    EAP_TYPE_TLS          = 13  /* RFC 2716 */,
    EAP_TYPE_LEAP         = 17  /* Cisco proprietary */,
    EAP_TYPE_SIM          = 18  /* draft-haverinen-pppext-eap-sim-12.txt */,
    EAP_TYPE_TTLS         = 21  /* draft-ietf-pppext-eap-ttls-02.txt */,
    EAP_TYPE_AKA          = 23  /* draft-arkko-pppext-eap-aka-12.txt */,
    EAP_TYPE_PEAP         = 25  /* draft-josefsson-pppext-eap-tls-eap-06.txt */,
    EAP_TYPE_MSCHAPV2     = 26  /* draft-kamath-pppext-eap-mschapv2-00.txt */,
    EAP_TYPE_TLV          = 33  /* draft-josefsson-pppext-eap-tls-eap-07.txt */,
    EAP_TYPE_FAST         = 43  /* draft-cam-winget-eap-fast-00.txt */,
    EAP_TYPE_PAX          = 46, /* draft-clancy-eap-pax-04.txt */
    EAP_TYPE_EXPANDED_NAK = 253 /* RFC 3748 */,
    EAP_TYPE_WPS          = 254 /* Wireless Simple Config */,
    EAP_TYPE_PSK          = 255 /* EXPERIMENTAL - type not yet allocated draft-bersani-eap-psk-09 */
} eap_type_t;

/**
 * EAPOL types
 */

typedef enum
{
    EAP_PACKET                   = 0,
    EAPOL_START                  = 1,
    EAPOL_LOGOFF                 = 2,
    EAPOL_KEY                    = 3,
    EAPOL_ENCAPSULATED_ASF_ALERT = 4
} eapol_packet_type_t;


/*
 * MSCHAPV2 codes
 */
typedef enum {
    MSCHAPV2_OPCODE_CHALLENGE       = 1,
    MSCHAPV2_OPCODE_RESPONSE        = 2,
    MSCHAPV2_OPCODE_SUCCESS         = 3,
    MSCHAPV2_OPCODE_FAILURE         = 4,
    MSCHAPV2_OPCODE_CHANGE_PASSWORD = 7,
} mschapv2_opcode_t;
#ifdef __cplusplus
} /*extern "C" */
#endif
