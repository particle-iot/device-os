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

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


//#undef USE_SSL3                // Insecure
//
//#undef USE_INSECURE_CIPHERS
//
//#undef USE_MD5_MAC             // Insecure
//#undef USE_RC4_128_CIPHER      // Insecure
//#undef USE_3DES_EDE_CBC_CIPHER // Insecure


#define USE_RSA_KEYSCHEME
#define USE_SHA_MAC
#define USE_SHA256_MAC
#define USE_SHA384_MAC
#define USE_AES_256_CBC_CIPHER
#define USE_AES_128_CBC_CIPHER
#define USE_CAMELLIA_128_CBC_CIPHER
#define USE_CAMELLIA_256_CBC_CIPHER
#define USE_SEED_CBC_CIPHER
#define USE_DHE_RSA_KEYSCHEME

typedef enum
{
    SSL3_0 = 0,
    TLS1_0 = 1,
    TLS1_1 = 2,
    TLS1_2 = 3,
} tls_version_num_t;

extern tls_version_num_t tls_minimum_version;
extern tls_version_num_t tls_maximum_version;

#ifdef USE_SSL3
#define MIN_TLS_VERSION_AVAILABLE  SSL3_0
#else
#define MIN_TLS_VERSION_AVAILABLE  TLS1_0
#endif

extern const char * tls_version_names[];


typedef enum
{
/* Insecure */
    NULL_CIPHER,
    RC4_40_CIPHER,
    RC4_128_CIPHER,
    RC2_CBC_40_CIPHER,
    IDEA_CBC_CIPHER,
    DES40_CBC_CIPHER,
    DES_CBC_CIPHER,
    DES_CBC_40_CIPHER,
    TRIPLE_DES_EDE_CBC_CIPHER,

/* Secure */
    AES_128_CBC_CIPHER,
    AES_256_CBC_CIPHER,
    AES_128_GCM_CIPHER,
    AES_256_GCM_CIPHER,
    AES_128_CCM_CIPHER,
    AES_256_CCM_CIPHER,
    AES_128_CCM_8_CIPHER,
    AES_256_CCM_8_CIPHER,
    CAMELLIA_128_CBC_CIPHER,
    CAMELLIA_256_CBC_CIPHER,
    CAMELLIA_128_GCM_CIPHER,
    CAMELLIA_256_GCM_CIPHER,
    SEED_CBC_CIPHER,
    ARIA_128_CBC_CIPHER,
    ARIA_256_CBC_CIPHER,
    ARIA_128_GCM_CIPHER,
    ARIA_256_GCM_CIPHER,
    CHACHA20_POLY1305_CIPHER,
} cipher_t;

typedef enum
{
/* Insecure */
    NULL_KEYSCHEME,
    KRB5_KEYSCHEME,
    KRB5_EXPORT_KEYSCHEME,
    RSA_EXPORT_KEYSCHEME,
    DH_DSS_EXPORT_KEYSCHEME,
    DHE_RSA_EXPORT_KEYSCHEME,
    DH_anon_KEYSCHEME,
    DH_anon_EXPORT_KEYSCHEME,
    DH_RSA_EXPORT_KEYSCHEME,
    DHE_DSS_EXPORT_KEYSCHEME,
    ECDH_anon_KEYSCHEME,

/* Secure */
    RSA_KEYSCHEME,
    DH_DSS_KEYSCHEME,
    DH_RSA_KEYSCHEME,
    DHE_DSS_KEYSCHEME,
    DHE_RSA_KEYSCHEME,
    ECDH_ECDSA_KEYSCHEME,
    ECDH_RSA_KEYSCHEME,
    ECDHE_RSA_KEYSCHEME,
    ECDHE_ECDSA_KEYSCHEME,
    PSK_KEYSCHEME,
    RSA_PSK_KEYSCHEME,
    DHE_PSK_KEYSCHEME,
    ECDHE_PSK_KEYSCHEME,
    SRP_SHA_KEYSCHEME,
    SRP_SHA_RSA_KEYSCHEME,
    SRP_SHA_DSS_KEYSCHEME,
} key_agreement_authentication_t;

typedef enum
{
/* Insecure */
    NULL_MAC,
    MD5_MAC,

/* Secure */
    SHA_MAC,
    SHA256_MAC,
    SHA384_MAC,
    AES_128_CCM_MAC,
    AES_256_CCM_MAC,
    AES_128_CCM_8_MAC,
    AES_266_CCM_8_MAC,
} message_authentication_t;

struct keyscheme_api_t;
struct cipher_api_t;
struct mac_api_t;


typedef struct
{
        uint16_t                       code;
        const struct keyscheme_api_t*  key_mechanism_driver;
        const struct cipher_api_t*     cipher_driver;
        const struct mac_api_t*        mac_driver;
} cipher_suite_t;




extern const char* mac_names[];
extern const char* keyscheme_names[];
extern const char * cipher_names[];

#ifndef USE_INSECURE_CIPHERS

#if defined( USE_NULL_CIPHER              ) || \
    defined( USE_RC4_40_CIPHER            ) || \
    defined( USE_RC4_128_CIPHER           ) || \
    defined( USE_RC2_CBC_40_CIPHER        ) || \
    defined( USE_IDEA_CBC_CIPHER          ) || \
    defined( USE_DES40_CBC_CIPHER         ) || \
    defined( USE_DES_CBC_CIPHER           ) || \
    defined( USE_DES_CBC_40_CIPHER        ) || \
    defined( USE_3DES_EDE_CBC_CIPHER      ) || \
    defined( USE_NULL_KEYSCHEME           ) || \
    defined( USE_KRB5_KEYSCHEME           ) || \
    defined( USE_KRB5_EXPORT_KEYSCHEME    ) || \
    defined( USE_RSA_EXPORT_KEYSCHEME     ) || \
    defined( USE_DH_DSS_EXPORT_KEYSCHEME  ) || \
    defined( USE_DHE_RSA_EXPORT_KEYSCHEME ) || \
    defined( USE_DH_anon_EXPORT_KEYSCHEME ) || \
    defined( USE_DH_RSA_EXPORT_KEYSCHEME  ) || \
    defined( USE_DHE_DSS_EXPORT_KEYSCHEME ) || \
    defined( USE_NULL_MAC                 ) || \
    defined( USE_MD5_MAC                  )
#error Insecure TLS define detected
#endif

#endif /* ifndef USE_INSECURE_CIPHERS */




/* Note - The following code was generated using the conv_cipher_suite.pl script */
/* See https://www.iana.org/assignments/tls-parameters/tls-parameters.xhtml#tls-parameters-4 */


#if defined( USE_NULL_KEYSCHEME ) && defined( USE_NULL_CIPHER ) && defined( USE_NULL_MAC )
extern const cipher_suite_t TLS_NULL_WITH_NULL_NULL;
#endif /* if defined( USE_NULL_KEYSCHEME ) && defined( USE_NULL_CIPHER ) && defined( USE_NULL_MAC ) */

#if defined( USE_RSA_KEYSCHEME ) && defined( USE_NULL_CIPHER ) && defined( USE_MD5_MAC )
extern const cipher_suite_t TLS_RSA_WITH_NULL_MD5;
#endif /* if defined( USE_RSA_KEYSCHEME ) && defined( USE_NULL_CIPHER ) && defined( USE_MD5_MAC ) */

#if defined( USE_RSA_KEYSCHEME ) && defined( USE_NULL_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_RSA_WITH_NULL_SHA;
#endif /* if defined( USE_RSA_KEYSCHEME ) && defined( USE_NULL_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_RSA_EXPORT_KEYSCHEME ) && defined( USE_RC4_40_CIPHER ) && defined( USE_MD5_MAC )
extern const cipher_suite_t TLS_RSA_EXPORT_WITH_RC4_40_MD5;
#endif /* if defined( USE_RSA_EXPORT_KEYSCHEME ) && defined( USE_RC4_40_CIPHER ) && defined( USE_MD5_MAC ) */

#if defined( USE_RSA_KEYSCHEME ) && defined( USE_RC4_128_CIPHER ) && defined( USE_MD5_MAC )
extern const cipher_suite_t TLS_RSA_WITH_RC4_128_MD5;
#endif /* if defined( USE_RSA_KEYSCHEME ) && defined( USE_RC4_128_CIPHER ) && defined( USE_MD5_MAC ) */

#if defined( USE_RSA_KEYSCHEME ) && defined( USE_RC4_128_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_RSA_WITH_RC4_128_SHA;
#endif /* if defined( USE_RSA_KEYSCHEME ) && defined( USE_RC4_128_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_RSA_EXPORT_KEYSCHEME ) && defined( USE_RC2_CBC_40_CIPHER ) && defined( USE_MD5_MAC )
extern const cipher_suite_t TLS_RSA_EXPORT_WITH_RC2_CBC_40_MD5;
#endif /* if defined( USE_RSA_EXPORT_KEYSCHEME ) && defined( USE_RC2_CBC_40_CIPHER ) && defined( USE_MD5_MAC ) */

#if defined( USE_RSA_KEYSCHEME ) && defined( USE_IDEA_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_RSA_WITH_IDEA_CBC_SHA;
#endif /* if defined( USE_RSA_KEYSCHEME ) && defined( USE_IDEA_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_RSA_EXPORT_KEYSCHEME ) && defined( USE_DES40_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_RSA_EXPORT_WITH_DES40_CBC_SHA;
#endif /* if defined( USE_RSA_EXPORT_KEYSCHEME ) && defined( USE_DES40_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_RSA_KEYSCHEME ) && defined( USE_DES_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_RSA_WITH_DES_CBC_SHA;
#endif /* if defined( USE_RSA_KEYSCHEME ) && defined( USE_DES_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_RSA_KEYSCHEME ) && defined( USE_3DES_EDE_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_RSA_WITH_3DES_EDE_CBC_SHA;
#endif /* if defined( USE_RSA_KEYSCHEME ) && defined( USE_3DES_EDE_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_DH_DSS_EXPORT_KEYSCHEME ) && defined( USE_DES40_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_DH_DSS_EXPORT_WITH_DES40_CBC_SHA;
#endif /* if defined( USE_DH_DSS_EXPORT_KEYSCHEME ) && defined( USE_DES40_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_DH_DSS_KEYSCHEME ) && defined( USE_DES_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_DH_DSS_WITH_DES_CBC_SHA;
#endif /* if defined( USE_DH_DSS_KEYSCHEME ) && defined( USE_DES_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_DH_DSS_KEYSCHEME ) && defined( USE_3DES_EDE_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_DH_DSS_WITH_3DES_EDE_CBC_SHA;
#endif /* if defined( USE_DH_DSS_KEYSCHEME ) && defined( USE_3DES_EDE_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_DH_RSA_EXPORT_KEYSCHEME ) && defined( USE_DES40_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_DH_RSA_EXPORT_WITH_DES40_CBC_SHA;
#endif /* if defined( USE_DH_RSA_EXPORT_KEYSCHEME ) && defined( USE_DES40_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_DH_RSA_KEYSCHEME ) && defined( USE_DES_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_DH_RSA_WITH_DES_CBC_SHA;
#endif /* if defined( USE_DH_RSA_KEYSCHEME ) && defined( USE_DES_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_DH_RSA_KEYSCHEME ) && defined( USE_3DES_EDE_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_DH_RSA_WITH_3DES_EDE_CBC_SHA;
#endif /* if defined( USE_DH_RSA_KEYSCHEME ) && defined( USE_3DES_EDE_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_DHE_DSS_EXPORT_KEYSCHEME ) && defined( USE_DES40_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_DHE_DSS_EXPORT_WITH_DES40_CBC_SHA;
#endif /* if defined( USE_DHE_DSS_EXPORT_KEYSCHEME ) && defined( USE_DES40_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_DHE_DSS_KEYSCHEME ) && defined( USE_DES_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_DHE_DSS_WITH_DES_CBC_SHA;
#endif /* if defined( USE_DHE_DSS_KEYSCHEME ) && defined( USE_DES_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_DHE_DSS_KEYSCHEME ) && defined( USE_3DES_EDE_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_DHE_DSS_WITH_3DES_EDE_CBC_SHA;
#endif /* if defined( USE_DHE_DSS_KEYSCHEME ) && defined( USE_3DES_EDE_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_DHE_RSA_EXPORT_KEYSCHEME ) && defined( USE_DES40_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_DHE_RSA_EXPORT_WITH_DES40_CBC_SHA;
#endif /* if defined( USE_DHE_RSA_EXPORT_KEYSCHEME ) && defined( USE_DES40_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_DHE_RSA_KEYSCHEME ) && defined( USE_DES_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_DHE_RSA_WITH_DES_CBC_SHA;
#endif /* if defined( USE_DHE_RSA_KEYSCHEME ) && defined( USE_DES_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_DHE_RSA_KEYSCHEME ) && defined( USE_3DES_EDE_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_DHE_RSA_WITH_3DES_EDE_CBC_SHA;
#endif /* if defined( USE_DHE_RSA_KEYSCHEME ) && defined( USE_3DES_EDE_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_DH_anon_EXPORT_KEYSCHEME ) && defined( USE_RC4_40_CIPHER ) && defined( USE_MD5_MAC )
extern const cipher_suite_t TLS_DH_anon_EXPORT_WITH_RC4_40_MD5;
#endif /* if defined( USE_DH_anon_EXPORT_KEYSCHEME ) && defined( USE_RC4_40_CIPHER ) && defined( USE_MD5_MAC ) */

#if defined( USE_DH_anon_KEYSCHEME ) && defined( USE_RC4_128_CIPHER ) && defined( USE_MD5_MAC )
extern const cipher_suite_t TLS_DH_anon_WITH_RC4_128_MD5;
#endif /* if defined( USE_DH_anon_KEYSCHEME ) && defined( USE_RC4_128_CIPHER ) && defined( USE_MD5_MAC ) */

#if defined( USE_DH_anon_EXPORT_KEYSCHEME ) && defined( USE_DES40_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_DH_anon_EXPORT_WITH_DES40_CBC_SHA;
#endif /* if defined( USE_DH_anon_EXPORT_KEYSCHEME ) && defined( USE_DES40_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_DH_anon_KEYSCHEME ) && defined( USE_DES_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_DH_anon_WITH_DES_CBC_SHA;
#endif /* if defined( USE_DH_anon_KEYSCHEME ) && defined( USE_DES_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_DH_anon_KEYSCHEME ) && defined( USE_3DES_EDE_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_DH_anon_WITH_3DES_EDE_CBC_SHA;
#endif /* if defined( USE_DH_anon_KEYSCHEME ) && defined( USE_3DES_EDE_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_KRB5_KEYSCHEME ) && defined( USE_DES_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_KRB5_WITH_DES_CBC_SHA;
#endif /* if defined( USE_KRB5_KEYSCHEME ) && defined( USE_DES_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_KRB5_KEYSCHEME ) && defined( USE_3DES_EDE_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_KRB5_WITH_3DES_EDE_CBC_SHA;
#endif /* if defined( USE_KRB5_KEYSCHEME ) && defined( USE_3DES_EDE_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_KRB5_KEYSCHEME ) && defined( USE_RC4_128_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_KRB5_WITH_RC4_128_SHA;
#endif /* if defined( USE_KRB5_KEYSCHEME ) && defined( USE_RC4_128_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_KRB5_KEYSCHEME ) && defined( USE_IDEA_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_KRB5_WITH_IDEA_CBC_SHA;
#endif /* if defined( USE_KRB5_KEYSCHEME ) && defined( USE_IDEA_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_KRB5_KEYSCHEME ) && defined( USE_DES_CBC_CIPHER ) && defined( USE_MD5_MAC )
extern const cipher_suite_t TLS_KRB5_WITH_DES_CBC_MD5;
#endif /* if defined( USE_KRB5_KEYSCHEME ) && defined( USE_DES_CBC_CIPHER ) && defined( USE_MD5_MAC ) */

#if defined( USE_KRB5_KEYSCHEME ) && defined( USE_3DES_EDE_CBC_CIPHER ) && defined( USE_MD5_MAC )
extern const cipher_suite_t TLS_KRB5_WITH_3DES_EDE_CBC_MD5;
#endif /* if defined( USE_KRB5_KEYSCHEME ) && defined( USE_3DES_EDE_CBC_CIPHER ) && defined( USE_MD5_MAC ) */

#if defined( USE_KRB5_KEYSCHEME ) && defined( USE_RC4_128_CIPHER ) && defined( USE_MD5_MAC )
extern const cipher_suite_t TLS_KRB5_WITH_RC4_128_MD5;
#endif /* if defined( USE_KRB5_KEYSCHEME ) && defined( USE_RC4_128_CIPHER ) && defined( USE_MD5_MAC ) */

#if defined( USE_KRB5_KEYSCHEME ) && defined( USE_IDEA_CBC_CIPHER ) && defined( USE_MD5_MAC )
extern const cipher_suite_t TLS_KRB5_WITH_IDEA_CBC_MD5;
#endif /* if defined( USE_KRB5_KEYSCHEME ) && defined( USE_IDEA_CBC_CIPHER ) && defined( USE_MD5_MAC ) */

#if defined( USE_KRB5_EXPORT_KEYSCHEME ) && defined( USE_DES_CBC_40_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_KRB5_EXPORT_WITH_DES_CBC_40_SHA;
#endif /* if defined( USE_KRB5_EXPORT_KEYSCHEME ) && defined( USE_DES_CBC_40_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_KRB5_EXPORT_KEYSCHEME ) && defined( USE_RC2_CBC_40_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_KRB5_EXPORT_WITH_RC2_CBC_40_SHA;
#endif /* if defined( USE_KRB5_EXPORT_KEYSCHEME ) && defined( USE_RC2_CBC_40_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_KRB5_EXPORT_KEYSCHEME ) && defined( USE_RC4_40_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_KRB5_EXPORT_WITH_RC4_40_SHA;
#endif /* if defined( USE_KRB5_EXPORT_KEYSCHEME ) && defined( USE_RC4_40_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_KRB5_EXPORT_KEYSCHEME ) && defined( USE_DES_CBC_40_CIPHER ) && defined( USE_MD5_MAC )
extern const cipher_suite_t TLS_KRB5_EXPORT_WITH_DES_CBC_40_MD5;
#endif /* if defined( USE_KRB5_EXPORT_KEYSCHEME ) && defined( USE_DES_CBC_40_CIPHER ) && defined( USE_MD5_MAC ) */

#if defined( USE_KRB5_EXPORT_KEYSCHEME ) && defined( USE_RC2_CBC_40_CIPHER ) && defined( USE_MD5_MAC )
extern const cipher_suite_t TLS_KRB5_EXPORT_WITH_RC2_CBC_40_MD5;
#endif /* if defined( USE_KRB5_EXPORT_KEYSCHEME ) && defined( USE_RC2_CBC_40_CIPHER ) && defined( USE_MD5_MAC ) */

#if defined( USE_KRB5_EXPORT_KEYSCHEME ) && defined( USE_RC4_40_CIPHER ) && defined( USE_MD5_MAC )
extern const cipher_suite_t TLS_KRB5_EXPORT_WITH_RC4_40_MD5;
#endif /* if defined( USE_KRB5_EXPORT_KEYSCHEME ) && defined( USE_RC4_40_CIPHER ) && defined( USE_MD5_MAC ) */

#if defined( USE_PSK_KEYSCHEME ) && defined( USE_NULL_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_PSK_WITH_NULL_SHA;
#endif /* if defined( USE_PSK_KEYSCHEME ) && defined( USE_NULL_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_DHE_PSK_KEYSCHEME ) && defined( USE_NULL_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_DHE_PSK_WITH_NULL_SHA;
#endif /* if defined( USE_DHE_PSK_KEYSCHEME ) && defined( USE_NULL_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_RSA_PSK_KEYSCHEME ) && defined( USE_NULL_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_RSA_PSK_WITH_NULL_SHA;
#endif /* if defined( USE_RSA_PSK_KEYSCHEME ) && defined( USE_NULL_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_RSA_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_RSA_WITH_AES_128_CBC_SHA;
#endif /* if defined( USE_RSA_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_DH_DSS_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_DH_DSS_WITH_AES_128_CBC_SHA;
#endif /* if defined( USE_DH_DSS_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_DH_RSA_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_DH_RSA_WITH_AES_128_CBC_SHA;
#endif /* if defined( USE_DH_RSA_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_DHE_DSS_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_DHE_DSS_WITH_AES_128_CBC_SHA;
#endif /* if defined( USE_DHE_DSS_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_DHE_RSA_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_DHE_RSA_WITH_AES_128_CBC_SHA;
#endif /* if defined( USE_DHE_RSA_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_DH_anon_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_DH_anon_WITH_AES_128_CBC_SHA;
#endif /* if defined( USE_DH_anon_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_RSA_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_RSA_WITH_AES_256_CBC_SHA;
#endif /* if defined( USE_RSA_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_DH_DSS_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_DH_DSS_WITH_AES_256_CBC_SHA;
#endif /* if defined( USE_DH_DSS_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_DH_RSA_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_DH_RSA_WITH_AES_256_CBC_SHA;
#endif /* if defined( USE_DH_RSA_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_DHE_DSS_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_DHE_DSS_WITH_AES_256_CBC_SHA;
#endif /* if defined( USE_DHE_DSS_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_DHE_RSA_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_DHE_RSA_WITH_AES_256_CBC_SHA;
#endif /* if defined( USE_DHE_RSA_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_DH_anon_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_DH_anon_WITH_AES_256_CBC_SHA;
#endif /* if defined( USE_DH_anon_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_RSA_KEYSCHEME ) && defined( USE_NULL_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_RSA_WITH_NULL_SHA256;
#endif /* if defined( USE_RSA_KEYSCHEME ) && defined( USE_NULL_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_RSA_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_RSA_WITH_AES_128_CBC_SHA256;
#endif /* if defined( USE_RSA_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_RSA_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_RSA_WITH_AES_256_CBC_SHA256;
#endif /* if defined( USE_RSA_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_DH_DSS_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_DH_DSS_WITH_AES_128_CBC_SHA256;
#endif /* if defined( USE_DH_DSS_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_DH_RSA_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_DH_RSA_WITH_AES_128_CBC_SHA256;
#endif /* if defined( USE_DH_RSA_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_DHE_DSS_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_DHE_DSS_WITH_AES_128_CBC_SHA256;
#endif /* if defined( USE_DHE_DSS_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_128_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_RSA_WITH_CAMELLIA_128_CBC_SHA;
#endif /* if defined( USE_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_128_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_DH_DSS_KEYSCHEME ) && defined( USE_CAMELLIA_128_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_DH_DSS_WITH_CAMELLIA_128_CBC_SHA;
#endif /* if defined( USE_DH_DSS_KEYSCHEME ) && defined( USE_CAMELLIA_128_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_DH_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_128_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_DH_RSA_WITH_CAMELLIA_128_CBC_SHA;
#endif /* if defined( USE_DH_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_128_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_DHE_DSS_KEYSCHEME ) && defined( USE_CAMELLIA_128_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_DHE_DSS_WITH_CAMELLIA_128_CBC_SHA;
#endif /* if defined( USE_DHE_DSS_KEYSCHEME ) && defined( USE_CAMELLIA_128_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_DHE_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_128_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_DHE_RSA_WITH_CAMELLIA_128_CBC_SHA;
#endif /* if defined( USE_DHE_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_128_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_DH_anon_KEYSCHEME ) && defined( USE_CAMELLIA_128_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_DH_anon_WITH_CAMELLIA_128_CBC_SHA;
#endif /* if defined( USE_DH_anon_KEYSCHEME ) && defined( USE_CAMELLIA_128_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_DHE_RSA_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_DHE_RSA_WITH_AES_128_CBC_SHA256;
#endif /* if defined( USE_DHE_RSA_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_DH_DSS_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_DH_DSS_WITH_AES_256_CBC_SHA256;
#endif /* if defined( USE_DH_DSS_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_DH_RSA_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_DH_RSA_WITH_AES_256_CBC_SHA256;
#endif /* if defined( USE_DH_RSA_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_DHE_DSS_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_DHE_DSS_WITH_AES_256_CBC_SHA256;
#endif /* if defined( USE_DHE_DSS_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_DHE_RSA_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_DHE_RSA_WITH_AES_256_CBC_SHA256;
#endif /* if defined( USE_DHE_RSA_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_DH_anon_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_DH_anon_WITH_AES_128_CBC_SHA256;
#endif /* if defined( USE_DH_anon_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_DH_anon_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_DH_anon_WITH_AES_256_CBC_SHA256;
#endif /* if defined( USE_DH_anon_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_256_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_RSA_WITH_CAMELLIA_256_CBC_SHA;
#endif /* if defined( USE_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_256_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_DH_DSS_KEYSCHEME ) && defined( USE_CAMELLIA_256_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_DH_DSS_WITH_CAMELLIA_256_CBC_SHA;
#endif /* if defined( USE_DH_DSS_KEYSCHEME ) && defined( USE_CAMELLIA_256_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_DH_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_256_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_DH_RSA_WITH_CAMELLIA_256_CBC_SHA;
#endif /* if defined( USE_DH_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_256_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_DHE_DSS_KEYSCHEME ) && defined( USE_CAMELLIA_256_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_DHE_DSS_WITH_CAMELLIA_256_CBC_SHA;
#endif /* if defined( USE_DHE_DSS_KEYSCHEME ) && defined( USE_CAMELLIA_256_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_DHE_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_256_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_DHE_RSA_WITH_CAMELLIA_256_CBC_SHA;
#endif /* if defined( USE_DHE_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_256_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_DH_anon_KEYSCHEME ) && defined( USE_CAMELLIA_256_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_DH_anon_WITH_CAMELLIA_256_CBC_SHA;
#endif /* if defined( USE_DH_anon_KEYSCHEME ) && defined( USE_CAMELLIA_256_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_PSK_KEYSCHEME ) && defined( USE_RC4_128_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_PSK_WITH_RC4_128_SHA;
#endif /* if defined( USE_PSK_KEYSCHEME ) && defined( USE_RC4_128_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_PSK_KEYSCHEME ) && defined( USE_3DES_EDE_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_PSK_WITH_3DES_EDE_CBC_SHA;
#endif /* if defined( USE_PSK_KEYSCHEME ) && defined( USE_3DES_EDE_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_PSK_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_PSK_WITH_AES_128_CBC_SHA;
#endif /* if defined( USE_PSK_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_PSK_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_PSK_WITH_AES_256_CBC_SHA;
#endif /* if defined( USE_PSK_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_DHE_PSK_KEYSCHEME ) && defined( USE_RC4_128_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_DHE_PSK_WITH_RC4_128_SHA;
#endif /* if defined( USE_DHE_PSK_KEYSCHEME ) && defined( USE_RC4_128_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_DHE_PSK_KEYSCHEME ) && defined( USE_3DES_EDE_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_DHE_PSK_WITH_3DES_EDE_CBC_SHA;
#endif /* if defined( USE_DHE_PSK_KEYSCHEME ) && defined( USE_3DES_EDE_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_DHE_PSK_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_DHE_PSK_WITH_AES_128_CBC_SHA;
#endif /* if defined( USE_DHE_PSK_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_DHE_PSK_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_DHE_PSK_WITH_AES_256_CBC_SHA;
#endif /* if defined( USE_DHE_PSK_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_RSA_PSK_KEYSCHEME ) && defined( USE_RC4_128_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_RSA_PSK_WITH_RC4_128_SHA;
#endif /* if defined( USE_RSA_PSK_KEYSCHEME ) && defined( USE_RC4_128_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_RSA_PSK_KEYSCHEME ) && defined( USE_3DES_EDE_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_RSA_PSK_WITH_3DES_EDE_CBC_SHA;
#endif /* if defined( USE_RSA_PSK_KEYSCHEME ) && defined( USE_3DES_EDE_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_RSA_PSK_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_RSA_PSK_WITH_AES_128_CBC_SHA;
#endif /* if defined( USE_RSA_PSK_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_RSA_PSK_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_RSA_PSK_WITH_AES_256_CBC_SHA;
#endif /* if defined( USE_RSA_PSK_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_RSA_KEYSCHEME ) && defined( USE_SEED_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_RSA_WITH_SEED_CBC_SHA;
#endif /* if defined( USE_RSA_KEYSCHEME ) && defined( USE_SEED_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_DH_DSS_KEYSCHEME ) && defined( USE_SEED_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_DH_DSS_WITH_SEED_CBC_SHA;
#endif /* if defined( USE_DH_DSS_KEYSCHEME ) && defined( USE_SEED_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_DH_RSA_KEYSCHEME ) && defined( USE_SEED_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_DH_RSA_WITH_SEED_CBC_SHA;
#endif /* if defined( USE_DH_RSA_KEYSCHEME ) && defined( USE_SEED_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_DHE_DSS_KEYSCHEME ) && defined( USE_SEED_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_DHE_DSS_WITH_SEED_CBC_SHA;
#endif /* if defined( USE_DHE_DSS_KEYSCHEME ) && defined( USE_SEED_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_DHE_RSA_KEYSCHEME ) && defined( USE_SEED_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_DHE_RSA_WITH_SEED_CBC_SHA;
#endif /* if defined( USE_DHE_RSA_KEYSCHEME ) && defined( USE_SEED_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_DH_anon_KEYSCHEME ) && defined( USE_SEED_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_DH_anon_WITH_SEED_CBC_SHA;
#endif /* if defined( USE_DH_anon_KEYSCHEME ) && defined( USE_SEED_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_RSA_KEYSCHEME ) && defined( USE_AES_128_GCM_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_RSA_WITH_AES_128_GCM_SHA256;
#endif /* if defined( USE_RSA_KEYSCHEME ) && defined( USE_AES_128_GCM_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_RSA_KEYSCHEME ) && defined( USE_AES_256_GCM_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_RSA_WITH_AES_256_GCM_SHA384;
#endif /* if defined( USE_RSA_KEYSCHEME ) && defined( USE_AES_256_GCM_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_DHE_RSA_KEYSCHEME ) && defined( USE_AES_128_GCM_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_DHE_RSA_WITH_AES_128_GCM_SHA256;
#endif /* if defined( USE_DHE_RSA_KEYSCHEME ) && defined( USE_AES_128_GCM_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_DHE_RSA_KEYSCHEME ) && defined( USE_AES_256_GCM_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_DHE_RSA_WITH_AES_256_GCM_SHA384;
#endif /* if defined( USE_DHE_RSA_KEYSCHEME ) && defined( USE_AES_256_GCM_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_DH_RSA_KEYSCHEME ) && defined( USE_AES_128_GCM_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_DH_RSA_WITH_AES_128_GCM_SHA256;
#endif /* if defined( USE_DH_RSA_KEYSCHEME ) && defined( USE_AES_128_GCM_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_DH_RSA_KEYSCHEME ) && defined( USE_AES_256_GCM_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_DH_RSA_WITH_AES_256_GCM_SHA384;
#endif /* if defined( USE_DH_RSA_KEYSCHEME ) && defined( USE_AES_256_GCM_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_DHE_DSS_KEYSCHEME ) && defined( USE_AES_128_GCM_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_DHE_DSS_WITH_AES_128_GCM_SHA256;
#endif /* if defined( USE_DHE_DSS_KEYSCHEME ) && defined( USE_AES_128_GCM_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_DHE_DSS_KEYSCHEME ) && defined( USE_AES_256_GCM_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_DHE_DSS_WITH_AES_256_GCM_SHA384;
#endif /* if defined( USE_DHE_DSS_KEYSCHEME ) && defined( USE_AES_256_GCM_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_DH_DSS_KEYSCHEME ) && defined( USE_AES_128_GCM_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_DH_DSS_WITH_AES_128_GCM_SHA256;
#endif /* if defined( USE_DH_DSS_KEYSCHEME ) && defined( USE_AES_128_GCM_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_DH_DSS_KEYSCHEME ) && defined( USE_AES_256_GCM_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_DH_DSS_WITH_AES_256_GCM_SHA384;
#endif /* if defined( USE_DH_DSS_KEYSCHEME ) && defined( USE_AES_256_GCM_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_DH_anon_KEYSCHEME ) && defined( USE_AES_128_GCM_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_DH_anon_WITH_AES_128_GCM_SHA256;
#endif /* if defined( USE_DH_anon_KEYSCHEME ) && defined( USE_AES_128_GCM_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_DH_anon_KEYSCHEME ) && defined( USE_AES_256_GCM_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_DH_anon_WITH_AES_256_GCM_SHA384;
#endif /* if defined( USE_DH_anon_KEYSCHEME ) && defined( USE_AES_256_GCM_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_PSK_KEYSCHEME ) && defined( USE_AES_128_GCM_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_PSK_WITH_AES_128_GCM_SHA256;
#endif /* if defined( USE_PSK_KEYSCHEME ) && defined( USE_AES_128_GCM_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_PSK_KEYSCHEME ) && defined( USE_AES_256_GCM_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_PSK_WITH_AES_256_GCM_SHA384;
#endif /* if defined( USE_PSK_KEYSCHEME ) && defined( USE_AES_256_GCM_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_DHE_PSK_KEYSCHEME ) && defined( USE_AES_128_GCM_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_DHE_PSK_WITH_AES_128_GCM_SHA256;
#endif /* if defined( USE_DHE_PSK_KEYSCHEME ) && defined( USE_AES_128_GCM_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_DHE_PSK_KEYSCHEME ) && defined( USE_AES_256_GCM_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_DHE_PSK_WITH_AES_256_GCM_SHA384;
#endif /* if defined( USE_DHE_PSK_KEYSCHEME ) && defined( USE_AES_256_GCM_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_RSA_PSK_KEYSCHEME ) && defined( USE_AES_128_GCM_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_RSA_PSK_WITH_AES_128_GCM_SHA256;
#endif /* if defined( USE_RSA_PSK_KEYSCHEME ) && defined( USE_AES_128_GCM_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_RSA_PSK_KEYSCHEME ) && defined( USE_AES_256_GCM_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_RSA_PSK_WITH_AES_256_GCM_SHA384;
#endif /* if defined( USE_RSA_PSK_KEYSCHEME ) && defined( USE_AES_256_GCM_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_PSK_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_PSK_WITH_AES_128_CBC_SHA256;
#endif /* if defined( USE_PSK_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_PSK_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_PSK_WITH_AES_256_CBC_SHA384;
#endif /* if defined( USE_PSK_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_PSK_KEYSCHEME ) && defined( USE_NULL_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_PSK_WITH_NULL_SHA256;
#endif /* if defined( USE_PSK_KEYSCHEME ) && defined( USE_NULL_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_PSK_KEYSCHEME ) && defined( USE_NULL_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_PSK_WITH_NULL_SHA384;
#endif /* if defined( USE_PSK_KEYSCHEME ) && defined( USE_NULL_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_DHE_PSK_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_DHE_PSK_WITH_AES_128_CBC_SHA256;
#endif /* if defined( USE_DHE_PSK_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_DHE_PSK_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_DHE_PSK_WITH_AES_256_CBC_SHA384;
#endif /* if defined( USE_DHE_PSK_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_DHE_PSK_KEYSCHEME ) && defined( USE_NULL_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_DHE_PSK_WITH_NULL_SHA256;
#endif /* if defined( USE_DHE_PSK_KEYSCHEME ) && defined( USE_NULL_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_DHE_PSK_KEYSCHEME ) && defined( USE_NULL_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_DHE_PSK_WITH_NULL_SHA384;
#endif /* if defined( USE_DHE_PSK_KEYSCHEME ) && defined( USE_NULL_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_RSA_PSK_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_RSA_PSK_WITH_AES_128_CBC_SHA256;
#endif /* if defined( USE_RSA_PSK_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_RSA_PSK_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_RSA_PSK_WITH_AES_256_CBC_SHA384;
#endif /* if defined( USE_RSA_PSK_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_RSA_PSK_KEYSCHEME ) && defined( USE_NULL_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_RSA_PSK_WITH_NULL_SHA256;
#endif /* if defined( USE_RSA_PSK_KEYSCHEME ) && defined( USE_NULL_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_RSA_PSK_KEYSCHEME ) && defined( USE_NULL_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_RSA_PSK_WITH_NULL_SHA384;
#endif /* if defined( USE_RSA_PSK_KEYSCHEME ) && defined( USE_NULL_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_RSA_WITH_CAMELLIA_128_CBC_SHA256;
#endif /* if defined( USE_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_DH_DSS_KEYSCHEME ) && defined( USE_CAMELLIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_DH_DSS_WITH_CAMELLIA_128_CBC_SHA256;
#endif /* if defined( USE_DH_DSS_KEYSCHEME ) && defined( USE_CAMELLIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_DH_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_DH_RSA_WITH_CAMELLIA_128_CBC_SHA256;
#endif /* if defined( USE_DH_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_DHE_DSS_KEYSCHEME ) && defined( USE_CAMELLIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_DHE_DSS_WITH_CAMELLIA_128_CBC_SHA256;
#endif /* if defined( USE_DHE_DSS_KEYSCHEME ) && defined( USE_CAMELLIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_DHE_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_DHE_RSA_WITH_CAMELLIA_128_CBC_SHA256;
#endif /* if defined( USE_DHE_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_DH_anon_KEYSCHEME ) && defined( USE_CAMELLIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_DH_anon_WITH_CAMELLIA_128_CBC_SHA256;
#endif /* if defined( USE_DH_anon_KEYSCHEME ) && defined( USE_CAMELLIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_256_CBC_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_RSA_WITH_CAMELLIA_256_CBC_SHA256;
#endif /* if defined( USE_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_256_CBC_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_DH_DSS_KEYSCHEME ) && defined( USE_CAMELLIA_256_CBC_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_DH_DSS_WITH_CAMELLIA_256_CBC_SHA256;
#endif /* if defined( USE_DH_DSS_KEYSCHEME ) && defined( USE_CAMELLIA_256_CBC_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_DH_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_256_CBC_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_DH_RSA_WITH_CAMELLIA_256_CBC_SHA256;
#endif /* if defined( USE_DH_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_256_CBC_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_DHE_DSS_KEYSCHEME ) && defined( USE_CAMELLIA_256_CBC_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_DHE_DSS_WITH_CAMELLIA_256_CBC_SHA256;
#endif /* if defined( USE_DHE_DSS_KEYSCHEME ) && defined( USE_CAMELLIA_256_CBC_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_DHE_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_256_CBC_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_DHE_RSA_WITH_CAMELLIA_256_CBC_SHA256;
#endif /* if defined( USE_DHE_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_256_CBC_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_DH_anon_KEYSCHEME ) && defined( USE_CAMELLIA_256_CBC_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_DH_anon_WITH_CAMELLIA_256_CBC_SHA256;
#endif /* if defined( USE_DH_anon_KEYSCHEME ) && defined( USE_CAMELLIA_256_CBC_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_ECDH_ECDSA_KEYSCHEME ) && defined( USE_NULL_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_ECDH_ECDSA_WITH_NULL_SHA;
#endif /* if defined( USE_ECDH_ECDSA_KEYSCHEME ) && defined( USE_NULL_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_ECDH_ECDSA_KEYSCHEME ) && defined( USE_RC4_128_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_ECDH_ECDSA_WITH_RC4_128_SHA;
#endif /* if defined( USE_ECDH_ECDSA_KEYSCHEME ) && defined( USE_RC4_128_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_ECDH_ECDSA_KEYSCHEME ) && defined( USE_3DES_EDE_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_ECDH_ECDSA_WITH_3DES_EDE_CBC_SHA;
#endif /* if defined( USE_ECDH_ECDSA_KEYSCHEME ) && defined( USE_3DES_EDE_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_ECDH_ECDSA_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA;
#endif /* if defined( USE_ECDH_ECDSA_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_ECDH_ECDSA_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA;
#endif /* if defined( USE_ECDH_ECDSA_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_NULL_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_ECDHE_ECDSA_WITH_NULL_SHA;
#endif /* if defined( USE_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_NULL_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_RC4_128_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_ECDHE_ECDSA_WITH_RC4_128_SHA;
#endif /* if defined( USE_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_RC4_128_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_3DES_EDE_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_ECDHE_ECDSA_WITH_3DES_EDE_CBC_SHA;
#endif /* if defined( USE_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_3DES_EDE_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA;
#endif /* if defined( USE_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA;
#endif /* if defined( USE_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_ECDH_RSA_KEYSCHEME ) && defined( USE_NULL_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_ECDH_RSA_WITH_NULL_SHA;
#endif /* if defined( USE_ECDH_RSA_KEYSCHEME ) && defined( USE_NULL_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_ECDH_RSA_KEYSCHEME ) && defined( USE_RC4_128_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_ECDH_RSA_WITH_RC4_128_SHA;
#endif /* if defined( USE_ECDH_RSA_KEYSCHEME ) && defined( USE_RC4_128_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_ECDH_RSA_KEYSCHEME ) && defined( USE_3DES_EDE_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_ECDH_RSA_WITH_3DES_EDE_CBC_SHA;
#endif /* if defined( USE_ECDH_RSA_KEYSCHEME ) && defined( USE_3DES_EDE_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_ECDH_RSA_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_ECDH_RSA_WITH_AES_128_CBC_SHA;
#endif /* if defined( USE_ECDH_RSA_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_ECDH_RSA_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_ECDH_RSA_WITH_AES_256_CBC_SHA;
#endif /* if defined( USE_ECDH_RSA_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_ECDHE_RSA_KEYSCHEME ) && defined( USE_NULL_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_ECDHE_RSA_WITH_NULL_SHA;
#endif /* if defined( USE_ECDHE_RSA_KEYSCHEME ) && defined( USE_NULL_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_ECDHE_RSA_KEYSCHEME ) && defined( USE_RC4_128_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_ECDHE_RSA_WITH_RC4_128_SHA;
#endif /* if defined( USE_ECDHE_RSA_KEYSCHEME ) && defined( USE_RC4_128_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_ECDHE_RSA_KEYSCHEME ) && defined( USE_3DES_EDE_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_ECDHE_RSA_WITH_3DES_EDE_CBC_SHA;
#endif /* if defined( USE_ECDHE_RSA_KEYSCHEME ) && defined( USE_3DES_EDE_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_ECDHE_RSA_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA;
#endif /* if defined( USE_ECDHE_RSA_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_ECDHE_RSA_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA;
#endif /* if defined( USE_ECDHE_RSA_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_ECDH_anon_KEYSCHEME ) && defined( USE_NULL_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_ECDH_anon_WITH_NULL_SHA;
#endif /* if defined( USE_ECDH_anon_KEYSCHEME ) && defined( USE_NULL_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_ECDH_anon_KEYSCHEME ) && defined( USE_RC4_128_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_ECDH_anon_WITH_RC4_128_SHA;
#endif /* if defined( USE_ECDH_anon_KEYSCHEME ) && defined( USE_RC4_128_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_ECDH_anon_KEYSCHEME ) && defined( USE_3DES_EDE_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_ECDH_anon_WITH_3DES_EDE_CBC_SHA;
#endif /* if defined( USE_ECDH_anon_KEYSCHEME ) && defined( USE_3DES_EDE_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_ECDH_anon_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_ECDH_anon_WITH_AES_128_CBC_SHA;
#endif /* if defined( USE_ECDH_anon_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_ECDH_anon_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_ECDH_anon_WITH_AES_256_CBC_SHA;
#endif /* if defined( USE_ECDH_anon_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_SRP_SHA_KEYSCHEME ) && defined( USE_3DES_EDE_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_SRP_SHA_WITH_3DES_EDE_CBC_SHA;
#endif /* if defined( USE_SRP_SHA_KEYSCHEME ) && defined( USE_3DES_EDE_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_SRP_SHA_RSA_KEYSCHEME ) && defined( USE_3DES_EDE_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_SRP_SHA_RSA_WITH_3DES_EDE_CBC_SHA;
#endif /* if defined( USE_SRP_SHA_RSA_KEYSCHEME ) && defined( USE_3DES_EDE_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_SRP_SHA_DSS_KEYSCHEME ) && defined( USE_3DES_EDE_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_SRP_SHA_DSS_WITH_3DES_EDE_CBC_SHA;
#endif /* if defined( USE_SRP_SHA_DSS_KEYSCHEME ) && defined( USE_3DES_EDE_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_SRP_SHA_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_SRP_SHA_WITH_AES_128_CBC_SHA;
#endif /* if defined( USE_SRP_SHA_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_SRP_SHA_RSA_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_SRP_SHA_RSA_WITH_AES_128_CBC_SHA;
#endif /* if defined( USE_SRP_SHA_RSA_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_SRP_SHA_DSS_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_SRP_SHA_DSS_WITH_AES_128_CBC_SHA;
#endif /* if defined( USE_SRP_SHA_DSS_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_SRP_SHA_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_SRP_SHA_WITH_AES_256_CBC_SHA;
#endif /* if defined( USE_SRP_SHA_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_SRP_SHA_RSA_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_SRP_SHA_RSA_WITH_AES_256_CBC_SHA;
#endif /* if defined( USE_SRP_SHA_RSA_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_SRP_SHA_DSS_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_SRP_SHA_DSS_WITH_AES_256_CBC_SHA;
#endif /* if defined( USE_SRP_SHA_DSS_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256;
#endif /* if defined( USE_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA384;
#endif /* if defined( USE_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_ECDH_ECDSA_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA256;
#endif /* if defined( USE_ECDH_ECDSA_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_ECDH_ECDSA_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA384;
#endif /* if defined( USE_ECDH_ECDSA_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_ECDHE_RSA_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256;
#endif /* if defined( USE_ECDHE_RSA_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_ECDHE_RSA_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384;
#endif /* if defined( USE_ECDHE_RSA_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_ECDH_RSA_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_ECDH_RSA_WITH_AES_128_CBC_SHA256;
#endif /* if defined( USE_ECDH_RSA_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_ECDH_RSA_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_ECDH_RSA_WITH_AES_256_CBC_SHA384;
#endif /* if defined( USE_ECDH_RSA_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_AES_128_GCM_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256;
#endif /* if defined( USE_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_AES_128_GCM_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_AES_256_GCM_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384;
#endif /* if defined( USE_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_AES_256_GCM_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_ECDH_ECDSA_KEYSCHEME ) && defined( USE_AES_128_GCM_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_ECDH_ECDSA_WITH_AES_128_GCM_SHA256;
#endif /* if defined( USE_ECDH_ECDSA_KEYSCHEME ) && defined( USE_AES_128_GCM_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_ECDH_ECDSA_KEYSCHEME ) && defined( USE_AES_256_GCM_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_ECDH_ECDSA_WITH_AES_256_GCM_SHA384;
#endif /* if defined( USE_ECDH_ECDSA_KEYSCHEME ) && defined( USE_AES_256_GCM_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_ECDHE_RSA_KEYSCHEME ) && defined( USE_AES_128_GCM_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256;
#endif /* if defined( USE_ECDHE_RSA_KEYSCHEME ) && defined( USE_AES_128_GCM_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_ECDHE_RSA_KEYSCHEME ) && defined( USE_AES_256_GCM_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384;
#endif /* if defined( USE_ECDHE_RSA_KEYSCHEME ) && defined( USE_AES_256_GCM_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_ECDH_RSA_KEYSCHEME ) && defined( USE_AES_128_GCM_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_ECDH_RSA_WITH_AES_128_GCM_SHA256;
#endif /* if defined( USE_ECDH_RSA_KEYSCHEME ) && defined( USE_AES_128_GCM_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_ECDH_RSA_KEYSCHEME ) && defined( USE_AES_256_GCM_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_ECDH_RSA_WITH_AES_256_GCM_SHA384;
#endif /* if defined( USE_ECDH_RSA_KEYSCHEME ) && defined( USE_AES_256_GCM_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_ECDHE_PSK_KEYSCHEME ) && defined( USE_RC4_128_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_ECDHE_PSK_WITH_RC4_128_SHA;
#endif /* if defined( USE_ECDHE_PSK_KEYSCHEME ) && defined( USE_RC4_128_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_ECDHE_PSK_KEYSCHEME ) && defined( USE_3DES_EDE_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_ECDHE_PSK_WITH_3DES_EDE_CBC_SHA;
#endif /* if defined( USE_ECDHE_PSK_KEYSCHEME ) && defined( USE_3DES_EDE_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_ECDHE_PSK_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_ECDHE_PSK_WITH_AES_128_CBC_SHA;
#endif /* if defined( USE_ECDHE_PSK_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_ECDHE_PSK_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_ECDHE_PSK_WITH_AES_256_CBC_SHA;
#endif /* if defined( USE_ECDHE_PSK_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_ECDHE_PSK_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_ECDHE_PSK_WITH_AES_128_CBC_SHA256;
#endif /* if defined( USE_ECDHE_PSK_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_ECDHE_PSK_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_ECDHE_PSK_WITH_AES_256_CBC_SHA384;
#endif /* if defined( USE_ECDHE_PSK_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_ECDHE_PSK_KEYSCHEME ) && defined( USE_NULL_CIPHER ) && defined( USE_SHA_MAC )
extern const cipher_suite_t TLS_ECDHE_PSK_WITH_NULL_SHA;
#endif /* if defined( USE_ECDHE_PSK_KEYSCHEME ) && defined( USE_NULL_CIPHER ) && defined( USE_SHA_MAC ) */

#if defined( USE_ECDHE_PSK_KEYSCHEME ) && defined( USE_NULL_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_ECDHE_PSK_WITH_NULL_SHA256;
#endif /* if defined( USE_ECDHE_PSK_KEYSCHEME ) && defined( USE_NULL_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_ECDHE_PSK_KEYSCHEME ) && defined( USE_NULL_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_ECDHE_PSK_WITH_NULL_SHA384;
#endif /* if defined( USE_ECDHE_PSK_KEYSCHEME ) && defined( USE_NULL_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_RSA_KEYSCHEME ) && defined( USE_ARIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_RSA_WITH_ARIA_128_CBC_SHA256;
#endif /* if defined( USE_RSA_KEYSCHEME ) && defined( USE_ARIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_RSA_KEYSCHEME ) && defined( USE_ARIA_256_CBC_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_RSA_WITH_ARIA_256_CBC_SHA384;
#endif /* if defined( USE_RSA_KEYSCHEME ) && defined( USE_ARIA_256_CBC_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_DH_DSS_KEYSCHEME ) && defined( USE_ARIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_DH_DSS_WITH_ARIA_128_CBC_SHA256;
#endif /* if defined( USE_DH_DSS_KEYSCHEME ) && defined( USE_ARIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_DH_DSS_KEYSCHEME ) && defined( USE_ARIA_256_CBC_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_DH_DSS_WITH_ARIA_256_CBC_SHA384;
#endif /* if defined( USE_DH_DSS_KEYSCHEME ) && defined( USE_ARIA_256_CBC_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_DH_RSA_KEYSCHEME ) && defined( USE_ARIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_DH_RSA_WITH_ARIA_128_CBC_SHA256;
#endif /* if defined( USE_DH_RSA_KEYSCHEME ) && defined( USE_ARIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_DH_RSA_KEYSCHEME ) && defined( USE_ARIA_256_CBC_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_DH_RSA_WITH_ARIA_256_CBC_SHA384;
#endif /* if defined( USE_DH_RSA_KEYSCHEME ) && defined( USE_ARIA_256_CBC_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_DHE_DSS_KEYSCHEME ) && defined( USE_ARIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_DHE_DSS_WITH_ARIA_128_CBC_SHA256;
#endif /* if defined( USE_DHE_DSS_KEYSCHEME ) && defined( USE_ARIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_DHE_DSS_KEYSCHEME ) && defined( USE_ARIA_256_CBC_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_DHE_DSS_WITH_ARIA_256_CBC_SHA384;
#endif /* if defined( USE_DHE_DSS_KEYSCHEME ) && defined( USE_ARIA_256_CBC_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_DHE_RSA_KEYSCHEME ) && defined( USE_ARIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_DHE_RSA_WITH_ARIA_128_CBC_SHA256;
#endif /* if defined( USE_DHE_RSA_KEYSCHEME ) && defined( USE_ARIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_DHE_RSA_KEYSCHEME ) && defined( USE_ARIA_256_CBC_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_DHE_RSA_WITH_ARIA_256_CBC_SHA384;
#endif /* if defined( USE_DHE_RSA_KEYSCHEME ) && defined( USE_ARIA_256_CBC_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_DH_anon_KEYSCHEME ) && defined( USE_ARIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_DH_anon_WITH_ARIA_128_CBC_SHA256;
#endif /* if defined( USE_DH_anon_KEYSCHEME ) && defined( USE_ARIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_DH_anon_KEYSCHEME ) && defined( USE_ARIA_256_CBC_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_DH_anon_WITH_ARIA_256_CBC_SHA384;
#endif /* if defined( USE_DH_anon_KEYSCHEME ) && defined( USE_ARIA_256_CBC_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_ARIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_ECDHE_ECDSA_WITH_ARIA_128_CBC_SHA256;
#endif /* if defined( USE_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_ARIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_ARIA_256_CBC_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_ECDHE_ECDSA_WITH_ARIA_256_CBC_SHA384;
#endif /* if defined( USE_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_ARIA_256_CBC_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_ECDH_ECDSA_KEYSCHEME ) && defined( USE_ARIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_ECDH_ECDSA_WITH_ARIA_128_CBC_SHA256;
#endif /* if defined( USE_ECDH_ECDSA_KEYSCHEME ) && defined( USE_ARIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_ECDH_ECDSA_KEYSCHEME ) && defined( USE_ARIA_256_CBC_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_ECDH_ECDSA_WITH_ARIA_256_CBC_SHA384;
#endif /* if defined( USE_ECDH_ECDSA_KEYSCHEME ) && defined( USE_ARIA_256_CBC_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_ECDHE_RSA_KEYSCHEME ) && defined( USE_ARIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_ECDHE_RSA_WITH_ARIA_128_CBC_SHA256;
#endif /* if defined( USE_ECDHE_RSA_KEYSCHEME ) && defined( USE_ARIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_ECDHE_RSA_KEYSCHEME ) && defined( USE_ARIA_256_CBC_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_ECDHE_RSA_WITH_ARIA_256_CBC_SHA384;
#endif /* if defined( USE_ECDHE_RSA_KEYSCHEME ) && defined( USE_ARIA_256_CBC_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_ECDH_RSA_KEYSCHEME ) && defined( USE_ARIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_ECDH_RSA_WITH_ARIA_128_CBC_SHA256;
#endif /* if defined( USE_ECDH_RSA_KEYSCHEME ) && defined( USE_ARIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_ECDH_RSA_KEYSCHEME ) && defined( USE_ARIA_256_CBC_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_ECDH_RSA_WITH_ARIA_256_CBC_SHA384;
#endif /* if defined( USE_ECDH_RSA_KEYSCHEME ) && defined( USE_ARIA_256_CBC_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_RSA_KEYSCHEME ) && defined( USE_ARIA_128_GCM_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_RSA_WITH_ARIA_128_GCM_SHA256;
#endif /* if defined( USE_RSA_KEYSCHEME ) && defined( USE_ARIA_128_GCM_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_RSA_KEYSCHEME ) && defined( USE_ARIA_256_GCM_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_RSA_WITH_ARIA_256_GCM_SHA384;
#endif /* if defined( USE_RSA_KEYSCHEME ) && defined( USE_ARIA_256_GCM_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_DHE_RSA_KEYSCHEME ) && defined( USE_ARIA_128_GCM_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_DHE_RSA_WITH_ARIA_128_GCM_SHA256;
#endif /* if defined( USE_DHE_RSA_KEYSCHEME ) && defined( USE_ARIA_128_GCM_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_DHE_RSA_KEYSCHEME ) && defined( USE_ARIA_256_GCM_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_DHE_RSA_WITH_ARIA_256_GCM_SHA384;
#endif /* if defined( USE_DHE_RSA_KEYSCHEME ) && defined( USE_ARIA_256_GCM_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_DH_RSA_KEYSCHEME ) && defined( USE_ARIA_128_GCM_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_DH_RSA_WITH_ARIA_128_GCM_SHA256;
#endif /* if defined( USE_DH_RSA_KEYSCHEME ) && defined( USE_ARIA_128_GCM_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_DH_RSA_KEYSCHEME ) && defined( USE_ARIA_256_GCM_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_DH_RSA_WITH_ARIA_256_GCM_SHA384;
#endif /* if defined( USE_DH_RSA_KEYSCHEME ) && defined( USE_ARIA_256_GCM_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_DHE_DSS_KEYSCHEME ) && defined( USE_ARIA_128_GCM_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_DHE_DSS_WITH_ARIA_128_GCM_SHA256;
#endif /* if defined( USE_DHE_DSS_KEYSCHEME ) && defined( USE_ARIA_128_GCM_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_DHE_DSS_KEYSCHEME ) && defined( USE_ARIA_256_GCM_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_DHE_DSS_WITH_ARIA_256_GCM_SHA384;
#endif /* if defined( USE_DHE_DSS_KEYSCHEME ) && defined( USE_ARIA_256_GCM_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_DH_DSS_KEYSCHEME ) && defined( USE_ARIA_128_GCM_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_DH_DSS_WITH_ARIA_128_GCM_SHA256;
#endif /* if defined( USE_DH_DSS_KEYSCHEME ) && defined( USE_ARIA_128_GCM_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_DH_DSS_KEYSCHEME ) && defined( USE_ARIA_256_GCM_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_DH_DSS_WITH_ARIA_256_GCM_SHA384;
#endif /* if defined( USE_DH_DSS_KEYSCHEME ) && defined( USE_ARIA_256_GCM_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_DH_anon_KEYSCHEME ) && defined( USE_ARIA_128_GCM_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_DH_anon_WITH_ARIA_128_GCM_SHA256;
#endif /* if defined( USE_DH_anon_KEYSCHEME ) && defined( USE_ARIA_128_GCM_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_DH_anon_KEYSCHEME ) && defined( USE_ARIA_256_GCM_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_DH_anon_WITH_ARIA_256_GCM_SHA384;
#endif /* if defined( USE_DH_anon_KEYSCHEME ) && defined( USE_ARIA_256_GCM_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_ARIA_128_GCM_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_ECDHE_ECDSA_WITH_ARIA_128_GCM_SHA256;
#endif /* if defined( USE_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_ARIA_128_GCM_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_ARIA_256_GCM_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_ECDHE_ECDSA_WITH_ARIA_256_GCM_SHA384;
#endif /* if defined( USE_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_ARIA_256_GCM_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_ECDH_ECDSA_KEYSCHEME ) && defined( USE_ARIA_128_GCM_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_ECDH_ECDSA_WITH_ARIA_128_GCM_SHA256;
#endif /* if defined( USE_ECDH_ECDSA_KEYSCHEME ) && defined( USE_ARIA_128_GCM_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_ECDH_ECDSA_KEYSCHEME ) && defined( USE_ARIA_256_GCM_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_ECDH_ECDSA_WITH_ARIA_256_GCM_SHA384;
#endif /* if defined( USE_ECDH_ECDSA_KEYSCHEME ) && defined( USE_ARIA_256_GCM_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_ECDHE_RSA_KEYSCHEME ) && defined( USE_ARIA_128_GCM_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_ECDHE_RSA_WITH_ARIA_128_GCM_SHA256;
#endif /* if defined( USE_ECDHE_RSA_KEYSCHEME ) && defined( USE_ARIA_128_GCM_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_ECDHE_RSA_KEYSCHEME ) && defined( USE_ARIA_256_GCM_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_ECDHE_RSA_WITH_ARIA_256_GCM_SHA384;
#endif /* if defined( USE_ECDHE_RSA_KEYSCHEME ) && defined( USE_ARIA_256_GCM_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_ECDH_RSA_KEYSCHEME ) && defined( USE_ARIA_128_GCM_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_ECDH_RSA_WITH_ARIA_128_GCM_SHA256;
#endif /* if defined( USE_ECDH_RSA_KEYSCHEME ) && defined( USE_ARIA_128_GCM_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_ECDH_RSA_KEYSCHEME ) && defined( USE_ARIA_256_GCM_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_ECDH_RSA_WITH_ARIA_256_GCM_SHA384;
#endif /* if defined( USE_ECDH_RSA_KEYSCHEME ) && defined( USE_ARIA_256_GCM_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_PSK_KEYSCHEME ) && defined( USE_ARIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_PSK_WITH_ARIA_128_CBC_SHA256;
#endif /* if defined( USE_PSK_KEYSCHEME ) && defined( USE_ARIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_PSK_KEYSCHEME ) && defined( USE_ARIA_256_CBC_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_PSK_WITH_ARIA_256_CBC_SHA384;
#endif /* if defined( USE_PSK_KEYSCHEME ) && defined( USE_ARIA_256_CBC_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_DHE_PSK_KEYSCHEME ) && defined( USE_ARIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_DHE_PSK_WITH_ARIA_128_CBC_SHA256;
#endif /* if defined( USE_DHE_PSK_KEYSCHEME ) && defined( USE_ARIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_DHE_PSK_KEYSCHEME ) && defined( USE_ARIA_256_CBC_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_DHE_PSK_WITH_ARIA_256_CBC_SHA384;
#endif /* if defined( USE_DHE_PSK_KEYSCHEME ) && defined( USE_ARIA_256_CBC_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_RSA_PSK_KEYSCHEME ) && defined( USE_ARIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_RSA_PSK_WITH_ARIA_128_CBC_SHA256;
#endif /* if defined( USE_RSA_PSK_KEYSCHEME ) && defined( USE_ARIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_RSA_PSK_KEYSCHEME ) && defined( USE_ARIA_256_CBC_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_RSA_PSK_WITH_ARIA_256_CBC_SHA384;
#endif /* if defined( USE_RSA_PSK_KEYSCHEME ) && defined( USE_ARIA_256_CBC_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_PSK_KEYSCHEME ) && defined( USE_ARIA_128_GCM_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_PSK_WITH_ARIA_128_GCM_SHA256;
#endif /* if defined( USE_PSK_KEYSCHEME ) && defined( USE_ARIA_128_GCM_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_PSK_KEYSCHEME ) && defined( USE_ARIA_256_GCM_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_PSK_WITH_ARIA_256_GCM_SHA384;
#endif /* if defined( USE_PSK_KEYSCHEME ) && defined( USE_ARIA_256_GCM_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_DHE_PSK_KEYSCHEME ) && defined( USE_ARIA_128_GCM_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_DHE_PSK_WITH_ARIA_128_GCM_SHA256;
#endif /* if defined( USE_DHE_PSK_KEYSCHEME ) && defined( USE_ARIA_128_GCM_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_DHE_PSK_KEYSCHEME ) && defined( USE_ARIA_256_GCM_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_DHE_PSK_WITH_ARIA_256_GCM_SHA384;
#endif /* if defined( USE_DHE_PSK_KEYSCHEME ) && defined( USE_ARIA_256_GCM_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_RSA_PSK_KEYSCHEME ) && defined( USE_ARIA_128_GCM_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_RSA_PSK_WITH_ARIA_128_GCM_SHA256;
#endif /* if defined( USE_RSA_PSK_KEYSCHEME ) && defined( USE_ARIA_128_GCM_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_RSA_PSK_KEYSCHEME ) && defined( USE_ARIA_256_GCM_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_RSA_PSK_WITH_ARIA_256_GCM_SHA384;
#endif /* if defined( USE_RSA_PSK_KEYSCHEME ) && defined( USE_ARIA_256_GCM_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_ECDHE_PSK_KEYSCHEME ) && defined( USE_ARIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_ECDHE_PSK_WITH_ARIA_128_CBC_SHA256;
#endif /* if defined( USE_ECDHE_PSK_KEYSCHEME ) && defined( USE_ARIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_ECDHE_PSK_KEYSCHEME ) && defined( USE_ARIA_256_CBC_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_ECDHE_PSK_WITH_ARIA_256_CBC_SHA384;
#endif /* if defined( USE_ECDHE_PSK_KEYSCHEME ) && defined( USE_ARIA_256_CBC_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_CAMELLIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_ECDHE_ECDSA_WITH_CAMELLIA_128_CBC_SHA256;
#endif /* if defined( USE_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_CAMELLIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_CAMELLIA_256_CBC_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_ECDHE_ECDSA_WITH_CAMELLIA_256_CBC_SHA384;
#endif /* if defined( USE_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_CAMELLIA_256_CBC_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_ECDH_ECDSA_KEYSCHEME ) && defined( USE_CAMELLIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_ECDH_ECDSA_WITH_CAMELLIA_128_CBC_SHA256;
#endif /* if defined( USE_ECDH_ECDSA_KEYSCHEME ) && defined( USE_CAMELLIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_ECDH_ECDSA_KEYSCHEME ) && defined( USE_CAMELLIA_256_CBC_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_ECDH_ECDSA_WITH_CAMELLIA_256_CBC_SHA384;
#endif /* if defined( USE_ECDH_ECDSA_KEYSCHEME ) && defined( USE_CAMELLIA_256_CBC_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_ECDHE_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_ECDHE_RSA_WITH_CAMELLIA_128_CBC_SHA256;
#endif /* if defined( USE_ECDHE_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_ECDHE_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_256_CBC_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_ECDHE_RSA_WITH_CAMELLIA_256_CBC_SHA384;
#endif /* if defined( USE_ECDHE_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_256_CBC_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_ECDH_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_ECDH_RSA_WITH_CAMELLIA_128_CBC_SHA256;
#endif /* if defined( USE_ECDH_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_ECDH_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_256_CBC_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_ECDH_RSA_WITH_CAMELLIA_256_CBC_SHA384;
#endif /* if defined( USE_ECDH_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_256_CBC_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_128_GCM_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_RSA_WITH_CAMELLIA_128_GCM_SHA256;
#endif /* if defined( USE_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_128_GCM_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_256_GCM_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_RSA_WITH_CAMELLIA_256_GCM_SHA384;
#endif /* if defined( USE_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_256_GCM_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_DHE_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_128_GCM_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_DHE_RSA_WITH_CAMELLIA_128_GCM_SHA256;
#endif /* if defined( USE_DHE_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_128_GCM_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_DHE_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_256_GCM_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_DHE_RSA_WITH_CAMELLIA_256_GCM_SHA384;
#endif /* if defined( USE_DHE_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_256_GCM_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_DH_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_128_GCM_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_DH_RSA_WITH_CAMELLIA_128_GCM_SHA256;
#endif /* if defined( USE_DH_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_128_GCM_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_DH_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_256_GCM_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_DH_RSA_WITH_CAMELLIA_256_GCM_SHA384;
#endif /* if defined( USE_DH_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_256_GCM_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_DHE_DSS_KEYSCHEME ) && defined( USE_CAMELLIA_128_GCM_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_DHE_DSS_WITH_CAMELLIA_128_GCM_SHA256;
#endif /* if defined( USE_DHE_DSS_KEYSCHEME ) && defined( USE_CAMELLIA_128_GCM_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_DHE_DSS_KEYSCHEME ) && defined( USE_CAMELLIA_256_GCM_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_DHE_DSS_WITH_CAMELLIA_256_GCM_SHA384;
#endif /* if defined( USE_DHE_DSS_KEYSCHEME ) && defined( USE_CAMELLIA_256_GCM_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_DH_DSS_KEYSCHEME ) && defined( USE_CAMELLIA_128_GCM_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_DH_DSS_WITH_CAMELLIA_128_GCM_SHA256;
#endif /* if defined( USE_DH_DSS_KEYSCHEME ) && defined( USE_CAMELLIA_128_GCM_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_DH_DSS_KEYSCHEME ) && defined( USE_CAMELLIA_256_GCM_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_DH_DSS_WITH_CAMELLIA_256_GCM_SHA384;
#endif /* if defined( USE_DH_DSS_KEYSCHEME ) && defined( USE_CAMELLIA_256_GCM_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_DH_anon_KEYSCHEME ) && defined( USE_CAMELLIA_128_GCM_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_DH_anon_WITH_CAMELLIA_128_GCM_SHA256;
#endif /* if defined( USE_DH_anon_KEYSCHEME ) && defined( USE_CAMELLIA_128_GCM_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_DH_anon_KEYSCHEME ) && defined( USE_CAMELLIA_256_GCM_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_DH_anon_WITH_CAMELLIA_256_GCM_SHA384;
#endif /* if defined( USE_DH_anon_KEYSCHEME ) && defined( USE_CAMELLIA_256_GCM_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_CAMELLIA_128_GCM_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_ECDHE_ECDSA_WITH_CAMELLIA_128_GCM_SHA256;
#endif /* if defined( USE_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_CAMELLIA_128_GCM_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_CAMELLIA_256_GCM_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_ECDHE_ECDSA_WITH_CAMELLIA_256_GCM_SHA384;
#endif /* if defined( USE_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_CAMELLIA_256_GCM_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_ECDH_ECDSA_KEYSCHEME ) && defined( USE_CAMELLIA_128_GCM_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_ECDH_ECDSA_WITH_CAMELLIA_128_GCM_SHA256;
#endif /* if defined( USE_ECDH_ECDSA_KEYSCHEME ) && defined( USE_CAMELLIA_128_GCM_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_ECDH_ECDSA_KEYSCHEME ) && defined( USE_CAMELLIA_256_GCM_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_ECDH_ECDSA_WITH_CAMELLIA_256_GCM_SHA384;
#endif /* if defined( USE_ECDH_ECDSA_KEYSCHEME ) && defined( USE_CAMELLIA_256_GCM_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_ECDHE_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_128_GCM_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_ECDHE_RSA_WITH_CAMELLIA_128_GCM_SHA256;
#endif /* if defined( USE_ECDHE_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_128_GCM_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_ECDHE_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_256_GCM_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_ECDHE_RSA_WITH_CAMELLIA_256_GCM_SHA384;
#endif /* if defined( USE_ECDHE_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_256_GCM_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_ECDH_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_128_GCM_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_ECDH_RSA_WITH_CAMELLIA_128_GCM_SHA256;
#endif /* if defined( USE_ECDH_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_128_GCM_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_ECDH_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_256_GCM_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_ECDH_RSA_WITH_CAMELLIA_256_GCM_SHA384;
#endif /* if defined( USE_ECDH_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_256_GCM_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_PSK_KEYSCHEME ) && defined( USE_CAMELLIA_128_GCM_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_PSK_WITH_CAMELLIA_128_GCM_SHA256;
#endif /* if defined( USE_PSK_KEYSCHEME ) && defined( USE_CAMELLIA_128_GCM_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_PSK_KEYSCHEME ) && defined( USE_CAMELLIA_256_GCM_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_PSK_WITH_CAMELLIA_256_GCM_SHA384;
#endif /* if defined( USE_PSK_KEYSCHEME ) && defined( USE_CAMELLIA_256_GCM_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_DHE_PSK_KEYSCHEME ) && defined( USE_CAMELLIA_128_GCM_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_DHE_PSK_WITH_CAMELLIA_128_GCM_SHA256;
#endif /* if defined( USE_DHE_PSK_KEYSCHEME ) && defined( USE_CAMELLIA_128_GCM_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_DHE_PSK_KEYSCHEME ) && defined( USE_CAMELLIA_256_GCM_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_DHE_PSK_WITH_CAMELLIA_256_GCM_SHA384;
#endif /* if defined( USE_DHE_PSK_KEYSCHEME ) && defined( USE_CAMELLIA_256_GCM_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_RSA_PSK_KEYSCHEME ) && defined( USE_CAMELLIA_128_GCM_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_RSA_PSK_WITH_CAMELLIA_128_GCM_SHA256;
#endif /* if defined( USE_RSA_PSK_KEYSCHEME ) && defined( USE_CAMELLIA_128_GCM_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_RSA_PSK_KEYSCHEME ) && defined( USE_CAMELLIA_256_GCM_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_RSA_PSK_WITH_CAMELLIA_256_GCM_SHA384;
#endif /* if defined( USE_RSA_PSK_KEYSCHEME ) && defined( USE_CAMELLIA_256_GCM_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_PSK_KEYSCHEME ) && defined( USE_CAMELLIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_PSK_WITH_CAMELLIA_128_CBC_SHA256;
#endif /* if defined( USE_PSK_KEYSCHEME ) && defined( USE_CAMELLIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_PSK_KEYSCHEME ) && defined( USE_CAMELLIA_256_CBC_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_PSK_WITH_CAMELLIA_256_CBC_SHA384;
#endif /* if defined( USE_PSK_KEYSCHEME ) && defined( USE_CAMELLIA_256_CBC_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_DHE_PSK_KEYSCHEME ) && defined( USE_CAMELLIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_DHE_PSK_WITH_CAMELLIA_128_CBC_SHA256;
#endif /* if defined( USE_DHE_PSK_KEYSCHEME ) && defined( USE_CAMELLIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_DHE_PSK_KEYSCHEME ) && defined( USE_CAMELLIA_256_CBC_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_DHE_PSK_WITH_CAMELLIA_256_CBC_SHA384;
#endif /* if defined( USE_DHE_PSK_KEYSCHEME ) && defined( USE_CAMELLIA_256_CBC_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_RSA_PSK_KEYSCHEME ) && defined( USE_CAMELLIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_RSA_PSK_WITH_CAMELLIA_128_CBC_SHA256;
#endif /* if defined( USE_RSA_PSK_KEYSCHEME ) && defined( USE_CAMELLIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_RSA_PSK_KEYSCHEME ) && defined( USE_CAMELLIA_256_CBC_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_RSA_PSK_WITH_CAMELLIA_256_CBC_SHA384;
#endif /* if defined( USE_RSA_PSK_KEYSCHEME ) && defined( USE_CAMELLIA_256_CBC_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_ECDHE_PSK_KEYSCHEME ) && defined( USE_CAMELLIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_ECDHE_PSK_WITH_CAMELLIA_128_CBC_SHA256;
#endif /* if defined( USE_ECDHE_PSK_KEYSCHEME ) && defined( USE_CAMELLIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_ECDHE_PSK_KEYSCHEME ) && defined( USE_CAMELLIA_256_CBC_CIPHER ) && defined( USE_SHA384_MAC )
extern const cipher_suite_t TLS_ECDHE_PSK_WITH_CAMELLIA_256_CBC_SHA384;
#endif /* if defined( USE_ECDHE_PSK_KEYSCHEME ) && defined( USE_CAMELLIA_256_CBC_CIPHER ) && defined( USE_SHA384_MAC ) */

#if defined( USE_RSA_KEYSCHEME ) && defined( USE_AES_128_CCM_CIPHER ) && defined( USE_AES_128_CCM_MAC )
extern const cipher_suite_t TLS_RSA_WITH_AES_128_CCM;
#endif /* if defined( USE_RSA_KEYSCHEME ) && defined( USE_AES_128_CCM_CIPHER ) && defined( USE_AES_128_CCM_MAC ) */

#if defined( USE_RSA_KEYSCHEME ) && defined( USE_AES_256_CCM_CIPHER ) && defined( USE_AES_256_CCM_MAC )
extern const cipher_suite_t TLS_RSA_WITH_AES_256_CCM;
#endif /* if defined( USE_RSA_KEYSCHEME ) && defined( USE_AES_256_CCM_CIPHER ) && defined( USE_AES_256_CCM_MAC ) */

#if defined( USE_DHE_RSA_KEYSCHEME ) && defined( USE_AES_128_CCM_CIPHER ) && defined( USE_AES_128_CCM_MAC )
extern const cipher_suite_t TLS_DHE_RSA_WITH_AES_128_CCM;
#endif /* if defined( USE_DHE_RSA_KEYSCHEME ) && defined( USE_AES_128_CCM_CIPHER ) && defined( USE_AES_128_CCM_MAC ) */

#if defined( USE_DHE_RSA_KEYSCHEME ) && defined( USE_AES_256_CCM_CIPHER ) && defined( USE_AES_256_CCM_MAC )
extern const cipher_suite_t TLS_DHE_RSA_WITH_AES_256_CCM;
#endif /* if defined( USE_DHE_RSA_KEYSCHEME ) && defined( USE_AES_256_CCM_CIPHER ) && defined( USE_AES_256_CCM_MAC ) */

#if defined( USE_RSA_KEYSCHEME ) && defined( USE_AES_128_CCM_8_CIPHER ) && defined( USE_AES_128_CCM_8_MAC )
extern const cipher_suite_t TLS_RSA_WITH_AES_128_CCM_8;
#endif /* if defined( USE_RSA_KEYSCHEME ) && defined( USE_AES_128_CCM_8_CIPHER ) && defined( USE_AES_128_CCM_8_MAC ) */

#if defined( USE_RSA_KEYSCHEME ) && defined( USE_AES_256_CCM_8_CIPHER ) && defined( USE_AES_256_CCM_8_MAC )
extern const cipher_suite_t TLS_RSA_WITH_AES_256_CCM_8;
#endif /* if defined( USE_RSA_KEYSCHEME ) && defined( USE_AES_256_CCM_8_CIPHER ) && defined( USE_AES_256_CCM_8_MAC ) */

#if defined( USE_DHE_RSA_KEYSCHEME ) && defined( USE_AES_128_CCM_8_CIPHER ) && defined( USE_AES_128_CCM_8_MAC )
extern const cipher_suite_t TLS_DHE_RSA_WITH_AES_128_CCM_8;
#endif /* if defined( USE_DHE_RSA_KEYSCHEME ) && defined( USE_AES_128_CCM_8_CIPHER ) && defined( USE_AES_128_CCM_8_MAC ) */

#if defined( USE_DHE_RSA_KEYSCHEME ) && defined( USE_AES_256_CCM_8_CIPHER ) && defined( USE_AES_256_CCM_8_MAC )
extern const cipher_suite_t TLS_DHE_RSA_WITH_AES_256_CCM_8;
#endif /* if defined( USE_DHE_RSA_KEYSCHEME ) && defined( USE_AES_256_CCM_8_CIPHER ) && defined( USE_AES_256_CCM_8_MAC ) */

#if defined( USE_PSK_KEYSCHEME ) && defined( USE_AES_128_CCM_CIPHER ) && defined( USE_AES_128_CCM_MAC )
extern const cipher_suite_t TLS_PSK_WITH_AES_128_CCM;
#endif /* if defined( USE_PSK_KEYSCHEME ) && defined( USE_AES_128_CCM_CIPHER ) && defined( USE_AES_128_CCM_MAC ) */

#if defined( USE_PSK_KEYSCHEME ) && defined( USE_AES_256_CCM_CIPHER ) && defined( USE_AES_256_CCM_MAC )
extern const cipher_suite_t TLS_PSK_WITH_AES_256_CCM;
#endif /* if defined( USE_PSK_KEYSCHEME ) && defined( USE_AES_256_CCM_CIPHER ) && defined( USE_AES_256_CCM_MAC ) */

#if defined( USE_DHE_PSK_KEYSCHEME ) && defined( USE_AES_128_CCM_CIPHER ) && defined( USE_AES_128_CCM_MAC )
extern const cipher_suite_t TLS_DHE_PSK_WITH_AES_128_CCM;
#endif /* if defined( USE_DHE_PSK_KEYSCHEME ) && defined( USE_AES_128_CCM_CIPHER ) && defined( USE_AES_128_CCM_MAC ) */

#if defined( USE_DHE_PSK_KEYSCHEME ) && defined( USE_AES_256_CCM_CIPHER ) && defined( USE_AES_256_CCM_MAC )
extern const cipher_suite_t TLS_DHE_PSK_WITH_AES_256_CCM;
#endif /* if defined( USE_DHE_PSK_KEYSCHEME ) && defined( USE_AES_256_CCM_CIPHER ) && defined( USE_AES_256_CCM_MAC ) */

#if defined( USE_PSK_KEYSCHEME ) && defined( USE_AES_128_CCM_8_CIPHER ) && defined( USE_AES_128_CCM_8_MAC )
extern const cipher_suite_t TLS_PSK_WITH_AES_128_CCM_8;
#endif /* if defined( USE_PSK_KEYSCHEME ) && defined( USE_AES_128_CCM_8_CIPHER ) && defined( USE_AES_128_CCM_8_MAC ) */

#if defined( USE_PSK_KEYSCHEME ) && defined( USE_AES_256_CCM_8_CIPHER ) && defined( USE_AES_256_CCM_8_MAC )
extern const cipher_suite_t TLS_PSK_WITH_AES_256_CCM_8;
#endif /* if defined( USE_PSK_KEYSCHEME ) && defined( USE_AES_256_CCM_8_CIPHER ) && defined( USE_AES_256_CCM_8_MAC ) */

#if defined( USE_DHE_PSK_KEYSCHEME ) && defined( USE_AES_128_CCM_8_CIPHER ) && defined( USE_AES_128_CCM_8_MAC )
extern const cipher_suite_t TLS_DHE_PSK_WITH_AES_128_CCM_8;
#endif /* if defined( USE_DHE_PSK_KEYSCHEME ) && defined( USE_AES_128_CCM_8_CIPHER ) && defined( USE_AES_128_CCM_8_MAC ) */

#if defined( USE_DHE_PSK_KEYSCHEME ) && defined( USE_AES_256_CCM_8_CIPHER ) && defined( USE_AES_256_CCM_8_MAC )
extern const cipher_suite_t TLS_DHE_PSK_WITH_AES_256_CCM_8;
#endif /* if defined( USE_DHE_PSK_KEYSCHEME ) && defined( USE_AES_256_CCM_8_CIPHER ) && defined( USE_AES_256_CCM_8_MAC ) */

#if defined( USE_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_AES_128_CCM_CIPHER ) && defined( USE_AES_128_CCM_MAC )
extern const cipher_suite_t TLS_ECDHE_ECDSA_WITH_AES_128_CCM;
#endif /* if defined( USE_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_AES_128_CCM_CIPHER ) && defined( USE_AES_128_CCM_MAC ) */

#if defined( USE_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_AES_256_CCM_CIPHER ) && defined( USE_AES_256_CCM_MAC )
extern const cipher_suite_t TLS_ECDHE_ECDSA_WITH_AES_256_CCM;
#endif /* if defined( USE_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_AES_256_CCM_CIPHER ) && defined( USE_AES_256_CCM_MAC ) */

#if defined( USE_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_AES_128_CCM_8_CIPHER ) && defined( USE_AES_128_CCM_8_MAC )
extern const cipher_suite_t TLS_ECDHE_ECDSA_WITH_AES_128_CCM_8;
#endif /* if defined( USE_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_AES_128_CCM_8_CIPHER ) && defined( USE_AES_128_CCM_8_MAC ) */

#if defined( USE_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_AES_256_CCM_8_CIPHER ) && defined( USE_AES_256_CCM_8_MAC )
extern const cipher_suite_t TLS_ECDHE_ECDSA_WITH_AES_256_CCM_8;
#endif /* if defined( USE_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_AES_256_CCM_8_CIPHER ) && defined( USE_AES_256_CCM_8_MAC ) */

#if defined( USE_ECDHE_RSA_KEYSCHEME ) && defined( USE_CHACHA20_POLY1305_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256;
#endif /* if defined( USE_ECDHE_RSA_KEYSCHEME ) && defined( USE_CHACHA20_POLY1305_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_CHACHA20_POLY1305_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256;
#endif /* if defined( USE_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_CHACHA20_POLY1305_CIPHER ) && defined( USE_SHA256_MAC ) */

#if defined( USE_DHE_RSA_KEYSCHEME ) && defined( USE_CHACHA20_POLY1305_CIPHER ) && defined( USE_SHA256_MAC )
extern const cipher_suite_t TLS_DHE_RSA_WITH_CHACHA20_POLY1305_SHA256;
#endif /* if defined( USE_DHE_RSA_KEYSCHEME ) && defined( USE_CHACHA20_POLY1305_CIPHER ) && defined( USE_SHA256_MAC ) */







#ifdef __cplusplus
} /*extern "C" */
#endif
