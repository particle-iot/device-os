/*
 * Copyright (c) 2015 Broadcom
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * 3. Neither the name of Broadcom nor the names of other contributors to this
 * software may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * 4. This software may not be used as a standalone product, and may only be used as
 * incorporated in your product or device that incorporates Broadcom wireless connectivity
 * products and solely for the purpose of enabling the functionalities of such Broadcom products.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY WARRANTIES OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT, ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
