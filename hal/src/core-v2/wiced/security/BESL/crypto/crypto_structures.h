/*
 * Copyright 2014, Broadcom Corporation
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
extern "C" {
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

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    AES_DECRYPT = 0,
    AES_ENCRYPT = 1,
} aes_mode_type_t;

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

typedef struct bignum_st
{
    unsigned long* d;
    int top;
    int dmax;
    int neg;
    int flags;
} BIGNUM;

typedef struct dh_st
{
    BIGNUM *p;
    BIGNUM *g;
    long length;
    BIGNUM *pub_key;
    BIGNUM *priv_key;
    int flags;
    char *method_mont_p;
    BIGNUM *q;
    BIGNUM *j;
    unsigned char *seed;
    int seedlen;
    BIGNUM *counter;
} DH;

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

typedef struct
{
    int32_t ver;
    int32_t len;

    mpi nN;
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

    int32_t padding;
    int32_t hash_id;
    int32_t (*f_rng)( void * );
    void *p_rng;
} rsa_context;

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
    unsigned char *p;
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
    rsa_context rsa;

    x509_buf issuer_id;
    x509_buf subject_id;
    x509_buf v3_ext;

    int32_t ca_istrue;
    int32_t max_pathlen;

    x509_buf sig_oid2;
    x509_buf sig;

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
    int32_t mode;                   /*!<  encrypt/decrypt   */
    uint32_t sk[96];       /*!<  3DES subkeys      */
}
des3_context;

typedef struct
{
    int32_t mode;                   /*!<  encrypt/decrypt   */
    uint32_t sk[32];       /*!<  DES subkeys       */
}
des_context;

typedef struct
{
    uint32_t total[2];     /*!< number of bytes processed  */
    uint32_t state[8];     /*!< intermediate digest state  */
    unsigned char buffer[64];   /*!< data block being processed */

    unsigned char ipad[64];     /*!< HMAC: inner padding        */
    unsigned char opad[64];     /*!< HMAC: outer padding        */
    int32_t is224;                  /*!< 0 => SHA-256, else SHA-224 */
}
sha2_context;

typedef struct
{
    int64_t total[2];    /*!< number of bytes processed  */
    int64_t state[8];    /*!< intermediate digest state  */
    unsigned char buffer[128];  /*!< data block being processed */

    unsigned char ipad[128];    /*!< HMAC: inner padding        */
    unsigned char opad[128];    /*!< HMAC: outer padding        */
    int32_t is384;                  /*!< 0 => SHA-512, else SHA-384 */
}
sha4_context;

typedef struct
{
    int32_t x;                      /*!< permutation index */
    int32_t y;                      /*!< permutation index */
    unsigned char m[256];       /*!< permutation table */
}
arc4_context;

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
    unsigned long rk[68];   /*!<  CAMELLIA round keys    */
} camellia_context;

typedef struct {
    uint32_t data[32];
} seed_context_t;

typedef unsigned char ed25519_signature[64];
typedef unsigned char ed25519_public_key[32];
typedef unsigned char ed25519_secret_key[32];

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

#ifdef __cplusplus
} /*extern "C" */
#endif
