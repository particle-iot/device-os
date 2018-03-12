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

#define AES_BLOCK_SZ        16

#define SHA_LBLOCK  16
#define SHA256_CBLOCK   (SHA_LBLOCK*4)
#define SHA256_DIGEST_LENGTH    32

#define SSL_VERIFY_NONE                 0
#define SSL_VERIFY_OPTIONAL             1
#define SSL_VERIFY_REQUIRED             2

#define SSL_MSG_CHANGE_CIPHER_SPEC     20
#define SSL_MSG_ALERT                  21
#define SSL_MSG_HANDSHAKE              22
#define SSL_MSG_APPLICATION_DATA       23

// ALERT PROTOCOL
// REFER RFC5246 Section 7.2
#define SSL_ALERT_WARNING               1
#define SSL_ALERT_FATAL                 2

#define SSL_ALERT_CLOSE_NOTIFY          0
#define SSL_ALERT_UNEXPECTED_MESSAGE   10
#define SSL_ALERT_BAD_RECORD_MAC       20
#define SSL_ALERT_DECRYPT_FAIL         21
#define SSL_ALERT_RECORD_OVERFLOW      22
#define SSL_ALERT_DECOMP_FAIL          30
#define SSL_ALERT_HANDSHAKE_FAILED     40
#define SSL_ALERT_CERT_NONE            41
#define SSL_ALERT_CERT_BAD             42
#define SSL_ALERT_CERT_UNSUPPORTED     43
#define SSL_ALERT_CERT_REVOKED         44
#define SSL_ALERT_CERT_EXPIRED         45
#define SSL_ALERT_CERT_UNKNOWN         46
#define SSL_ALERT_ILLEGAL_PARAM        47
#define SSL_ALERT_CA_UNKNOWN           48
#define SSL_ALERT_ACCESS_DENIED        49
#define SSL_ALERT_DECODE_ERROR         50
#define SSL_ALERT_DECRYPT_ERROR        51
#define SSL_ALERT_EXPORT_RESTRICT      60
#define SSL_ALERT_PROTOCOL_VERSION     70
#define SSL_ALERT_INSUFFICIENT_SEC     71
#define SSL_ALERT_INTERNAL_ERROR       80
#define SSL_ALERT_USER_CANCELED        90
#define SSL_ALERT_NO_RENEGOTIATION     100
#define SSL_ALERT_UNSUPPORTED_EXTENT   110  // RFC4366
#define SSL_ALERT_CERT_UNOBTAINABLE    111  // RFC4366
#define SSL_ALERT_UNRECOGNIZED_NAME    112  // RFC4366
#define SSL_ALERT_BAD_CERT_STATUS_RESP 113  // RFC4366
#define SSL_ALERT_BAD_CERT_HASH_VALUE  114  // RFC4366

// Diffie-Hellman Constants
#define MYKROSSL_ERR_DHM_BAD_INPUT_DATA                    -0x0480
#define MYKROSSL_ERR_DHM_READ_PARAMS_FAILED                -0x0490
#define MYKROSSL_ERR_DHM_MAKE_PARAMS_FAILED                -0x04A0
#define MYKROSSL_ERR_DHM_READ_PUBLIC_FAILED                -0x04B0
#define MYKROSSL_ERR_DHM_MAKE_PUBLIC_FAILED                -0x04C0
#define MYKROSSL_ERR_DHM_CALC_SECRET_FAILED                -0x04D0

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

#ifdef __cplusplus
} /*extern "C" */
#endif
