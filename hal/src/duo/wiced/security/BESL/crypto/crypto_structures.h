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
#include <stddef.h>
#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define CRYPTO_STRUCTURE

#define N_ROW                   4
#define N_COL                   4
#define N_BLOCK   (N_ROW * N_COL)
#define N_MAX_ROUNDS           14

/*
 * maxium ECC key size in byte
 *
 * 144 bytes for sect571k1, sect571r1 (570 bit)
 */
#define ECC_KEY_MAX           144

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    AES_DECRYPT = 0,
    AES_ENCRYPT = 1,
} aes_mode_type_t;

typedef enum
{
    DES_ENCRYPT     = 1,
    DES_DECRYPT     = 0,
} des_mode_t;

/*
 * PKCS#1 constants
 */
typedef enum
{
    RSA_RAW         =  0,
    RSA_MD2         =  2,
    RSA_MD4         =  3,
    RSA_MD5         =  4,
    RSA_SHA1        =  5,
    RSA_SHA224      = 10,
    RSA_SHA256      = 11,
    RSA_SHA384      = 12,
    RSA_SHA512      = 13,
} rsa_hash_id_t;

typedef enum
{
    TLS_RSA_KEY,
    TLS_ECC_KEY,
    TLS_PSK_KEY,
} wiced_tls_key_type_t;

typedef enum
{
    DTLS_PSK_KEY,
} wiced_dtls_key_type_t;

typedef enum
{
    RSA_PUBLIC      = 0,
    RSA_PRIVATE     = 1,
} rsa_mode_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct
{
    uint32_t number_of_rounds;
    uint32_t* round_keys;
    uint32_t unaligned_data[68];
} aes_context_t;

typedef struct
{
    int32_t s;
    int32_t n;
    uint32_t *p;
} mpi;

typedef struct
{
    int32_t len;
    mpi P;
    mpi G;
    mpi X;
    mpi GX;
    mpi GY;
    mpi K;
    mpi RP;
} dhm_context;

typedef struct
{
    uint32_t total[2];
    uint32_t state[4];
    unsigned char buffer[64];

    unsigned char ipad[64];
    unsigned char opad[64];
} md5_context;

typedef struct {
    uint32_t total[2];
    uint32_t state[4];
    unsigned char buffer[64];

    unsigned char ipad[64];
    unsigned char opad[64];
} md4_context;

typedef struct
{
    uint32_t len;
    uint32_t num[1];
} NN_t;

typedef struct
{
    uint32_t len;
    uint32_t num[48];
} wps_NN_t;

typedef enum
{
    RSA_PKCS_V15    = 0,
    RSA_PKCS_V21    = 1,
} rsa_pkcs_padding_t;

typedef struct
{
    wiced_tls_key_type_t type;
    uint32_t             version;
    uint32_t             length;

    mpi N;
    mpi E;

    mpi D;
    mpi P;
    mpi Q;
    mpi DP;
    mpi DQ;
    mpi QP;

    mpi RN;
    mpi RP;
    mpi RQ;

    rsa_pkcs_padding_t padding;
    int32_t hash_id;
    int32_t (*f_rng)( void * );
    void *p_rng;
} wiced_tls_rsa_key_t;

/*
 * We support up to 570-bit ECC keys
 */
typedef struct
{
    wiced_tls_key_type_t type;
    uint32_t             version;
    uint32_t             length;
    uint8_t              key[ECC_KEY_MAX];
} wiced_tls_ecc_key_t;

typedef struct
{
    wiced_tls_key_type_t type;
    uint32_t             version;
    uint32_t             length;
    uint8_t              psk_identity[256];
    uint8_t              psk_key[256];
} wiced_tls_psk_key_t;

typedef wiced_tls_psk_key_t wiced_dtls_psk_key_t;
typedef wiced_tls_rsa_key_t rsa_context;

/*
 * wiced_tls_key_t is a prototype for
 */
typedef struct
{
    wiced_tls_key_type_t type;
    uint32_t             version;
    uint32_t             length;
    uint8_t              data[1];
} wiced_tls_key_t;

typedef struct _x509_buf
{
    int32_t tag;
    uint32_t len;
    const unsigned char *p;
} x509_buf;

typedef struct _x509_buf_allocated
{
    int32_t tag;
    uint32_t len;
    const unsigned char *p;
} x509_buf_allocated;

typedef struct _x509_name
{
    x509_buf oid;
    x509_buf val;
    struct _x509_name *next;
} x509_name;

typedef struct _x509_time
{
    uint16_t year;
    uint8_t  mon;
    uint8_t  day;
    uint8_t  hour;
    uint8_t  min;
    uint8_t  sec;
} x509_time;

typedef struct _x509_cert
{
    x509_buf_allocated raw;
    x509_buf tbs;

    int32_t version;
    x509_buf serial;
    x509_buf sig_oid1;

    x509_buf issuer_raw;
    x509_buf subject_raw;

    x509_name issuer;
    x509_name subject;

    x509_time valid_from;
    x509_time valid_to;

    x509_buf pk_oid;
    wiced_tls_key_t* public_key;

    x509_buf issuer_id;
    x509_buf subject_id;
    x509_buf v3_ext;

    int32_t ca_istrue;
    int32_t max_pathlen;

    x509_buf sig_oid2;
    x509_buf sig;

    uint8_t  der_certificate_malloced;
    uint8_t* der_certificate_data;
    uint32_t der_certificate_length;

    struct _x509_cert *next;
} x509_cert;

typedef struct _x509_node
{
    unsigned char *data;
    unsigned char *p;
    unsigned char *end;
    uint32_t len;
} x509_node;

typedef struct _x509_raw
{
    x509_node raw;
    x509_node tbs;

    x509_node version;
    x509_node serial;
    x509_node tbs_signalg;
    x509_node issuer;
    x509_node validity;
    x509_node subject;
    x509_node subpubkey;

    x509_node signalg;
    x509_node sign;
} x509_raw;

typedef struct SHA256state_st
{
    unsigned int h[8];
    unsigned int Nl, Nh;
    unsigned int data[16];
    unsigned int num, md_len;
} SHA256_CTX;

typedef struct
{
    uint32_t total[2];
    uint32_t state[5];
    unsigned char buffer[64];

    unsigned char ipad[64];
    unsigned char opad[64];
} sha1_context;

typedef struct
{
    uint32_t entropy;
    uint32_t reserved;
} microrng_state;

typedef struct
{
    int32_t mode;          /*!<  encrypt/decrypt   */
    uint32_t sk[96];       /*!<  3DES subkeys      */
} des3_context;

typedef struct
{
    int32_t mode;       /*!<  encrypt/decrypt   */
    uint32_t sk[32];    /*!<  DES subkeys       */
} des_context;

typedef struct
{
    uint32_t total[2];          /*!< number of bytes processed  */
    uint32_t state[8];          /*!< intermediate digest state  */
    unsigned char buffer[64];   /*!< data block being processed */

    unsigned char ipad[64];     /*!< HMAC: inner padding        */
    unsigned char opad[64];     /*!< HMAC: outer padding        */
    int32_t is224;              /*!< 0 => SHA-256, else SHA-224 */
} sha2_context;

typedef struct
{
    int64_t total[2];           /*!< number of bytes processed  */
    int64_t state[8];           /*!< intermediate digest state  */
    unsigned char buffer[128];  /*!< data block being processed */

    unsigned char ipad[128];    /*!< HMAC: inner padding        */
    unsigned char opad[128];    /*!< HMAC: outer padding        */
    int32_t is384;              /*!< 0 => SHA-512, else SHA-384 */
} sha4_context;

typedef struct
{
    int32_t x;                  /*!< permutation index */
    int32_t y;                  /*!< permutation index */
    unsigned char m[256];       /*!< permutation table */
} arc4_context;

typedef struct poly1305_context {
    size_t aligner;
    unsigned char opaque[136];
} poly1305_context;

typedef struct
{
  uint32_t input[16];
} chacha_context_t;

typedef struct {
    int nr;         /*!<  number of rounds  */
    uint32_t rk[68];   /*!<  CAMELLIA round keys    */
} camellia_context;

typedef struct {
    uint32_t data[32];
} seed_context_t;

#ifndef ED25519_H
typedef unsigned char ed25519_signature[64];
typedef unsigned char ed25519_public_key[32];
typedef unsigned char ed25519_secret_key[32];
#endif

typedef wiced_tls_key_t     wiced_dtls_key_t;
typedef wiced_tls_ecc_key_t wiced_dtls_ecc_key_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

#ifdef __cplusplus
} /*extern "C" */
#endif
