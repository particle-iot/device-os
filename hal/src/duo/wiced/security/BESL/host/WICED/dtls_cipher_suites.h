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
#include "cipher_suites.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef WICED_USE_CUSTOM_DTLS_CIPHER_SUITES

    /* Default supported key schemes */
    // #define USE_DTLS_PSK_KEYSCHEME
    // #define USE_DTLS_ECDHE_ECDSA_KEYSCHEME

    /* Default supported MACs */
    // #define USE_DTLS_AES_128_CCM_8_MAC

    /* Default supported ciphers */
    // #define USE_DTLS_AES_128_CCM_8_CIPHER

#endif /* #ifndef WICED_USE_CUSTOM_CIPHER_SUITES */


/* Note - The following code was generated using the conv_cipher_suite.pl script */
/* See https://www.iana.org/assignments/tls-parameters/tls-parameters.xhtml#tls-parameters-4 */


#if defined( USE_DTLS_NULL_KEYSCHEME ) && defined( USE_DTLS_NULL_CIPHER ) && defined( USE_DTLS_NULL_MAC )
extern const cipher_suite_t DTLS_NULL_WITH_NULL_NULL;
#endif /* if defined( USE_DTLS_NULL_KEYSCHEME ) && defined( USE_DTLS_NULL_CIPHER ) && defined( USE_DTLS_NULL_MAC ) */

#if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_NULL_CIPHER ) && defined( USE_DTLS_MD5_MAC )
extern const cipher_suite_t DTLS_RSA_WITH_NULL_MD5;
#endif /* if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_NULL_CIPHER ) && defined( USE_DTLS_MD5_MAC ) */

#if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_NULL_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_RSA_WITH_NULL_SHA;
#endif /* if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_NULL_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_RSA_EXPORT_KEYSCHEME ) && defined( USE_DTLS_RC4_40_CIPHER ) && defined( USE_DTLS_MD5_MAC )
extern const cipher_suite_t DTLS_RSA_EXPORT_WITH_RC4_40_MD5;
#endif /* if defined( USE_DTLS_RSA_EXPORT_KEYSCHEME ) && defined( USE_DTLS_RC4_40_CIPHER ) && defined( USE_DTLS_MD5_MAC ) */

#if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_RC4_128_CIPHER ) && defined( USE_DTLS_MD5_MAC )
extern const cipher_suite_t DTLS_RSA_WITH_RC4_128_MD5;
#endif /* if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_RC4_128_CIPHER ) && defined( USE_DTLS_MD5_MAC ) */

#if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_RC4_128_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_RSA_WITH_RC4_128_SHA;
#endif /* if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_RC4_128_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_RSA_EXPORT_KEYSCHEME ) && defined( USE_DTLS_RC2_CBC_40_CIPHER ) && defined( USE_DTLS_MD5_MAC )
extern const cipher_suite_t DTLS_RSA_EXPORT_WITH_RC2_CBC_40_MD5;
#endif /* if defined( USE_DTLS_RSA_EXPORT_KEYSCHEME ) && defined( USE_DTLS_RC2_CBC_40_CIPHER ) && defined( USE_DTLS_MD5_MAC ) */

#if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_IDEA_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_RSA_WITH_IDEA_CBC_SHA;
#endif /* if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_IDEA_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_RSA_EXPORT_KEYSCHEME ) && defined( USE_DTLS_DES40_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_RSA_EXPORT_WITH_DES40_CBC_SHA;
#endif /* if defined( USE_DTLS_RSA_EXPORT_KEYSCHEME ) && defined( USE_DTLS_DES40_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_DES_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_RSA_WITH_DES_CBC_SHA;
#endif /* if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_DES_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_3DES_EDE_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_RSA_WITH_3DES_EDE_CBC_SHA;
#endif /* if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_3DES_EDE_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_DH_DSS_EXPORT_KEYSCHEME ) && defined( USE_DTLS_DES40_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_DH_DSS_EXPORT_WITH_DES40_CBC_SHA;
#endif /* if defined( USE_DTLS_DH_DSS_EXPORT_KEYSCHEME ) && defined( USE_DTLS_DES40_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_DH_DSS_KEYSCHEME ) && defined( USE_DTLS_DES_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_DH_DSS_WITH_DES_CBC_SHA;
#endif /* if defined( USE_DTLS_DH_DSS_KEYSCHEME ) && defined( USE_DTLS_DES_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_DH_DSS_KEYSCHEME ) && defined( USE_DTLS_3DES_EDE_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_DH_DSS_WITH_3DES_EDE_CBC_SHA;
#endif /* if defined( USE_DTLS_DH_DSS_KEYSCHEME ) && defined( USE_DTLS_3DES_EDE_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_DH_RSA_EXPORT_KEYSCHEME ) && defined( USE_DTLS_DES40_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_DH_RSA_EXPORT_WITH_DES40_CBC_SHA;
#endif /* if defined( USE_DTLS_DH_RSA_EXPORT_KEYSCHEME ) && defined( USE_DTLS_DES40_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_DH_RSA_KEYSCHEME ) && defined( USE_DTLS_DES_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_DH_RSA_WITH_DES_CBC_SHA;
#endif /* if defined( USE_DTLS_DH_RSA_KEYSCHEME ) && defined( USE_DTLS_DES_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_DH_RSA_KEYSCHEME ) && defined( USE_DTLS_3DES_EDE_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_DH_RSA_WITH_3DES_EDE_CBC_SHA;
#endif /* if defined( USE_DTLS_DH_RSA_KEYSCHEME ) && defined( USE_DTLS_3DES_EDE_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_DHE_DSS_EXPORT_KEYSCHEME ) && defined( USE_DTLS_DES40_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_DHE_DSS_EXPORT_WITH_DES40_CBC_SHA;
#endif /* if defined( USE_DTLS_DHE_DSS_EXPORT_KEYSCHEME ) && defined( USE_DTLS_DES40_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_DHE_DSS_KEYSCHEME ) && defined( USE_DTLS_DES_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_DHE_DSS_WITH_DES_CBC_SHA;
#endif /* if defined( USE_DTLS_DHE_DSS_KEYSCHEME ) && defined( USE_DTLS_DES_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_DHE_DSS_KEYSCHEME ) && defined( USE_DTLS_3DES_EDE_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_DHE_DSS_WITH_3DES_EDE_CBC_SHA;
#endif /* if defined( USE_DTLS_DHE_DSS_KEYSCHEME ) && defined( USE_DTLS_3DES_EDE_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_DHE_RSA_EXPORT_KEYSCHEME ) && defined( USE_DTLS_DES40_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_DHE_RSA_EXPORT_WITH_DES40_CBC_SHA;
#endif /* if defined( USE_DTLS_DHE_RSA_EXPORT_KEYSCHEME ) && defined( USE_DTLS_DES40_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_DHE_RSA_KEYSCHEME ) && defined( USE_DTLS_DES_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_DHE_RSA_WITH_DES_CBC_SHA;
#endif /* if defined( USE_DTLS_DHE_RSA_KEYSCHEME ) && defined( USE_DTLS_DES_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_DHE_RSA_KEYSCHEME ) && defined( USE_DTLS_3DES_EDE_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_DHE_RSA_WITH_3DES_EDE_CBC_SHA;
#endif /* if defined( USE_DTLS_DHE_RSA_KEYSCHEME ) && defined( USE_DTLS_3DES_EDE_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_DH_anon_EXPORT_KEYSCHEME ) && defined( USE_DTLS_RC4_40_CIPHER ) && defined( USE_DTLS_MD5_MAC )
extern const cipher_suite_t DTLS_DH_anon_EXPORT_WITH_RC4_40_MD5;
#endif /* if defined( USE_DTLS_DH_anon_EXPORT_KEYSCHEME ) && defined( USE_DTLS_RC4_40_CIPHER ) && defined( USE_DTLS_MD5_MAC ) */

#if defined( USE_DTLS_DH_anon_KEYSCHEME ) && defined( USE_DTLS_RC4_128_CIPHER ) && defined( USE_DTLS_MD5_MAC )
extern const cipher_suite_t DTLS_DH_anon_WITH_RC4_128_MD5;
#endif /* if defined( USE_DTLS_DH_anon_KEYSCHEME ) && defined( USE_DTLS_RC4_128_CIPHER ) && defined( USE_DTLS_MD5_MAC ) */

#if defined( USE_DTLS_DH_anon_EXPORT_KEYSCHEME ) && defined( USE_DTLS_DES40_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_DH_anon_EXPORT_WITH_DES40_CBC_SHA;
#endif /* if defined( USE_DTLS_DH_anon_EXPORT_KEYSCHEME ) && defined( USE_DTLS_DES40_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_DH_anon_KEYSCHEME ) && defined( USE_DTLS_DES_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_DH_anon_WITH_DES_CBC_SHA;
#endif /* if defined( USE_DTLS_DH_anon_KEYSCHEME ) && defined( USE_DTLS_DES_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_DH_anon_KEYSCHEME ) && defined( USE_DTLS_3DES_EDE_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_DH_anon_WITH_3DES_EDE_CBC_SHA;
#endif /* if defined( USE_DTLS_DH_anon_KEYSCHEME ) && defined( USE_DTLS_3DES_EDE_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_KRB5_KEYSCHEME ) && defined( USE_DTLS_DES_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_KRB5_WITH_DES_CBC_SHA;
#endif /* if defined( USE_DTLS_KRB5_KEYSCHEME ) && defined( USE_DTLS_DES_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_KRB5_KEYSCHEME ) && defined( USE_DTLS_3DES_EDE_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_KRB5_WITH_3DES_EDE_CBC_SHA;
#endif /* if defined( USE_DTLS_KRB5_KEYSCHEME ) && defined( USE_DTLS_3DES_EDE_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_KRB5_KEYSCHEME ) && defined( USE_DTLS_RC4_128_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_KRB5_WITH_RC4_128_SHA;
#endif /* if defined( USE_DTLS_KRB5_KEYSCHEME ) && defined( USE_DTLS_RC4_128_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_KRB5_KEYSCHEME ) && defined( USE_DTLS_IDEA_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_KRB5_WITH_IDEA_CBC_SHA;
#endif /* if defined( USE_DTLS_KRB5_KEYSCHEME ) && defined( USE_DTLS_IDEA_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_KRB5_KEYSCHEME ) && defined( USE_DTLS_DES_CBC_CIPHER ) && defined( USE_DTLS_MD5_MAC )
extern const cipher_suite_t DTLS_KRB5_WITH_DES_CBC_MD5;
#endif /* if defined( USE_DTLS_KRB5_KEYSCHEME ) && defined( USE_DTLS_DES_CBC_CIPHER ) && defined( USE_DTLS_MD5_MAC ) */

#if defined( USE_DTLS_KRB5_KEYSCHEME ) && defined( USE_DTLS_3DES_EDE_CBC_CIPHER ) && defined( USE_DTLS_MD5_MAC )
extern const cipher_suite_t DTLS_KRB5_WITH_3DES_EDE_CBC_MD5;
#endif /* if defined( USE_DTLS_KRB5_KEYSCHEME ) && defined( USE_DTLS_3DES_EDE_CBC_CIPHER ) && defined( USE_DTLS_MD5_MAC ) */

#if defined( USE_DTLS_KRB5_KEYSCHEME ) && defined( USE_DTLS_RC4_128_CIPHER ) && defined( USE_DTLS_MD5_MAC )
extern const cipher_suite_t DTLS_KRB5_WITH_RC4_128_MD5;
#endif /* if defined( USE_DTLS_KRB5_KEYSCHEME ) && defined( USE_DTLS_RC4_128_CIPHER ) && defined( USE_DTLS_MD5_MAC ) */

#if defined( USE_DTLS_KRB5_KEYSCHEME ) && defined( USE_DTLS_IDEA_CBC_CIPHER ) && defined( USE_DTLS_MD5_MAC )
extern const cipher_suite_t DTLS_KRB5_WITH_IDEA_CBC_MD5;
#endif /* if defined( USE_DTLS_KRB5_KEYSCHEME ) && defined( USE_DTLS_IDEA_CBC_CIPHER ) && defined( USE_DTLS_MD5_MAC ) */

#if defined( USE_DTLS_KRB5_EXPORT_KEYSCHEME ) && defined( USE_DTLS_DES_CBC_40_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_KRB5_EXPORT_WITH_DES_CBC_40_SHA;
#endif /* if defined( USE_DTLS_KRB5_EXPORT_KEYSCHEME ) && defined( USE_DTLS_DES_CBC_40_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_KRB5_EXPORT_KEYSCHEME ) && defined( USE_DTLS_RC2_CBC_40_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_KRB5_EXPORT_WITH_RC2_CBC_40_SHA;
#endif /* if defined( USE_DTLS_KRB5_EXPORT_KEYSCHEME ) && defined( USE_DTLS_RC2_CBC_40_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_KRB5_EXPORT_KEYSCHEME ) && defined( USE_DTLS_RC4_40_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_KRB5_EXPORT_WITH_RC4_40_SHA;
#endif /* if defined( USE_DTLS_KRB5_EXPORT_KEYSCHEME ) && defined( USE_DTLS_RC4_40_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_KRB5_EXPORT_KEYSCHEME ) && defined( USE_DTLS_DES_CBC_40_CIPHER ) && defined( USE_DTLS_MD5_MAC )
extern const cipher_suite_t DTLS_KRB5_EXPORT_WITH_DES_CBC_40_MD5;
#endif /* if defined( USE_DTLS_KRB5_EXPORT_KEYSCHEME ) && defined( USE_DTLS_DES_CBC_40_CIPHER ) && defined( USE_DTLS_MD5_MAC ) */

#if defined( USE_DTLS_KRB5_EXPORT_KEYSCHEME ) && defined( USE_DTLS_RC2_CBC_40_CIPHER ) && defined( USE_DTLS_MD5_MAC )
extern const cipher_suite_t DTLS_KRB5_EXPORT_WITH_RC2_CBC_40_MD5;
#endif /* if defined( USE_DTLS_KRB5_EXPORT_KEYSCHEME ) && defined( USE_DTLS_RC2_CBC_40_CIPHER ) && defined( USE_DTLS_MD5_MAC ) */

#if defined( USE_DTLS_KRB5_EXPORT_KEYSCHEME ) && defined( USE_DTLS_RC4_40_CIPHER ) && defined( USE_DTLS_MD5_MAC )
extern const cipher_suite_t DTLS_KRB5_EXPORT_WITH_RC4_40_MD5;
#endif /* if defined( USE_DTLS_KRB5_EXPORT_KEYSCHEME ) && defined( USE_DTLS_RC4_40_CIPHER ) && defined( USE_DTLS_MD5_MAC ) */

#if defined( USE_DTLS_PSK_KEYSCHEME ) && defined( USE_DTLS_NULL_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_PSK_WITH_NULL_SHA;
#endif /* if defined( USE_DTLS_PSK_KEYSCHEME ) && defined( USE_DTLS_NULL_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_DHE_PSK_KEYSCHEME ) && defined( USE_DTLS_NULL_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_DHE_PSK_WITH_NULL_SHA;
#endif /* if defined( USE_DTLS_DHE_PSK_KEYSCHEME ) && defined( USE_DTLS_NULL_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_RSA_PSK_KEYSCHEME ) && defined( USE_DTLS_NULL_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_RSA_PSK_WITH_NULL_SHA;
#endif /* if defined( USE_DTLS_RSA_PSK_KEYSCHEME ) && defined( USE_DTLS_NULL_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_RSA_WITH_AES_128_CBC_SHA;
#endif /* if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_DH_DSS_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_DH_DSS_WITH_AES_128_CBC_SHA;
#endif /* if defined( USE_DTLS_DH_DSS_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_DH_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_DH_RSA_WITH_AES_128_CBC_SHA;
#endif /* if defined( USE_DTLS_DH_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_DHE_DSS_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_DHE_DSS_WITH_AES_128_CBC_SHA;
#endif /* if defined( USE_DTLS_DHE_DSS_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_DHE_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_DHE_RSA_WITH_AES_128_CBC_SHA;
#endif /* if defined( USE_DTLS_DHE_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_DH_anon_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_DH_anon_WITH_AES_128_CBC_SHA;
#endif /* if defined( USE_DTLS_DH_anon_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_RSA_WITH_AES_256_CBC_SHA;
#endif /* if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_DH_DSS_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_DH_DSS_WITH_AES_256_CBC_SHA;
#endif /* if defined( USE_DTLS_DH_DSS_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_DH_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_DH_RSA_WITH_AES_256_CBC_SHA;
#endif /* if defined( USE_DTLS_DH_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_DHE_DSS_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_DHE_DSS_WITH_AES_256_CBC_SHA;
#endif /* if defined( USE_DTLS_DHE_DSS_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_DHE_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_DHE_RSA_WITH_AES_256_CBC_SHA;
#endif /* if defined( USE_DTLS_DHE_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_DH_anon_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_DH_anon_WITH_AES_256_CBC_SHA;
#endif /* if defined( USE_DTLS_DH_anon_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_NULL_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_RSA_WITH_NULL_SHA256;
#endif /* if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_NULL_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_RSA_WITH_AES_128_CBC_SHA256;
#endif /* if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_RSA_WITH_AES_256_CBC_SHA256;
#endif /* if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_DH_DSS_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_DH_DSS_WITH_AES_128_CBC_SHA256;
#endif /* if defined( USE_DTLS_DH_DSS_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_DH_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_DH_RSA_WITH_AES_128_CBC_SHA256;
#endif /* if defined( USE_DTLS_DH_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_DHE_DSS_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_DHE_DSS_WITH_AES_128_CBC_SHA256;
#endif /* if defined( USE_DTLS_DHE_DSS_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_RSA_WITH_CAMELLIA_128_CBC_SHA;
#endif /* if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_DH_DSS_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_DH_DSS_WITH_CAMELLIA_128_CBC_SHA;
#endif /* if defined( USE_DTLS_DH_DSS_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_DH_RSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_DH_RSA_WITH_CAMELLIA_128_CBC_SHA;
#endif /* if defined( USE_DTLS_DH_RSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_DHE_DSS_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_DHE_DSS_WITH_CAMELLIA_128_CBC_SHA;
#endif /* if defined( USE_DTLS_DHE_DSS_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_DHE_RSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_DHE_RSA_WITH_CAMELLIA_128_CBC_SHA;
#endif /* if defined( USE_DTLS_DHE_RSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_DH_anon_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_DH_anon_WITH_CAMELLIA_128_CBC_SHA;
#endif /* if defined( USE_DTLS_DH_anon_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_DHE_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_DHE_RSA_WITH_AES_128_CBC_SHA256;
#endif /* if defined( USE_DTLS_DHE_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_DH_DSS_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_DH_DSS_WITH_AES_256_CBC_SHA256;
#endif /* if defined( USE_DTLS_DH_DSS_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_DH_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_DH_RSA_WITH_AES_256_CBC_SHA256;
#endif /* if defined( USE_DTLS_DH_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_DHE_DSS_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_DHE_DSS_WITH_AES_256_CBC_SHA256;
#endif /* if defined( USE_DTLS_DHE_DSS_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_DHE_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_DHE_RSA_WITH_AES_256_CBC_SHA256;
#endif /* if defined( USE_DTLS_DHE_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_DH_anon_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_DH_anon_WITH_AES_128_CBC_SHA256;
#endif /* if defined( USE_DTLS_DH_anon_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_DH_anon_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_DH_anon_WITH_AES_256_CBC_SHA256;
#endif /* if defined( USE_DTLS_DH_anon_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_RSA_WITH_CAMELLIA_256_CBC_SHA;
#endif /* if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_DH_DSS_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_DH_DSS_WITH_CAMELLIA_256_CBC_SHA;
#endif /* if defined( USE_DTLS_DH_DSS_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_DH_RSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_DH_RSA_WITH_CAMELLIA_256_CBC_SHA;
#endif /* if defined( USE_DTLS_DH_RSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_DHE_DSS_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_DHE_DSS_WITH_CAMELLIA_256_CBC_SHA;
#endif /* if defined( USE_DTLS_DHE_DSS_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_DHE_RSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_DHE_RSA_WITH_CAMELLIA_256_CBC_SHA;
#endif /* if defined( USE_DTLS_DHE_RSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_DH_anon_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_DH_anon_WITH_CAMELLIA_256_CBC_SHA;
#endif /* if defined( USE_DTLS_DH_anon_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_PSK_KEYSCHEME ) && defined( USE_DTLS_RC4_128_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_PSK_WITH_RC4_128_SHA;
#endif /* if defined( USE_DTLS_PSK_KEYSCHEME ) && defined( USE_DTLS_RC4_128_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_PSK_KEYSCHEME ) && defined( USE_DTLS_3DES_EDE_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_PSK_WITH_3DES_EDE_CBC_SHA;
#endif /* if defined( USE_DTLS_PSK_KEYSCHEME ) && defined( USE_DTLS_3DES_EDE_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_PSK_WITH_AES_128_CBC_SHA;
#endif /* if defined( USE_DTLS_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_PSK_WITH_AES_256_CBC_SHA;
#endif /* if defined( USE_DTLS_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_DHE_PSK_KEYSCHEME ) && defined( USE_DTLS_RC4_128_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_DHE_PSK_WITH_RC4_128_SHA;
#endif /* if defined( USE_DTLS_DHE_PSK_KEYSCHEME ) && defined( USE_DTLS_RC4_128_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_DHE_PSK_KEYSCHEME ) && defined( USE_DTLS_3DES_EDE_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_DHE_PSK_WITH_3DES_EDE_CBC_SHA;
#endif /* if defined( USE_DTLS_DHE_PSK_KEYSCHEME ) && defined( USE_DTLS_3DES_EDE_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_DHE_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_DHE_PSK_WITH_AES_128_CBC_SHA;
#endif /* if defined( USE_DTLS_DHE_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_DHE_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_DHE_PSK_WITH_AES_256_CBC_SHA;
#endif /* if defined( USE_DTLS_DHE_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_RSA_PSK_KEYSCHEME ) && defined( USE_DTLS_RC4_128_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_RSA_PSK_WITH_RC4_128_SHA;
#endif /* if defined( USE_DTLS_RSA_PSK_KEYSCHEME ) && defined( USE_DTLS_RC4_128_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_RSA_PSK_KEYSCHEME ) && defined( USE_DTLS_3DES_EDE_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_RSA_PSK_WITH_3DES_EDE_CBC_SHA;
#endif /* if defined( USE_DTLS_RSA_PSK_KEYSCHEME ) && defined( USE_DTLS_3DES_EDE_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_RSA_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_RSA_PSK_WITH_AES_128_CBC_SHA;
#endif /* if defined( USE_DTLS_RSA_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_RSA_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_RSA_PSK_WITH_AES_256_CBC_SHA;
#endif /* if defined( USE_DTLS_RSA_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_SEED_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_RSA_WITH_SEED_CBC_SHA;
#endif /* if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_SEED_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_DH_DSS_KEYSCHEME ) && defined( USE_DTLS_SEED_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_DH_DSS_WITH_SEED_CBC_SHA;
#endif /* if defined( USE_DTLS_DH_DSS_KEYSCHEME ) && defined( USE_DTLS_SEED_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_DH_RSA_KEYSCHEME ) && defined( USE_DTLS_SEED_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_DH_RSA_WITH_SEED_CBC_SHA;
#endif /* if defined( USE_DTLS_DH_RSA_KEYSCHEME ) && defined( USE_DTLS_SEED_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_DHE_DSS_KEYSCHEME ) && defined( USE_DTLS_SEED_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_DHE_DSS_WITH_SEED_CBC_SHA;
#endif /* if defined( USE_DTLS_DHE_DSS_KEYSCHEME ) && defined( USE_DTLS_SEED_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_DHE_RSA_KEYSCHEME ) && defined( USE_DTLS_SEED_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_DHE_RSA_WITH_SEED_CBC_SHA;
#endif /* if defined( USE_DTLS_DHE_RSA_KEYSCHEME ) && defined( USE_DTLS_SEED_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_DH_anon_KEYSCHEME ) && defined( USE_DTLS_SEED_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_DH_anon_WITH_SEED_CBC_SHA;
#endif /* if defined( USE_DTLS_DH_anon_KEYSCHEME ) && defined( USE_DTLS_SEED_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_RSA_WITH_AES_128_GCM_SHA256;
#endif /* if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_RSA_WITH_AES_256_GCM_SHA384;
#endif /* if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_DHE_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_DHE_RSA_WITH_AES_128_GCM_SHA256;
#endif /* if defined( USE_DTLS_DHE_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_DHE_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_DHE_RSA_WITH_AES_256_GCM_SHA384;
#endif /* if defined( USE_DTLS_DHE_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_DH_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_DH_RSA_WITH_AES_128_GCM_SHA256;
#endif /* if defined( USE_DTLS_DH_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_DH_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_DH_RSA_WITH_AES_256_GCM_SHA384;
#endif /* if defined( USE_DTLS_DH_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_DHE_DSS_KEYSCHEME ) && defined( USE_DTLS_AES_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_DHE_DSS_WITH_AES_128_GCM_SHA256;
#endif /* if defined( USE_DTLS_DHE_DSS_KEYSCHEME ) && defined( USE_DTLS_AES_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_DHE_DSS_KEYSCHEME ) && defined( USE_DTLS_AES_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_DHE_DSS_WITH_AES_256_GCM_SHA384;
#endif /* if defined( USE_DTLS_DHE_DSS_KEYSCHEME ) && defined( USE_DTLS_AES_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_DH_DSS_KEYSCHEME ) && defined( USE_DTLS_AES_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_DH_DSS_WITH_AES_128_GCM_SHA256;
#endif /* if defined( USE_DTLS_DH_DSS_KEYSCHEME ) && defined( USE_DTLS_AES_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_DH_DSS_KEYSCHEME ) && defined( USE_DTLS_AES_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_DH_DSS_WITH_AES_256_GCM_SHA384;
#endif /* if defined( USE_DTLS_DH_DSS_KEYSCHEME ) && defined( USE_DTLS_AES_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_DH_anon_KEYSCHEME ) && defined( USE_DTLS_AES_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_DH_anon_WITH_AES_128_GCM_SHA256;
#endif /* if defined( USE_DTLS_DH_anon_KEYSCHEME ) && defined( USE_DTLS_AES_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_DH_anon_KEYSCHEME ) && defined( USE_DTLS_AES_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_DH_anon_WITH_AES_256_GCM_SHA384;
#endif /* if defined( USE_DTLS_DH_anon_KEYSCHEME ) && defined( USE_DTLS_AES_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_PSK_WITH_AES_128_GCM_SHA256;
#endif /* if defined( USE_DTLS_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_PSK_WITH_AES_256_GCM_SHA384;
#endif /* if defined( USE_DTLS_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_DHE_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_DHE_PSK_WITH_AES_128_GCM_SHA256;
#endif /* if defined( USE_DTLS_DHE_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_DHE_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_DHE_PSK_WITH_AES_256_GCM_SHA384;
#endif /* if defined( USE_DTLS_DHE_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_RSA_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_RSA_PSK_WITH_AES_128_GCM_SHA256;
#endif /* if defined( USE_DTLS_RSA_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_RSA_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_RSA_PSK_WITH_AES_256_GCM_SHA384;
#endif /* if defined( USE_DTLS_RSA_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_PSK_WITH_AES_128_CBC_SHA256;
#endif /* if defined( USE_DTLS_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_PSK_WITH_AES_256_CBC_SHA384;
#endif /* if defined( USE_DTLS_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_PSK_KEYSCHEME ) && defined( USE_DTLS_NULL_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_PSK_WITH_NULL_SHA256;
#endif /* if defined( USE_DTLS_PSK_KEYSCHEME ) && defined( USE_DTLS_NULL_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_PSK_KEYSCHEME ) && defined( USE_DTLS_NULL_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_PSK_WITH_NULL_SHA384;
#endif /* if defined( USE_DTLS_PSK_KEYSCHEME ) && defined( USE_DTLS_NULL_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_DHE_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_DHE_PSK_WITH_AES_128_CBC_SHA256;
#endif /* if defined( USE_DTLS_DHE_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_DHE_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_DHE_PSK_WITH_AES_256_CBC_SHA384;
#endif /* if defined( USE_DTLS_DHE_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_DHE_PSK_KEYSCHEME ) && defined( USE_DTLS_NULL_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_DHE_PSK_WITH_NULL_SHA256;
#endif /* if defined( USE_DTLS_DHE_PSK_KEYSCHEME ) && defined( USE_DTLS_NULL_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_DHE_PSK_KEYSCHEME ) && defined( USE_DTLS_NULL_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_DHE_PSK_WITH_NULL_SHA384;
#endif /* if defined( USE_DTLS_DHE_PSK_KEYSCHEME ) && defined( USE_DTLS_NULL_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_RSA_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_RSA_PSK_WITH_AES_128_CBC_SHA256;
#endif /* if defined( USE_DTLS_RSA_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_RSA_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_RSA_PSK_WITH_AES_256_CBC_SHA384;
#endif /* if defined( USE_DTLS_RSA_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_RSA_PSK_KEYSCHEME ) && defined( USE_DTLS_NULL_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_RSA_PSK_WITH_NULL_SHA256;
#endif /* if defined( USE_DTLS_RSA_PSK_KEYSCHEME ) && defined( USE_DTLS_NULL_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_RSA_PSK_KEYSCHEME ) && defined( USE_DTLS_NULL_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_RSA_PSK_WITH_NULL_SHA384;
#endif /* if defined( USE_DTLS_RSA_PSK_KEYSCHEME ) && defined( USE_DTLS_NULL_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_RSA_WITH_CAMELLIA_128_CBC_SHA256;
#endif /* if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_DH_DSS_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_DH_DSS_WITH_CAMELLIA_128_CBC_SHA256;
#endif /* if defined( USE_DTLS_DH_DSS_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_DH_RSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_DH_RSA_WITH_CAMELLIA_128_CBC_SHA256;
#endif /* if defined( USE_DTLS_DH_RSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_DHE_DSS_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_DHE_DSS_WITH_CAMELLIA_128_CBC_SHA256;
#endif /* if defined( USE_DTLS_DHE_DSS_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_DHE_RSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_DHE_RSA_WITH_CAMELLIA_128_CBC_SHA256;
#endif /* if defined( USE_DTLS_DHE_RSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_DH_anon_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_DH_anon_WITH_CAMELLIA_128_CBC_SHA256;
#endif /* if defined( USE_DTLS_DH_anon_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_RSA_WITH_CAMELLIA_256_CBC_SHA256;
#endif /* if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_DH_DSS_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_DH_DSS_WITH_CAMELLIA_256_CBC_SHA256;
#endif /* if defined( USE_DTLS_DH_DSS_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_DH_RSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_DH_RSA_WITH_CAMELLIA_256_CBC_SHA256;
#endif /* if defined( USE_DTLS_DH_RSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_DHE_DSS_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_DHE_DSS_WITH_CAMELLIA_256_CBC_SHA256;
#endif /* if defined( USE_DTLS_DHE_DSS_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_DHE_RSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_DHE_RSA_WITH_CAMELLIA_256_CBC_SHA256;
#endif /* if defined( USE_DTLS_DHE_RSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_DH_anon_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_DH_anon_WITH_CAMELLIA_256_CBC_SHA256;
#endif /* if defined( USE_DTLS_DH_anon_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_ECDH_ECDSA_KEYSCHEME ) && defined( USE_DTLS_NULL_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_ECDH_ECDSA_WITH_NULL_SHA;
#endif /* if defined( USE_DTLS_ECDH_ECDSA_KEYSCHEME ) && defined( USE_DTLS_NULL_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_ECDH_ECDSA_KEYSCHEME ) && defined( USE_DTLS_RC4_128_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_ECDH_ECDSA_WITH_RC4_128_SHA;
#endif /* if defined( USE_DTLS_ECDH_ECDSA_KEYSCHEME ) && defined( USE_DTLS_RC4_128_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_ECDH_ECDSA_KEYSCHEME ) && defined( USE_DTLS_3DES_EDE_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_ECDH_ECDSA_WITH_3DES_EDE_CBC_SHA;
#endif /* if defined( USE_DTLS_ECDH_ECDSA_KEYSCHEME ) && defined( USE_DTLS_3DES_EDE_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_ECDH_ECDSA_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA;
#endif /* if defined( USE_DTLS_ECDH_ECDSA_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_ECDH_ECDSA_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA;
#endif /* if defined( USE_DTLS_ECDH_ECDSA_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_DTLS_NULL_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_ECDHE_ECDSA_WITH_NULL_SHA;
#endif /* if defined( USE_DTLS_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_DTLS_NULL_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_DTLS_RC4_128_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_ECDHE_ECDSA_WITH_RC4_128_SHA;
#endif /* if defined( USE_DTLS_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_DTLS_RC4_128_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_DTLS_3DES_EDE_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_ECDHE_ECDSA_WITH_3DES_EDE_CBC_SHA;
#endif /* if defined( USE_DTLS_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_DTLS_3DES_EDE_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA;
#endif /* if defined( USE_DTLS_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA;
#endif /* if defined( USE_DTLS_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_ECDH_RSA_KEYSCHEME ) && defined( USE_DTLS_NULL_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_ECDH_RSA_WITH_NULL_SHA;
#endif /* if defined( USE_DTLS_ECDH_RSA_KEYSCHEME ) && defined( USE_DTLS_NULL_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_ECDH_RSA_KEYSCHEME ) && defined( USE_DTLS_RC4_128_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_ECDH_RSA_WITH_RC4_128_SHA;
#endif /* if defined( USE_DTLS_ECDH_RSA_KEYSCHEME ) && defined( USE_DTLS_RC4_128_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_ECDH_RSA_KEYSCHEME ) && defined( USE_DTLS_3DES_EDE_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_ECDH_RSA_WITH_3DES_EDE_CBC_SHA;
#endif /* if defined( USE_DTLS_ECDH_RSA_KEYSCHEME ) && defined( USE_DTLS_3DES_EDE_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_ECDH_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_ECDH_RSA_WITH_AES_128_CBC_SHA;
#endif /* if defined( USE_DTLS_ECDH_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_ECDH_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_ECDH_RSA_WITH_AES_256_CBC_SHA;
#endif /* if defined( USE_DTLS_ECDH_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_ECDHE_RSA_KEYSCHEME ) && defined( USE_DTLS_NULL_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_ECDHE_RSA_WITH_NULL_SHA;
#endif /* if defined( USE_DTLS_ECDHE_RSA_KEYSCHEME ) && defined( USE_DTLS_NULL_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_ECDHE_RSA_KEYSCHEME ) && defined( USE_DTLS_RC4_128_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_ECDHE_RSA_WITH_RC4_128_SHA;
#endif /* if defined( USE_DTLS_ECDHE_RSA_KEYSCHEME ) && defined( USE_DTLS_RC4_128_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_ECDHE_RSA_KEYSCHEME ) && defined( USE_DTLS_3DES_EDE_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_ECDHE_RSA_WITH_3DES_EDE_CBC_SHA;
#endif /* if defined( USE_DTLS_ECDHE_RSA_KEYSCHEME ) && defined( USE_DTLS_3DES_EDE_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_ECDHE_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_ECDHE_RSA_WITH_AES_128_CBC_SHA;
#endif /* if defined( USE_DTLS_ECDHE_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_ECDHE_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_ECDHE_RSA_WITH_AES_256_CBC_SHA;
#endif /* if defined( USE_DTLS_ECDHE_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_ECDH_anon_KEYSCHEME ) && defined( USE_DTLS_NULL_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_ECDH_anon_WITH_NULL_SHA;
#endif /* if defined( USE_DTLS_ECDH_anon_KEYSCHEME ) && defined( USE_DTLS_NULL_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_ECDH_anon_KEYSCHEME ) && defined( USE_DTLS_RC4_128_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_ECDH_anon_WITH_RC4_128_SHA;
#endif /* if defined( USE_DTLS_ECDH_anon_KEYSCHEME ) && defined( USE_DTLS_RC4_128_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_ECDH_anon_KEYSCHEME ) && defined( USE_DTLS_3DES_EDE_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_ECDH_anon_WITH_3DES_EDE_CBC_SHA;
#endif /* if defined( USE_DTLS_ECDH_anon_KEYSCHEME ) && defined( USE_DTLS_3DES_EDE_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_ECDH_anon_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_ECDH_anon_WITH_AES_128_CBC_SHA;
#endif /* if defined( USE_DTLS_ECDH_anon_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_ECDH_anon_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_ECDH_anon_WITH_AES_256_CBC_SHA;
#endif /* if defined( USE_DTLS_ECDH_anon_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_SRP_SHA_KEYSCHEME ) && defined( USE_DTLS_3DES_EDE_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_SRP_SHA_WITH_3DES_EDE_CBC_SHA;
#endif /* if defined( USE_DTLS_SRP_SHA_KEYSCHEME ) && defined( USE_DTLS_3DES_EDE_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_SRP_SHA_RSA_KEYSCHEME ) && defined( USE_DTLS_3DES_EDE_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_SRP_SHA_RSA_WITH_3DES_EDE_CBC_SHA;
#endif /* if defined( USE_DTLS_SRP_SHA_RSA_KEYSCHEME ) && defined( USE_DTLS_3DES_EDE_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_SRP_SHA_DSS_KEYSCHEME ) && defined( USE_DTLS_3DES_EDE_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_SRP_SHA_DSS_WITH_3DES_EDE_CBC_SHA;
#endif /* if defined( USE_DTLS_SRP_SHA_DSS_KEYSCHEME ) && defined( USE_DTLS_3DES_EDE_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_SRP_SHA_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_SRP_SHA_WITH_AES_128_CBC_SHA;
#endif /* if defined( USE_DTLS_SRP_SHA_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_SRP_SHA_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_SRP_SHA_RSA_WITH_AES_128_CBC_SHA;
#endif /* if defined( USE_DTLS_SRP_SHA_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_SRP_SHA_DSS_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_SRP_SHA_DSS_WITH_AES_128_CBC_SHA;
#endif /* if defined( USE_DTLS_SRP_SHA_DSS_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_SRP_SHA_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_SRP_SHA_WITH_AES_256_CBC_SHA;
#endif /* if defined( USE_DTLS_SRP_SHA_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_SRP_SHA_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_SRP_SHA_RSA_WITH_AES_256_CBC_SHA;
#endif /* if defined( USE_DTLS_SRP_SHA_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_SRP_SHA_DSS_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_SRP_SHA_DSS_WITH_AES_256_CBC_SHA;
#endif /* if defined( USE_DTLS_SRP_SHA_DSS_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256;
#endif /* if defined( USE_DTLS_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA384;
#endif /* if defined( USE_DTLS_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_ECDH_ECDSA_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA256;
#endif /* if defined( USE_DTLS_ECDH_ECDSA_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_ECDH_ECDSA_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA384;
#endif /* if defined( USE_DTLS_ECDH_ECDSA_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_ECDHE_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256;
#endif /* if defined( USE_DTLS_ECDHE_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_ECDHE_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384;
#endif /* if defined( USE_DTLS_ECDHE_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_ECDH_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_ECDH_RSA_WITH_AES_128_CBC_SHA256;
#endif /* if defined( USE_DTLS_ECDH_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_ECDH_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_ECDH_RSA_WITH_AES_256_CBC_SHA384;
#endif /* if defined( USE_DTLS_ECDH_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_DTLS_AES_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256;
#endif /* if defined( USE_DTLS_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_DTLS_AES_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_DTLS_AES_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384;
#endif /* if defined( USE_DTLS_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_DTLS_AES_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_ECDH_ECDSA_KEYSCHEME ) && defined( USE_DTLS_AES_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_ECDH_ECDSA_WITH_AES_128_GCM_SHA256;
#endif /* if defined( USE_DTLS_ECDH_ECDSA_KEYSCHEME ) && defined( USE_DTLS_AES_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_ECDH_ECDSA_KEYSCHEME ) && defined( USE_DTLS_AES_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_ECDH_ECDSA_WITH_AES_256_GCM_SHA384;
#endif /* if defined( USE_DTLS_ECDH_ECDSA_KEYSCHEME ) && defined( USE_DTLS_AES_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_ECDHE_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256;
#endif /* if defined( USE_DTLS_ECDHE_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_ECDHE_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384;
#endif /* if defined( USE_DTLS_ECDHE_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_ECDH_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_ECDH_RSA_WITH_AES_128_GCM_SHA256;
#endif /* if defined( USE_DTLS_ECDH_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_ECDH_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_ECDH_RSA_WITH_AES_256_GCM_SHA384;
#endif /* if defined( USE_DTLS_ECDH_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_ECDHE_PSK_KEYSCHEME ) && defined( USE_DTLS_RC4_128_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_ECDHE_PSK_WITH_RC4_128_SHA;
#endif /* if defined( USE_DTLS_ECDHE_PSK_KEYSCHEME ) && defined( USE_DTLS_RC4_128_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_ECDHE_PSK_KEYSCHEME ) && defined( USE_DTLS_3DES_EDE_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_ECDHE_PSK_WITH_3DES_EDE_CBC_SHA;
#endif /* if defined( USE_DTLS_ECDHE_PSK_KEYSCHEME ) && defined( USE_DTLS_3DES_EDE_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_ECDHE_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_ECDHE_PSK_WITH_AES_128_CBC_SHA;
#endif /* if defined( USE_DTLS_ECDHE_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_ECDHE_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_ECDHE_PSK_WITH_AES_256_CBC_SHA;
#endif /* if defined( USE_DTLS_ECDHE_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_ECDHE_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_ECDHE_PSK_WITH_AES_128_CBC_SHA256;
#endif /* if defined( USE_DTLS_ECDHE_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_ECDHE_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_ECDHE_PSK_WITH_AES_256_CBC_SHA384;
#endif /* if defined( USE_DTLS_ECDHE_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_ECDHE_PSK_KEYSCHEME ) && defined( USE_DTLS_NULL_CIPHER ) && defined( USE_DTLS_SHA_MAC )
extern const cipher_suite_t DTLS_ECDHE_PSK_WITH_NULL_SHA;
#endif /* if defined( USE_DTLS_ECDHE_PSK_KEYSCHEME ) && defined( USE_DTLS_NULL_CIPHER ) && defined( USE_DTLS_SHA_MAC ) */

#if defined( USE_DTLS_ECDHE_PSK_KEYSCHEME ) && defined( USE_DTLS_NULL_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_ECDHE_PSK_WITH_NULL_SHA256;
#endif /* if defined( USE_DTLS_ECDHE_PSK_KEYSCHEME ) && defined( USE_DTLS_NULL_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_ECDHE_PSK_KEYSCHEME ) && defined( USE_DTLS_NULL_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_ECDHE_PSK_WITH_NULL_SHA384;
#endif /* if defined( USE_DTLS_ECDHE_PSK_KEYSCHEME ) && defined( USE_DTLS_NULL_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_ARIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_RSA_WITH_ARIA_128_CBC_SHA256;
#endif /* if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_ARIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_ARIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_RSA_WITH_ARIA_256_CBC_SHA384;
#endif /* if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_ARIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_DH_DSS_KEYSCHEME ) && defined( USE_DTLS_ARIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_DH_DSS_WITH_ARIA_128_CBC_SHA256;
#endif /* if defined( USE_DTLS_DH_DSS_KEYSCHEME ) && defined( USE_DTLS_ARIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_DH_DSS_KEYSCHEME ) && defined( USE_DTLS_ARIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_DH_DSS_WITH_ARIA_256_CBC_SHA384;
#endif /* if defined( USE_DTLS_DH_DSS_KEYSCHEME ) && defined( USE_DTLS_ARIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_DH_RSA_KEYSCHEME ) && defined( USE_DTLS_ARIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_DH_RSA_WITH_ARIA_128_CBC_SHA256;
#endif /* if defined( USE_DTLS_DH_RSA_KEYSCHEME ) && defined( USE_DTLS_ARIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_DH_RSA_KEYSCHEME ) && defined( USE_DTLS_ARIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_DH_RSA_WITH_ARIA_256_CBC_SHA384;
#endif /* if defined( USE_DTLS_DH_RSA_KEYSCHEME ) && defined( USE_DTLS_ARIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_DHE_DSS_KEYSCHEME ) && defined( USE_DTLS_ARIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_DHE_DSS_WITH_ARIA_128_CBC_SHA256;
#endif /* if defined( USE_DTLS_DHE_DSS_KEYSCHEME ) && defined( USE_DTLS_ARIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_DHE_DSS_KEYSCHEME ) && defined( USE_DTLS_ARIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_DHE_DSS_WITH_ARIA_256_CBC_SHA384;
#endif /* if defined( USE_DTLS_DHE_DSS_KEYSCHEME ) && defined( USE_DTLS_ARIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_DHE_RSA_KEYSCHEME ) && defined( USE_DTLS_ARIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_DHE_RSA_WITH_ARIA_128_CBC_SHA256;
#endif /* if defined( USE_DTLS_DHE_RSA_KEYSCHEME ) && defined( USE_DTLS_ARIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_DHE_RSA_KEYSCHEME ) && defined( USE_DTLS_ARIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_DHE_RSA_WITH_ARIA_256_CBC_SHA384;
#endif /* if defined( USE_DTLS_DHE_RSA_KEYSCHEME ) && defined( USE_DTLS_ARIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_DH_anon_KEYSCHEME ) && defined( USE_DTLS_ARIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_DH_anon_WITH_ARIA_128_CBC_SHA256;
#endif /* if defined( USE_DTLS_DH_anon_KEYSCHEME ) && defined( USE_DTLS_ARIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_DH_anon_KEYSCHEME ) && defined( USE_DTLS_ARIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_DH_anon_WITH_ARIA_256_CBC_SHA384;
#endif /* if defined( USE_DTLS_DH_anon_KEYSCHEME ) && defined( USE_DTLS_ARIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_DTLS_ARIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_ECDHE_ECDSA_WITH_ARIA_128_CBC_SHA256;
#endif /* if defined( USE_DTLS_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_DTLS_ARIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_DTLS_ARIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_ECDHE_ECDSA_WITH_ARIA_256_CBC_SHA384;
#endif /* if defined( USE_DTLS_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_DTLS_ARIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_ECDH_ECDSA_KEYSCHEME ) && defined( USE_DTLS_ARIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_ECDH_ECDSA_WITH_ARIA_128_CBC_SHA256;
#endif /* if defined( USE_DTLS_ECDH_ECDSA_KEYSCHEME ) && defined( USE_DTLS_ARIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_ECDH_ECDSA_KEYSCHEME ) && defined( USE_DTLS_ARIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_ECDH_ECDSA_WITH_ARIA_256_CBC_SHA384;
#endif /* if defined( USE_DTLS_ECDH_ECDSA_KEYSCHEME ) && defined( USE_DTLS_ARIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_ECDHE_RSA_KEYSCHEME ) && defined( USE_DTLS_ARIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_ECDHE_RSA_WITH_ARIA_128_CBC_SHA256;
#endif /* if defined( USE_DTLS_ECDHE_RSA_KEYSCHEME ) && defined( USE_DTLS_ARIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_ECDHE_RSA_KEYSCHEME ) && defined( USE_DTLS_ARIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_ECDHE_RSA_WITH_ARIA_256_CBC_SHA384;
#endif /* if defined( USE_DTLS_ECDHE_RSA_KEYSCHEME ) && defined( USE_DTLS_ARIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_ECDH_RSA_KEYSCHEME ) && defined( USE_DTLS_ARIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_ECDH_RSA_WITH_ARIA_128_CBC_SHA256;
#endif /* if defined( USE_DTLS_ECDH_RSA_KEYSCHEME ) && defined( USE_DTLS_ARIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_ECDH_RSA_KEYSCHEME ) && defined( USE_DTLS_ARIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_ECDH_RSA_WITH_ARIA_256_CBC_SHA384;
#endif /* if defined( USE_DTLS_ECDH_RSA_KEYSCHEME ) && defined( USE_DTLS_ARIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_ARIA_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_RSA_WITH_ARIA_128_GCM_SHA256;
#endif /* if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_ARIA_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_ARIA_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_RSA_WITH_ARIA_256_GCM_SHA384;
#endif /* if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_ARIA_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_DHE_RSA_KEYSCHEME ) && defined( USE_DTLS_ARIA_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_DHE_RSA_WITH_ARIA_128_GCM_SHA256;
#endif /* if defined( USE_DTLS_DHE_RSA_KEYSCHEME ) && defined( USE_DTLS_ARIA_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_DHE_RSA_KEYSCHEME ) && defined( USE_DTLS_ARIA_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_DHE_RSA_WITH_ARIA_256_GCM_SHA384;
#endif /* if defined( USE_DTLS_DHE_RSA_KEYSCHEME ) && defined( USE_DTLS_ARIA_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_DH_RSA_KEYSCHEME ) && defined( USE_DTLS_ARIA_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_DH_RSA_WITH_ARIA_128_GCM_SHA256;
#endif /* if defined( USE_DTLS_DH_RSA_KEYSCHEME ) && defined( USE_DTLS_ARIA_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_DH_RSA_KEYSCHEME ) && defined( USE_DTLS_ARIA_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_DH_RSA_WITH_ARIA_256_GCM_SHA384;
#endif /* if defined( USE_DTLS_DH_RSA_KEYSCHEME ) && defined( USE_DTLS_ARIA_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_DHE_DSS_KEYSCHEME ) && defined( USE_DTLS_ARIA_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_DHE_DSS_WITH_ARIA_128_GCM_SHA256;
#endif /* if defined( USE_DTLS_DHE_DSS_KEYSCHEME ) && defined( USE_DTLS_ARIA_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_DHE_DSS_KEYSCHEME ) && defined( USE_DTLS_ARIA_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_DHE_DSS_WITH_ARIA_256_GCM_SHA384;
#endif /* if defined( USE_DTLS_DHE_DSS_KEYSCHEME ) && defined( USE_DTLS_ARIA_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_DH_DSS_KEYSCHEME ) && defined( USE_DTLS_ARIA_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_DH_DSS_WITH_ARIA_128_GCM_SHA256;
#endif /* if defined( USE_DTLS_DH_DSS_KEYSCHEME ) && defined( USE_DTLS_ARIA_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_DH_DSS_KEYSCHEME ) && defined( USE_DTLS_ARIA_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_DH_DSS_WITH_ARIA_256_GCM_SHA384;
#endif /* if defined( USE_DTLS_DH_DSS_KEYSCHEME ) && defined( USE_DTLS_ARIA_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_DH_anon_KEYSCHEME ) && defined( USE_DTLS_ARIA_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_DH_anon_WITH_ARIA_128_GCM_SHA256;
#endif /* if defined( USE_DTLS_DH_anon_KEYSCHEME ) && defined( USE_DTLS_ARIA_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_DH_anon_KEYSCHEME ) && defined( USE_DTLS_ARIA_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_DH_anon_WITH_ARIA_256_GCM_SHA384;
#endif /* if defined( USE_DTLS_DH_anon_KEYSCHEME ) && defined( USE_DTLS_ARIA_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_DTLS_ARIA_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_ECDHE_ECDSA_WITH_ARIA_128_GCM_SHA256;
#endif /* if defined( USE_DTLS_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_DTLS_ARIA_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_DTLS_ARIA_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_ECDHE_ECDSA_WITH_ARIA_256_GCM_SHA384;
#endif /* if defined( USE_DTLS_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_DTLS_ARIA_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_ECDH_ECDSA_KEYSCHEME ) && defined( USE_DTLS_ARIA_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_ECDH_ECDSA_WITH_ARIA_128_GCM_SHA256;
#endif /* if defined( USE_DTLS_ECDH_ECDSA_KEYSCHEME ) && defined( USE_DTLS_ARIA_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_ECDH_ECDSA_KEYSCHEME ) && defined( USE_DTLS_ARIA_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_ECDH_ECDSA_WITH_ARIA_256_GCM_SHA384;
#endif /* if defined( USE_DTLS_ECDH_ECDSA_KEYSCHEME ) && defined( USE_DTLS_ARIA_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_ECDHE_RSA_KEYSCHEME ) && defined( USE_DTLS_ARIA_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_ECDHE_RSA_WITH_ARIA_128_GCM_SHA256;
#endif /* if defined( USE_DTLS_ECDHE_RSA_KEYSCHEME ) && defined( USE_DTLS_ARIA_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_ECDHE_RSA_KEYSCHEME ) && defined( USE_DTLS_ARIA_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_ECDHE_RSA_WITH_ARIA_256_GCM_SHA384;
#endif /* if defined( USE_DTLS_ECDHE_RSA_KEYSCHEME ) && defined( USE_DTLS_ARIA_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_ECDH_RSA_KEYSCHEME ) && defined( USE_DTLS_ARIA_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_ECDH_RSA_WITH_ARIA_128_GCM_SHA256;
#endif /* if defined( USE_DTLS_ECDH_RSA_KEYSCHEME ) && defined( USE_DTLS_ARIA_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_ECDH_RSA_KEYSCHEME ) && defined( USE_DTLS_ARIA_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_ECDH_RSA_WITH_ARIA_256_GCM_SHA384;
#endif /* if defined( USE_DTLS_ECDH_RSA_KEYSCHEME ) && defined( USE_DTLS_ARIA_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_PSK_KEYSCHEME ) && defined( USE_DTLS_ARIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_PSK_WITH_ARIA_128_CBC_SHA256;
#endif /* if defined( USE_DTLS_PSK_KEYSCHEME ) && defined( USE_DTLS_ARIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_PSK_KEYSCHEME ) && defined( USE_DTLS_ARIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_PSK_WITH_ARIA_256_CBC_SHA384;
#endif /* if defined( USE_DTLS_PSK_KEYSCHEME ) && defined( USE_DTLS_ARIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_DHE_PSK_KEYSCHEME ) && defined( USE_DTLS_ARIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_DHE_PSK_WITH_ARIA_128_CBC_SHA256;
#endif /* if defined( USE_DTLS_DHE_PSK_KEYSCHEME ) && defined( USE_DTLS_ARIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_DHE_PSK_KEYSCHEME ) && defined( USE_DTLS_ARIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_DHE_PSK_WITH_ARIA_256_CBC_SHA384;
#endif /* if defined( USE_DTLS_DHE_PSK_KEYSCHEME ) && defined( USE_DTLS_ARIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_RSA_PSK_KEYSCHEME ) && defined( USE_DTLS_ARIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_RSA_PSK_WITH_ARIA_128_CBC_SHA256;
#endif /* if defined( USE_DTLS_RSA_PSK_KEYSCHEME ) && defined( USE_DTLS_ARIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_RSA_PSK_KEYSCHEME ) && defined( USE_DTLS_ARIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_RSA_PSK_WITH_ARIA_256_CBC_SHA384;
#endif /* if defined( USE_DTLS_RSA_PSK_KEYSCHEME ) && defined( USE_DTLS_ARIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_PSK_KEYSCHEME ) && defined( USE_DTLS_ARIA_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_PSK_WITH_ARIA_128_GCM_SHA256;
#endif /* if defined( USE_DTLS_PSK_KEYSCHEME ) && defined( USE_DTLS_ARIA_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_PSK_KEYSCHEME ) && defined( USE_DTLS_ARIA_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_PSK_WITH_ARIA_256_GCM_SHA384;
#endif /* if defined( USE_DTLS_PSK_KEYSCHEME ) && defined( USE_DTLS_ARIA_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_DHE_PSK_KEYSCHEME ) && defined( USE_DTLS_ARIA_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_DHE_PSK_WITH_ARIA_128_GCM_SHA256;
#endif /* if defined( USE_DTLS_DHE_PSK_KEYSCHEME ) && defined( USE_DTLS_ARIA_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_DHE_PSK_KEYSCHEME ) && defined( USE_DTLS_ARIA_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_DHE_PSK_WITH_ARIA_256_GCM_SHA384;
#endif /* if defined( USE_DTLS_DHE_PSK_KEYSCHEME ) && defined( USE_DTLS_ARIA_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_RSA_PSK_KEYSCHEME ) && defined( USE_DTLS_ARIA_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_RSA_PSK_WITH_ARIA_128_GCM_SHA256;
#endif /* if defined( USE_DTLS_RSA_PSK_KEYSCHEME ) && defined( USE_DTLS_ARIA_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_RSA_PSK_KEYSCHEME ) && defined( USE_DTLS_ARIA_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_RSA_PSK_WITH_ARIA_256_GCM_SHA384;
#endif /* if defined( USE_DTLS_RSA_PSK_KEYSCHEME ) && defined( USE_DTLS_ARIA_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_ECDHE_PSK_KEYSCHEME ) && defined( USE_DTLS_ARIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_ECDHE_PSK_WITH_ARIA_128_CBC_SHA256;
#endif /* if defined( USE_DTLS_ECDHE_PSK_KEYSCHEME ) && defined( USE_DTLS_ARIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_ECDHE_PSK_KEYSCHEME ) && defined( USE_DTLS_ARIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_ECDHE_PSK_WITH_ARIA_256_CBC_SHA384;
#endif /* if defined( USE_DTLS_ECDHE_PSK_KEYSCHEME ) && defined( USE_DTLS_ARIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_ECDHE_ECDSA_WITH_CAMELLIA_128_CBC_SHA256;
#endif /* if defined( USE_DTLS_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_ECDHE_ECDSA_WITH_CAMELLIA_256_CBC_SHA384;
#endif /* if defined( USE_DTLS_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_ECDH_ECDSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_ECDH_ECDSA_WITH_CAMELLIA_128_CBC_SHA256;
#endif /* if defined( USE_DTLS_ECDH_ECDSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_ECDH_ECDSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_ECDH_ECDSA_WITH_CAMELLIA_256_CBC_SHA384;
#endif /* if defined( USE_DTLS_ECDH_ECDSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_ECDHE_RSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_ECDHE_RSA_WITH_CAMELLIA_128_CBC_SHA256;
#endif /* if defined( USE_DTLS_ECDHE_RSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_ECDHE_RSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_ECDHE_RSA_WITH_CAMELLIA_256_CBC_SHA384;
#endif /* if defined( USE_DTLS_ECDHE_RSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_ECDH_RSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_ECDH_RSA_WITH_CAMELLIA_128_CBC_SHA256;
#endif /* if defined( USE_DTLS_ECDH_RSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_ECDH_RSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_ECDH_RSA_WITH_CAMELLIA_256_CBC_SHA384;
#endif /* if defined( USE_DTLS_ECDH_RSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_RSA_WITH_CAMELLIA_128_GCM_SHA256;
#endif /* if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_RSA_WITH_CAMELLIA_256_GCM_SHA384;
#endif /* if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_DHE_RSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_DHE_RSA_WITH_CAMELLIA_128_GCM_SHA256;
#endif /* if defined( USE_DTLS_DHE_RSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_DHE_RSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_DHE_RSA_WITH_CAMELLIA_256_GCM_SHA384;
#endif /* if defined( USE_DTLS_DHE_RSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_DH_RSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_DH_RSA_WITH_CAMELLIA_128_GCM_SHA256;
#endif /* if defined( USE_DTLS_DH_RSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_DH_RSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_DH_RSA_WITH_CAMELLIA_256_GCM_SHA384;
#endif /* if defined( USE_DTLS_DH_RSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_DHE_DSS_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_DHE_DSS_WITH_CAMELLIA_128_GCM_SHA256;
#endif /* if defined( USE_DTLS_DHE_DSS_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_DHE_DSS_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_DHE_DSS_WITH_CAMELLIA_256_GCM_SHA384;
#endif /* if defined( USE_DTLS_DHE_DSS_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_DH_DSS_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_DH_DSS_WITH_CAMELLIA_128_GCM_SHA256;
#endif /* if defined( USE_DTLS_DH_DSS_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_DH_DSS_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_DH_DSS_WITH_CAMELLIA_256_GCM_SHA384;
#endif /* if defined( USE_DTLS_DH_DSS_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_DH_anon_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_DH_anon_WITH_CAMELLIA_128_GCM_SHA256;
#endif /* if defined( USE_DTLS_DH_anon_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_DH_anon_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_DH_anon_WITH_CAMELLIA_256_GCM_SHA384;
#endif /* if defined( USE_DTLS_DH_anon_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_ECDHE_ECDSA_WITH_CAMELLIA_128_GCM_SHA256;
#endif /* if defined( USE_DTLS_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_ECDHE_ECDSA_WITH_CAMELLIA_256_GCM_SHA384;
#endif /* if defined( USE_DTLS_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_ECDH_ECDSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_ECDH_ECDSA_WITH_CAMELLIA_128_GCM_SHA256;
#endif /* if defined( USE_DTLS_ECDH_ECDSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_ECDH_ECDSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_ECDH_ECDSA_WITH_CAMELLIA_256_GCM_SHA384;
#endif /* if defined( USE_DTLS_ECDH_ECDSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_ECDHE_RSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_ECDHE_RSA_WITH_CAMELLIA_128_GCM_SHA256;
#endif /* if defined( USE_DTLS_ECDHE_RSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_ECDHE_RSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_ECDHE_RSA_WITH_CAMELLIA_256_GCM_SHA384;
#endif /* if defined( USE_DTLS_ECDHE_RSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_ECDH_RSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_ECDH_RSA_WITH_CAMELLIA_128_GCM_SHA256;
#endif /* if defined( USE_DTLS_ECDH_RSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_ECDH_RSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_ECDH_RSA_WITH_CAMELLIA_256_GCM_SHA384;
#endif /* if defined( USE_DTLS_ECDH_RSA_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_PSK_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_PSK_WITH_CAMELLIA_128_GCM_SHA256;
#endif /* if defined( USE_DTLS_PSK_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_PSK_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_PSK_WITH_CAMELLIA_256_GCM_SHA384;
#endif /* if defined( USE_DTLS_PSK_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_DHE_PSK_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_DHE_PSK_WITH_CAMELLIA_128_GCM_SHA256;
#endif /* if defined( USE_DTLS_DHE_PSK_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_DHE_PSK_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_DHE_PSK_WITH_CAMELLIA_256_GCM_SHA384;
#endif /* if defined( USE_DTLS_DHE_PSK_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_RSA_PSK_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_RSA_PSK_WITH_CAMELLIA_128_GCM_SHA256;
#endif /* if defined( USE_DTLS_RSA_PSK_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_GCM_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_RSA_PSK_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_RSA_PSK_WITH_CAMELLIA_256_GCM_SHA384;
#endif /* if defined( USE_DTLS_RSA_PSK_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_GCM_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_PSK_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_PSK_WITH_CAMELLIA_128_CBC_SHA256;
#endif /* if defined( USE_DTLS_PSK_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_PSK_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_PSK_WITH_CAMELLIA_256_CBC_SHA384;
#endif /* if defined( USE_DTLS_PSK_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_DHE_PSK_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_DHE_PSK_WITH_CAMELLIA_128_CBC_SHA256;
#endif /* if defined( USE_DTLS_DHE_PSK_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_DHE_PSK_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_DHE_PSK_WITH_CAMELLIA_256_CBC_SHA384;
#endif /* if defined( USE_DTLS_DHE_PSK_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_RSA_PSK_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_RSA_PSK_WITH_CAMELLIA_128_CBC_SHA256;
#endif /* if defined( USE_DTLS_RSA_PSK_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_RSA_PSK_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_RSA_PSK_WITH_CAMELLIA_256_CBC_SHA384;
#endif /* if defined( USE_DTLS_RSA_PSK_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_ECDHE_PSK_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_ECDHE_PSK_WITH_CAMELLIA_128_CBC_SHA256;
#endif /* if defined( USE_DTLS_ECDHE_PSK_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_128_CBC_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_ECDHE_PSK_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC )
extern const cipher_suite_t DTLS_ECDHE_PSK_WITH_CAMELLIA_256_CBC_SHA384;
#endif /* if defined( USE_DTLS_ECDHE_PSK_KEYSCHEME ) && defined( USE_DTLS_CAMELLIA_256_CBC_CIPHER ) && defined( USE_DTLS_SHA384_MAC ) */

#if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_128_CCM_CIPHER ) && defined( USE_DTLS_AES_128_CCM_MAC )
extern const cipher_suite_t DTLS_RSA_WITH_AES_128_CCM;
#endif /* if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_128_CCM_CIPHER ) && defined( USE_DTLS_AES_128_CCM_MAC ) */

#if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_256_CCM_CIPHER ) && defined( USE_DTLS_AES_256_CCM_MAC )
extern const cipher_suite_t DTLS_RSA_WITH_AES_256_CCM;
#endif /* if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_256_CCM_CIPHER ) && defined( USE_DTLS_AES_256_CCM_MAC ) */

#if defined( USE_DTLS_DHE_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_128_CCM_CIPHER ) && defined( USE_DTLS_AES_128_CCM_MAC )
extern const cipher_suite_t DTLS_DHE_RSA_WITH_AES_128_CCM;
#endif /* if defined( USE_DTLS_DHE_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_128_CCM_CIPHER ) && defined( USE_DTLS_AES_128_CCM_MAC ) */

#if defined( USE_DTLS_DHE_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_256_CCM_CIPHER ) && defined( USE_DTLS_AES_256_CCM_MAC )
extern const cipher_suite_t DTLS_DHE_RSA_WITH_AES_256_CCM;
#endif /* if defined( USE_DTLS_DHE_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_256_CCM_CIPHER ) && defined( USE_DTLS_AES_256_CCM_MAC ) */

#if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_128_CCM_8_CIPHER ) && defined( USE_DTLS_AES_128_CCM_8_MAC )
extern const cipher_suite_t DTLS_RSA_WITH_AES_128_CCM_8;
#endif /* if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_128_CCM_8_CIPHER ) && defined( USE_DTLS_AES_128_CCM_8_MAC ) */

#if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_256_CCM_8_CIPHER ) && defined( USE_DTLS_AES_256_CCM_8_MAC )
extern const cipher_suite_t DTLS_RSA_WITH_AES_256_CCM_8;
#endif /* if defined( USE_DTLS_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_256_CCM_8_CIPHER ) && defined( USE_DTLS_AES_256_CCM_8_MAC ) */

#if defined( USE_DTLS_DHE_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_128_CCM_8_CIPHER ) && defined( USE_DTLS_AES_128_CCM_8_MAC )
extern const cipher_suite_t DTLS_DHE_RSA_WITH_AES_128_CCM_8;
#endif /* if defined( USE_DTLS_DHE_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_128_CCM_8_CIPHER ) && defined( USE_DTLS_AES_128_CCM_8_MAC ) */

#if defined( USE_DTLS_DHE_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_256_CCM_8_CIPHER ) && defined( USE_DTLS_AES_256_CCM_8_MAC )
extern const cipher_suite_t DTLS_DHE_RSA_WITH_AES_256_CCM_8;
#endif /* if defined( USE_DTLS_DHE_RSA_KEYSCHEME ) && defined( USE_DTLS_AES_256_CCM_8_CIPHER ) && defined( USE_DTLS_AES_256_CCM_8_MAC ) */

#if defined( USE_DTLS_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_128_CCM_CIPHER ) && defined( USE_DTLS_AES_128_CCM_MAC )
extern const cipher_suite_t DTLS_PSK_WITH_AES_128_CCM;
#endif /* if defined( USE_DTLS_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_128_CCM_CIPHER ) && defined( USE_DTLS_AES_128_CCM_MAC ) */

#if defined( USE_DTLS_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_256_CCM_CIPHER ) && defined( USE_DTLS_AES_256_CCM_MAC )
extern const cipher_suite_t DTLS_PSK_WITH_AES_256_CCM;
#endif /* if defined( USE_DTLS_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_256_CCM_CIPHER ) && defined( USE_DTLS_AES_256_CCM_MAC ) */

#if defined( USE_DTLS_DHE_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_128_CCM_CIPHER ) && defined( USE_DTLS_AES_128_CCM_MAC )
extern const cipher_suite_t DTLS_DHE_PSK_WITH_AES_128_CCM;
#endif /* if defined( USE_DTLS_DHE_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_128_CCM_CIPHER ) && defined( USE_DTLS_AES_128_CCM_MAC ) */

#if defined( USE_DTLS_DHE_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_256_CCM_CIPHER ) && defined( USE_DTLS_AES_256_CCM_MAC )
extern const cipher_suite_t DTLS_DHE_PSK_WITH_AES_256_CCM;
#endif /* if defined( USE_DTLS_DHE_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_256_CCM_CIPHER ) && defined( USE_DTLS_AES_256_CCM_MAC ) */

#if defined( USE_DTLS_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_128_CCM_8_CIPHER ) && defined( USE_DTLS_AES_128_CCM_8_MAC )
extern const cipher_suite_t DTLS_PSK_WITH_AES_128_CCM_8;
#endif /* if defined( USE_DTLS_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_128_CCM_8_CIPHER ) && defined( USE_DTLS_AES_128_CCM_8_MAC ) */

#if defined( USE_DTLS_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_256_CCM_8_CIPHER ) && defined( USE_DTLS_AES_256_CCM_8_MAC )
extern const cipher_suite_t DTLS_PSK_WITH_AES_256_CCM_8;
#endif /* if defined( USE_DTLS_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_256_CCM_8_CIPHER ) && defined( USE_DTLS_AES_256_CCM_8_MAC ) */

#if defined( USE_DTLS_DHE_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_128_CCM_8_CIPHER ) && defined( USE_DTLS_AES_128_CCM_8_MAC )
extern const cipher_suite_t DTLS_DHE_PSK_WITH_AES_128_CCM_8;
#endif /* if defined( USE_DTLS_DHE_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_128_CCM_8_CIPHER ) && defined( USE_DTLS_AES_128_CCM_8_MAC ) */

#if defined( USE_DTLS_DHE_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_256_CCM_8_CIPHER ) && defined( USE_DTLS_AES_256_CCM_8_MAC )
extern const cipher_suite_t DTLS_DHE_PSK_WITH_AES_256_CCM_8;
#endif /* if defined( USE_DTLS_DHE_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_256_CCM_8_CIPHER ) && defined( USE_DTLS_AES_256_CCM_8_MAC ) */

#if defined( USE_DTLS_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_DTLS_AES_128_CCM_CIPHER ) && defined( USE_DTLS_AES_128_CCM_MAC )
extern const cipher_suite_t DTLS_ECDHE_ECDSA_WITH_AES_128_CCM;
#endif /* if defined( USE_DTLS_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_DTLS_AES_128_CCM_CIPHER ) && defined( USE_DTLS_AES_128_CCM_MAC ) */

#if defined( USE_DTLS_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_DTLS_AES_256_CCM_CIPHER ) && defined( USE_DTLS_AES_256_CCM_MAC )
extern const cipher_suite_t DTLS_ECDHE_ECDSA_WITH_AES_256_CCM;
#endif /* if defined( USE_DTLS_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_DTLS_AES_256_CCM_CIPHER ) && defined( USE_DTLS_AES_256_CCM_MAC ) */

#if defined( USE_DTLS_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_DTLS_AES_128_CCM_8_CIPHER ) && defined( USE_DTLS_AES_128_CCM_8_MAC )
extern const cipher_suite_t DTLS_ECDHE_ECDSA_WITH_AES_128_CCM_8;
#endif /* if defined( USE_DTLS_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_DTLS_AES_128_CCM_8_CIPHER ) && defined( USE_DTLS_AES_128_CCM_8_MAC ) */

#if defined( USE_DTLS_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_DTLS_AES_256_CCM_8_CIPHER ) && defined( USE_DTLS_AES_256_CCM_8_MAC )
extern const cipher_suite_t DTLS_ECDHE_ECDSA_WITH_AES_256_CCM_8;
#endif /* if defined( USE_DTLS_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_DTLS_AES_256_CCM_8_CIPHER ) && defined( USE_DTLS_AES_256_CCM_8_MAC ) */

#if defined( USE_DTLS_ECDHE_RSA_KEYSCHEME ) && defined( USE_DTLS_CHACHA20_POLY1305_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256;
#endif /* if defined( USE_DTLS_ECDHE_RSA_KEYSCHEME ) && defined( USE_DTLS_CHACHA20_POLY1305_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_DTLS_CHACHA20_POLY1305_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256;
#endif /* if defined( USE_DTLS_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_DTLS_CHACHA20_POLY1305_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#if defined( USE_DTLS_DHE_RSA_KEYSCHEME ) && defined( USE_DTLS_CHACHA20_POLY1305_CIPHER ) && defined( USE_DTLS_SHA256_MAC )
extern const cipher_suite_t DTLS_DHE_RSA_WITH_CHACHA20_POLY1305_SHA256;
#endif /* if defined( USE_DTLS_DHE_RSA_KEYSCHEME ) && defined( USE_DTLS_CHACHA20_POLY1305_CIPHER ) && defined( USE_DTLS_SHA256_MAC ) */

#ifdef __cplusplus
} /*extern "C" */
#endif
