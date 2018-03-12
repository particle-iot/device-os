/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#ifndef _CRYPTO_API_H
#define _CRYPTO_API_H

#include "typedefs.h"
#include <sbhnddma.h>
#include <platform_cache_def.h>
#include "platform_constants.h"
#include "wiced_utilities.h"

/******************************************************
 *                      Macros
 ******************************************************/

#define HWCRYPTO_ALIGNMENT          16
#if defined( PLATFORM_L1_CACHE_SHIFT )
#define HWCRYPTO_ALIGNMENT_BYTES    MAX(HWCRYPTO_ALIGNMENT, PLATFORM_L1_CACHE_BYTES)
#else
#define HWCRYPTO_ALIGNMENT_BYTES    HWCRYPTO_ALIGNMENT
#define
#endif /* defined( PLATFORM_L1_CACHE_SHIFT ) */
#define HWCRYPTO_DESCRIPTOR_ALIGNMENT_BYTES HWCRYPTO_ALIGNMENT_BYTES

/*****************************************************
 *                    Constants
 ******************************************************/
#define DES_IV_LEN                      8
#define DES_BLOCK_SZ                    8
#define AES_IV_LEN                      16
#define AES_BLOCK_SZ                    16
#define AES128_KEY_LEN                  16
#define HMAC256_KEY_LEN                 32
#define HMAC224_KEY_LEN                 28
#define SHA256_HASH_SIZE                32
#define SHA224_HASH_SIZE                28
#define HMAC256_OUTPUT_SIZE             32
#define HMAC224_OUTPUT_SIZE             28
#define HMAC_INNERHASHCONTEXT_SIZE      64
#define HMAC_OUTERHASHCONTEXT_SIZE      64
#define HMAC_BLOCK_SIZE                 64
#define HMAC_IPAD                       0x36
#define HMAC_OPAD                       0x5C
#define SHA_BLOCK_SIZE                  64 /* 64bytes = 512 bits */
#define SHA_BLOCK_SIZE_MASK             ( SHA_BLOCK_SIZE - 1 ) /* SHA1/SHA2 Input data should be at least 64bytes aligned */
#define HWCRYPTO_MAX_PAYLOAD_SIZE       PLATFORM_L1_CACHE_ROUND_UP( 63*1024 )

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    OUTBOUND    = 0,
    INBOUND     = 1
} sctx_inbound_t;

typedef enum
{
    HW_AES_ENCRYPT     = 0,
    HW_AES_DECRYPT     = 1
} hw_aes_mode_type_t;

typedef enum
{
    HW_DES_ENCRYPT     = 0,
    HW_DES_DECRYPT     = 1
} hw_des_mode_type_t;

typedef enum
{
    ENCR_AUTH = 0,
    AUTH_ENCR = 1
} sctx_order_t;

typedef enum
{
    CRYPT_NULL  = 0,
    RC4         = 1,
    DES         = 2,
    _3DES       = 3,
    AES         = 4
} sctx_crypt_algo_t;

typedef enum
{
    ECB = 0,
    CBC = 1,
    OFB = 2,
    CFB = 3,
    CTR = 4,
    CCM = 5,
    GCM = 6,
    XTS = 7
} sctx_crypt_mode_t;

typedef enum
{
    NULL_   = 0,
    MD5     = 1,
    SHA1    = 2,
    SHA224  = 3,
    SHA256  = 4,
    AES_H   = 5,
    FHMAC   = 6
} sctx_hash_algo_t;

typedef enum
{
    HASH=0,
    CTXT=1,
    HMAC=2,
    CCM_H=5,
    GCM_H=6
} sctx_hash_mode_t;

typedef enum
{
    HASH_FULL   = 0,
    HASH_INIT   = 1,
    HASH_UPDT   = 2,
    HASH_FINAL  = 3
} sctx_hash_optype_t;

typedef enum
{
    HWCRYPTO_ENCR_ALG_NONE = 0,
    HWCRYPTO_ENCR_ALG_AES_128,
    HWCRYPTO_ENCR_ALG_AES_192,
    HWCRYPTO_ENCR_ALG_AES_256,
    HWCRYPTO_ENCR_ALG_DES,
    HWCRYPTO_ENCR_ALG_3DES,
    HWCRYPTO_ENCR_ALG_RC4_INIT,
    HWCRYPTO_ENCR_ALG_RC4_UPDT
} hwcrypto_encr_alg_t;

typedef enum
{
    HWCRYPTO_ENCR_MODE_NONE = 0,
    HWCRYPTO_ENCR_MODE_CBC,
    HWCRYPTO_ENCR_MODE_ECB ,
    HWCRYPTO_ENCR_MODE_CTR,
    HWCRYPTO_ENCR_MODE_CCM = 5,
    HWCRYPTO_ENCR_MODE_CMAC,
    HWCRYPTO_ENCR_MODE_OFB,
    HWCRYPTO_ENCR_MODE_CFB,
    HWCRYPTO_ENCR_MODE_GCM,
    HWCRYPTO_ENCR_MODE_XTS
} hwcrypto_encr_mode_t;

typedef enum
{
      HWCRYPTO_AUTH_ALG_NULL = 0,
      HWCRYPTO_AUTH_ALG_MD5 ,
      HWCRYPTO_AUTH_ALG_SHA1,
      HWCRYPTO_AUTH_ALG_SHA224,
      HWCRYPTO_AUTH_ALG_SHA256,
      HWCRYPTO_AUTH_ALG_AES
} hwcrypto_auth_alg_t;

typedef enum
{
      HWCRYPTO_AUTH_MODE_HASH = 0,
      HWCRYPTO_AUTH_MODE_CTXT,
      HWCRYPTO_AUTH_MODE_HMAC,
      HWCRYPTO_AUTH_MODE_FHMAC,
      HWCRYPTO_AUTH_MODE_CCM,
      HWCRYPTO_AUTH_MODE_GCM,
      HWCRYPTO_AUTH_MODE_XCBCMAC,
      HWCRYPTO_AUTH_MODE_CMAC
} hwcrypto_auth_mode_t;

typedef enum
{
      HWCRYPTO_AUTH_OPTYPE_FULL = 0,
      HWCRYPTO_AUTH_OPTYPE_INIT,
      HWCRYPTO_AUTH_OPTYPE_UPDATE,
      HWCRYPTO_AUTH_OPTYPE_FINAL,
      HWCRYPTO_AUTH_OPTYPE_HMAC_HASH
} hwcrypto_auth_optype_t;

typedef enum
{
    HWCRYPTO_CIPHER_MODE_NULL = 0,
    HWCRYPTO_CIPHER_MODE_ENCRYPT,
    HWCRYPTO_CIPHER_MODE_DECRYPT,
    HWCRYPTO_CIPHER_MODE_AUTHONLY
} hwcrypto_cipher_mode_t;

typedef enum
{
    HWCRYPTO_CIPHER_ORDER_NULL = 0,
    HWCRYPTO_CIPHER_ORDER_AUTH_CRYPT,
    HWCRYPTO_CIPHER_ORDER_CRYPT_AUTH
} hwcrypto_cipher_order_t;

typedef enum
{
    HWCRYPTO_CIPHER_OPTYPE_RC4_OPTYPE_INIT = 0,
    HWCRYPTO_CIPHER_OPTYPE_RC4_OPTYPE_UPDT,
    HWCRYPTO_CIPHER_OPTYPE_DES_OPTYPE_K56,
    HWCRYPTO_CIPHER_OPTYPE_3DES_OPTYPE_K168EDE,
    HWCRYPTO_CIPHER_OPTYPE_AES_OPTYPE_K128,
    HWCRYPTO_CIPHER_OPTYPE_AES_OPTYPE_K192,
    HWCRYPTO_CIPHER_OPTYPE_AES_OPTYPE_K256
} hwcrypto_cipher_optype_t;

typedef enum
{
    HWCRYPTO_HASHMODE_HASH_HASH = 0,
    HWCRYPTO_HASHMODE_HASH_CTXT,
    HWCRYPTO_HASHMODE_HASH_HMAC,
    HWCRYPTO_HASHMODE_HASH_FHMAC,
    HWCRYPTO_HASHMODE_AES_HASH_XCBC_MAC,
    HWCRYPTO_HASHMODE_AES_HASH_CMAC,
    HWCRYPTO_HASHMODE_AES_HASH_CCM,
    HWCRYPTO_HASHMODE_AES_HASH_GCM
}  hwcrypto_hash_mode_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/



typedef union
{
    uint32_t raw;
    struct
    {
        unsigned int reserved3      : 16;   /* [ 15:0 ] */
        unsigned int opcode         : 8;    /* [ 23:16] */
        unsigned int supdt_present  : 1;    /* [ 24:1 ] */
        unsigned int reserved2      : 1;    /* [ 25:1 ] */
        unsigned int hash_present   : 1;    /* [ 26:1 ] */
        unsigned int bd_present     : 1;    /* [ 27:1 ] */
        unsigned int mfm_present    : 1;    /* [ 28:1 ] */
        unsigned int bdesc_present  : 1;    /* [ 29:1 ] */
        unsigned int reserved1      : 1;    /* [ 30:1 ] */
        unsigned int sctx_present   : 1;    /* [ 31:1 ] */
    } bits;
} cryptofield_message_header_t;

typedef union
{
    uint32_t raw;
    struct
    {
        unsigned int sctx_size      : 8;    /* [ 7:0   ]  */
        unsigned int reserved       : 22;   /* [ 29:8  ]  */
        unsigned int sctx_type      : 2;    /* [ 31:30 ]  */
    } bits;
} cryptofield_sctx1_header_t;

typedef union
{
    uint32_t raw;
    struct
    {
        unsigned int updt_ofst      : 8;    /* [ 7:0   ]  */
        unsigned int hash_optype    : 2;    /* [ 9:8   ]  */
        unsigned int hash_mode      : 3;    /* [ 12:10 ]  */
        unsigned int hash_algo      : 3;    /* [ 15:13 ]  */
        unsigned int crypt_optype   : 2;    /* [ 17:16 ]  */
        unsigned int crypt_mode     : 3;    /* [ 20:18 ]  */
        unsigned int crypt_algo     : 3;    /* [ 23:21 ]  */
        unsigned int reserved       : 6;    /* [ 29:24 ]  */
        unsigned int order          : 1;    /* [ 30    ]  */
        unsigned int inbound        : 1;    /* [ 31    ]  */
    } bits;
} cryptofield_sctx2_header_t;

typedef union
{
    uint32_t raw;
    struct
    {
        unsigned int exp_iv_size    : 3;    /* [ 2:0   ]  */
        unsigned int iv_ov_ofst     : 2;    /* [ 4:3   ]  */
        unsigned int iv_flags       : 3;    /* [ 7:5   ]  */
        unsigned int icv_size       : 4;    /* [ 11:8  ]  */
        unsigned int icv_flags      : 2;    /* [ 13:12 ]  */
        unsigned int reserved1      : 6;    /* [ 19:14 ]  */
        unsigned int key_handle     : 9;    /* [ 28:20 ]  */
        unsigned int reserved2      : 2;    /* [ 30:29 ]  */
        unsigned int protected_key  : 1;    /* [ 31    ]  */
    } bits;
} cryptofield_sctx3_header_t;

typedef struct
{
    unsigned int length_crypto      : 16;    /* [ 15:0  ]  */
    unsigned int offset_crypto      : 16;    /* [ 31:16 ]  */
} cryptofield_bdesc_crypto_t;

typedef struct
{
    unsigned int length_mac         : 16;    /* [ 15:0  ]  */
    unsigned int offset_mac         : 16;    /* [ 31:16 ]  */
} cryptofield_bdesc_mac_t;

typedef struct
{
    unsigned int offset_iv          : 16;    /* [ 15:0  ]  */
    unsigned int offset_icv         : 16;    /* [ 31:16 ]  */
} cryptofield_bdesc_iv_t;

typedef struct
{
    cryptofield_bdesc_iv_t          iv;
    cryptofield_bdesc_crypto_t      crypto;
    cryptofield_bdesc_mac_t         mac;
} cryptofield_bdesc_t;

typedef struct
{
    unsigned int prev_length        : 16;    /* [ 15:0  ]  */
    unsigned int size               : 16;    /* [ 31:16 ]  */
} cryptofield_bd_t;

typedef struct
{
    cryptofield_message_header_t    message_header;
    uint32_t                        extended_header;
    cryptofield_sctx1_header_t      sctx1;
    cryptofield_sctx2_header_t      sctx2;
    cryptofield_sctx3_header_t      sctx3;
    cryptofield_bdesc_t             bdesc;
    cryptofield_bd_t                bd;
} spum_message_fields_t;

typedef struct crypto_cmd
{
    uint8_t*  source;          /* input buffer */
    uint8_t*  output;          /* output buffer */
    uint8_t*  hash_output;     /* buffer to store hash output
                                   (If NULL, hash output is stored at the end of output payload)*/
    uint8_t*  hash_key;        /* HMAC Key / HASH State (For incremental Hash operations) */
    uint32_t  hash_key_len;    /* HMAC Key / HASH State length */
    uint8_t*  crypt_key;       /* Crypt Key */
    uint32_t  crypt_key_len;   /* Crypt Key length */
    uint8_t*  crypt_iv;        /* Crypt IV */
    uint32_t  crypt_iv_len;    /* Crypt IV length */
    spum_message_fields_t msg; /* SPU-M (HWcrypto) message structure */

} crypto_cmd_t;

typedef struct
{
    uint8_t     state[ SHA256_HASH_SIZE ];               /* HMAC Key/ Result of previous HASH (Used in HASH_UPDT/HASH_FINISH) */
    uint8_t*    payload_buffer;                          /* Buffer to store Output payload
                                                            HWCrypto engine outputs the input payload + HASH result */
    uint8_t     hash_optype;                             /* HASH_INIT/HASH_UPDT/HASH_FINISH, Used for incremental hash operations */
    uint32_t    prev_len;                                /* Used for HASH_UPDT/HASH_FISH, length of data hashed till now */
    int32_t     is224;                                   /* 0 : SHA256, 1: SHA256 */
    uint8_t     i_key_pad[ HMAC_INNERHASHCONTEXT_SIZE ]; /* Used for HMAC > HWCRYPTO_MAX_PAYLOAD_SIZE */
    uint8_t     o_key_pad[ HMAC_INNERHASHCONTEXT_SIZE ]; /* Used for HMAC > HWCRYPTO_MAX_PAYLOAD_SIZE */
} sha256_context_t;

typedef struct
{
    uint8_t*      key;          /* AES Key */
    uint32_t      keylen;       /* AES Key length */
    uint32_t      direction;    /* OUTBOUND : HW_AES_ENCRYPT, INBOUND : HW_AES_DECRYPT */
    uint32_t      cipher_mode;  /* CBC/ECB/CTR/OFB */
} hw_aes_context_t;

typedef struct
{
    uint8_t*    key;            /* DES Key */
    uint32_t    keylen;         /* DES Key length */
    uint32_t    direction;      /* OUTBOUND : HW_DES_ENCRYPT, INBOUND : HW_DES_DECRYPT */
    uint32_t    cipher_mode;    /* CBC/ECB */
    uint32_t    crypt_algo;
} hw_des_context_t;
/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

platform_result_t platform_hwcrypto_execute(crypto_cmd_t cmd);
platform_result_t platform_hwcrypto_aes128cbc_decrypt( uint8_t* key, uint32_t keysize, uint8_t* iv, uint32_t size,
        uint8_t* src, uint8_t* dest );
platform_result_t platform_hwcrypto_sha256_hash(uint8_t* source, uint32_t size, uint8_t* output_payload_buffer, uint8_t* hash_output);
platform_result_t platform_hwcrypto_sha256_incremental(sha256_context_t* ctx, uint8_t* source, uint32_t length);
void platform_hwcrypto_init( void );
platform_result_t platform_hwcrypto_sha2_starts( sha256_context_t* ctx, const unsigned char* input, uint32_t ilen, int32_t is224 );
platform_result_t platform_hwcrypto_sha2_update( sha256_context_t* ctx, const unsigned char* input, uint32_t ilen );
platform_result_t platform_hwcrypto_sha2_finish( sha256_context_t* ctx, const unsigned char* input, uint32_t ilen, unsigned char output[ SHA256_HASH_SIZE ] );
platform_result_t platform_hwcrypto_sha2_incremental( const unsigned char* input, uint32_t ilen, unsigned char output[ SHA256_HASH_SIZE ], int32_t is224);
platform_result_t platform_hwcrypto_sha2(const unsigned char* input, uint32_t ilen, unsigned char hash_output[ SHA256_HASH_SIZE ], int32_t is224);
platform_result_t platform_hwcrypto_sha2_hmac( const unsigned char* key, uint32_t keylen, const unsigned char* input, uint32_t ilen,
        unsigned char output[ SHA256_HASH_SIZE ], int32_t is224);
platform_result_t platform_hwcrypto_sha2_hmac_incremental( const unsigned char* key, uint32_t keylen, const unsigned char* input, uint32_t ilen,
        unsigned char output[ SHA256_HASH_SIZE ], int32_t is224);
platform_result_t platform_hwcrypto_sha2_hmac_starts( sha256_context_t* ctx, const unsigned char* key, uint32_t keylen, int32_t is224 );
platform_result_t platform_hwcrypto_sha2_hmac_update( sha256_context_t* ctx, const unsigned char* input, int32_t ilen );
platform_result_t platform_hwcrypto_sha2_hmac_finish( sha256_context_t* ctx, uint8_t* input, int32_t ilen, unsigned char output[ SHA256_HASH_SIZE ] );
platform_result_t platform_hwcrypto_aescbc_decrypt_sha256_hmac(uint8_t* crypt_key, uint8_t* crypt_iv, uint32_t crypt_size,
        uint32_t auth_size, uint8_t* hmac_key, uint32_t hmac_key_len, uint8_t* src, uint8_t* crypt_dest, uint8_t* hash_dest);
platform_result_t platform_hwcrypto_sha256_hmac( uint8_t* hmac_key, uint32_t keysize, uint8_t* source, uint32_t size,
        uint8_t* output_payload_buffer, uint8_t* hash_output );
platform_result_t platform_hwcrypto_sha256_hmac_aescbc_encrypt(uint8_t* crypt_key, uint8_t* crypt_iv, uint32_t crypt_size,
        uint32_t auth_size, uint8_t* hmac_key, uint32_t hmac_key_len, uint8_t* src, uint8_t* crypt_dest, uint8_t* hash_dest);
platform_result_t platform_hwcrypto_aes128cbc_encrypt( uint8_t* key, uint32_t keysize, uint8_t* iv, uint32_t size, uint8_t* src, uint8_t* dest );
void hw_aes_setkey_enc( hw_aes_context_t* ctx, unsigned char* key, uint32_t keysize_bits );
void hw_aes_setkey_dec( hw_aes_context_t* ctx, unsigned char* key, uint32_t keysize_bits );
void hw_aes_crypt_cbc(hw_aes_context_t* ctx, hw_aes_mode_type_t mode, uint32_t length, unsigned char iv[16], const unsigned char* input, unsigned char* output);
void hw_aes_crypt_ecb(hw_aes_context_t* ctx, hw_aes_mode_type_t mode, const unsigned char input[16], unsigned char output[16]);
void hw_aes_crypt_cfb(hw_aes_context_t* ctx, hw_aes_mode_type_t mode, uint32_t length, uint32_t* iv_off, unsigned char iv[16], const unsigned char* input, unsigned char* output);
void hw_aes_crypt_ctr(hw_aes_context_t* ctx, hw_aes_mode_type_t mode, uint32_t length, unsigned char iv[16], const unsigned char* input, unsigned char* output);
void hw_des_setkey( hw_des_context_t* ctx, unsigned char* key );
void hw_des3_setkey(hw_des_context_t* ctx, unsigned char* key);
void hw_des3_set2key(hw_des_context_t* ctx, unsigned char* key);
void hw_des3_set3key(hw_des_context_t* ctx, unsigned char* key);
void hw_des_crypt_ecb( hw_des_context_t* ctx, hw_des_mode_type_t mode, const unsigned char input[8], unsigned char output[8] );
void hw_des_crypt_cbc( hw_des_context_t* ctx, hw_des_mode_type_t mode, uint32_t length, unsigned char iv[8], const unsigned char* input, unsigned char* output );
void hw_des3_crypt_ecb( hw_des_context_t* ctx, hw_des_mode_type_t mode, const unsigned char input[8], unsigned char output[8] );
void hw_des3_crypt_cbc( hw_des_context_t* ctx, hw_des_mode_type_t mode, uint32_t length, unsigned char iv[8], const unsigned char* input, unsigned char* output );

#endif  /*_CRYPTO_API_H */

