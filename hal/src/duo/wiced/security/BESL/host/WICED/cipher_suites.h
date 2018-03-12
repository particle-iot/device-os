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

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

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
struct ssl3_driver;


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

extern const struct cipher_api_t null_cipher_driver;
extern const struct cipher_api_t rc4_40_cipher_driver;
extern const struct cipher_api_t rc4_128_cipher_driver;
extern const struct cipher_api_t rc2_cbc_40_cipher_driver;
extern const struct cipher_api_t idea_cbc_cipher_driver;
extern const struct cipher_api_t des40_cbc_cipher_driver;
extern const struct cipher_api_t des_cbc_cipher_driver;
extern const struct cipher_api_t des_cbc_40_cipher_driver;
extern const struct cipher_api_t triple_des_ede_cbc_cipher_driver;
extern const struct cipher_api_t aes_128_cbc_cipher_driver;
extern const struct cipher_api_t aes_256_cbc_cipher_driver;
extern const struct cipher_api_t aes_128_gcm_cipher_driver;
extern const struct cipher_api_t aes_256_gcm_cipher_driver;
extern const struct cipher_api_t aes_128_ccm_cipher_driver;
extern const struct cipher_api_t aes_256_ccm_cipher_driver;
extern const struct cipher_api_t aes_128_ccm_8_cipher_driver;
extern const struct cipher_api_t aes_256_ccm_8_cipher_driver;
extern const struct cipher_api_t camellia_128_cbc_cipher_driver;
extern const struct cipher_api_t camellia_256_cbc_cipher_driver;
extern const struct cipher_api_t camellia_128_gcm_cipher_driver;
extern const struct cipher_api_t camellia_256_gcm_cipher_driver;
extern const struct cipher_api_t seed_cbc_cipher_driver;
extern const struct cipher_api_t aria_128_cbc_cipher_driver;
extern const struct cipher_api_t aria_256_cbc_cipher_driver;
extern const struct cipher_api_t aria_128_gcm_cipher_driver;
extern const struct cipher_api_t aria_256_gcm_cipher_driver;
extern const struct cipher_api_t chacha20_poly1305_cipher_driver;

extern const struct keyscheme_api_t null_keyscheme_driver;
extern const struct keyscheme_api_t krb5_keyscheme_driver;
extern const struct keyscheme_api_t krb5_export_keyscheme_driver;
extern const struct keyscheme_api_t rsa_export_keyscheme_driver;
extern const struct keyscheme_api_t dh_dss_export_keyscheme_driver;
extern const struct keyscheme_api_t dhe_rsa_export_keyscheme_driver;
extern const struct keyscheme_api_t dh_anon__keyscheme_driver;
extern const struct keyscheme_api_t dh_anon_export_keyscheme_driver;
extern const struct keyscheme_api_t dh_rsa_export_keyscheme_driver;
extern const struct keyscheme_api_t dhe_dss_export_keyscheme_driver;
extern const struct keyscheme_api_t ecdh_anon_keyscheme_driver;
extern const struct keyscheme_api_t psk_keyscheme_driver;
extern const struct keyscheme_api_t rsa_keyscheme_driver;
extern const struct keyscheme_api_t dh_dss_keyscheme_driver;
extern const struct keyscheme_api_t dh_rsa_keyscheme_driver;
extern const struct keyscheme_api_t dhe_dss_keyscheme_driver;
extern const struct keyscheme_api_t dhe_rsa_keyscheme_driver;
extern const struct keyscheme_api_t ecdh_ecdsa_keyscheme_driver;
extern const struct keyscheme_api_t ecdh_rsa_keyscheme_driver;
extern const struct keyscheme_api_t ecdhe_rsa_keyscheme_driver;
extern const struct keyscheme_api_t ecdhe_ecdsa_keyscheme_driver;
extern const struct keyscheme_api_t rsa_psk_keyscheme_driver;
extern const struct keyscheme_api_t dhe_psk_keyscheme_driver;
extern const struct keyscheme_api_t ecdhe_psk_keyscheme_driver;
extern const struct keyscheme_api_t srp_sha_keyscheme_driver;
extern const struct keyscheme_api_t srp_sha_rsa_keyscheme_driver;
extern const struct keyscheme_api_t srp_sha_dss_keyscheme_driver;

extern const struct mac_api_t null_mac_driver;
extern const struct mac_api_t md5_mac_driver;
extern const struct mac_api_t sha_mac_driver;
extern const struct mac_api_t sha256_mac_driver;
extern const struct mac_api_t sha384_mac_driver;
extern const struct mac_api_t aes_128_ccm_mac_driver;
extern const struct mac_api_t aes_256_ccm_mac_driver;
extern const struct mac_api_t aes_128_ccm_8_mac_driver;
extern const struct mac_api_t aes_256_ccm_8_mac_driver;

extern const struct ssl3_driver ssl3_driver_impl;

#ifdef __cplusplus
} /*extern "C" */
#endif
