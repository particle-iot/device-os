#include "static_assert.h"

#include "wiced.h"
#include "wiced_security.h"
#include "crypto_open/x509.h"
#include "crypto_open/bignum.h"
#include "micro-ecc/configuration.h"
#include "micro-ecc/uECC.h"

#include "mbedtls/aes.h"
#include "mbedtls/rsa.h"
#include "mbedtls/bignum.h"
#include "mbedtls/sha1.h"
#include "mbedtls/md.h"
#include "mbedtls/md5.h"
#include "mbedtls/md4.h"
#include "mbedtls/sha256.h"
#include "mbedtls/sha512.h"
#include "mbedtls/des.h"
#include "mbedtls/x509.h"
#include "mbedtls/x509_crt.h"
#include "debug.h"
#include "mbedtls_util.h"
#include "timer_hal.h"

LOG_SOURCE_CATEGORY("crypto.compat")

struct MbedTlsCallbackInitializer {
    MbedTlsCallbackInitializer() {
        mbedtls_callbacks_t cb = {0};
        cb.version = 0;
        cb.size = sizeof(cb);
        cb.mbedtls_md_list = mbedtls_md_list;
        cb.mbedtls_md_info_from_string = mbedtls_md_info_from_string;
        cb.mbedtls_md_info_from_type = mbedtls_md_info_from_type;
        cb.millis = HAL_Timer_Get_Milli_Seconds;
        mbedtls_set_callbacks(&cb, NULL);
    }
};

MbedTlsCallbackInitializer s_mbedtls_callback_initializer;

STATIC_ASSERT(wiced_aes_context_size, sizeof(aes_context_t) >= sizeof(mbedtls_aes_context));
STATIC_ASSERT(wiced_sha1_context_size, sizeof(sha1_context) >= sizeof(mbedtls_sha1_context));
STATIC_ASSERT(wiced_sha1_context_size_md, sizeof(sha1_context) >= sizeof(mbedtls_md_context_t));
STATIC_ASSERT(wiced_sha2_context_size, sizeof(sha2_context) >= sizeof(mbedtls_sha256_context));
STATIC_ASSERT(wiced_sha2_context_size_md, sizeof(sha2_context) >= sizeof(mbedtls_md_context_t));
STATIC_ASSERT(wiced_sha4_context_size, sizeof(sha4_context) >= sizeof(mbedtls_sha512_context));
STATIC_ASSERT(wiced_sha4_context_size_md, sizeof(sha4_context) >= sizeof(mbedtls_md_context_t));
STATIC_ASSERT(wiced_md5_context_size, sizeof(md5_context) >= sizeof(mbedtls_md5_context));
STATIC_ASSERT(wiced_md5_context_size_md, sizeof(md5_context) >= sizeof(mbedtls_md_context_t));
STATIC_ASSERT(wiced_md4_context_size, sizeof(md4_context) >= sizeof(mbedtls_md4_context));
STATIC_ASSERT(wiced_mpi_size, sizeof(mpi) >= sizeof(mbedtls_mpi));
STATIC_ASSERT(wiced_des_context_size, sizeof(des_context) >= sizeof(mbedtls_des_context));
STATIC_ASSERT(wiced_des3_context_size, sizeof(des3_context) >= sizeof(mbedtls_des3_context));
STATIC_ASSERT(wiced_mpi_size, sizeof(mpi) == sizeof(mbedtls_mpi));
// NOTE: X509 is incompatible
// STATIC_ASSERT(wiced_x509_cert_size, sizeof(x509_cert) >= sizeof(mbedtls_x509_crt));
STATIC_ASSERT(wiced_x509_buf_size, sizeof(x509_buf) == sizeof(mbedtls_asn1_buf));
STATIC_ASSERT(wiced_x509_name_offset_1, offsetof(x509_name, oid) == offsetof(mbedtls_asn1_named_data, oid));
STATIC_ASSERT(wiced_x509_name_offset_2, offsetof(x509_name, val) == offsetof(mbedtls_asn1_named_data, val));
STATIC_ASSERT(wiced_x509_name_offset_3, offsetof(x509_name, next) == offsetof(mbedtls_asn1_named_data, next));
// NOTE: RSA is incompatible
// STATIC_ASSERT(wiced_rsa_context_size, sizeof(rsa_context) >= sizeof(mbedtls_rsa_context));

wiced_tls_key_type_t type = TLS_RSA_KEY;

void* tls_host_malloc( const char* name, uint32_t size)
{
    void* ptr = malloc(size);
    return ptr;
}

void* tls_host_calloc( const char* name, size_t nelem, size_t elsize )
{
    void* ptr = calloc(nelem, elsize);
    return ptr;
}

void tls_host_free(void* p)
{
    free(p);
}

/* SHA-384/SHA-512 */
void sha4_starts(sha4_context* context, int32_t is384) {
    mbedtls_sha512_starts((mbedtls_sha512_context*)context, is384);
}

void sha4_update(sha4_context* context, const unsigned char* input_data, int32_t input_len) {
    mbedtls_sha512_update((mbedtls_sha512_context*)context, input_data, input_len);
}

void sha4_finish(sha4_context* context, unsigned char hash_output[64]) {
    mbedtls_sha512_finish((mbedtls_sha512_context*)context, hash_output);
}

void sha4(const unsigned char *input, uint32_t ilen, unsigned char output[64], int32_t is384)
{
    mbedtls_sha512(input, ilen, output, is384);
}

int uECC_sign(const uint8_t private_key[uECC_BYTES], const uint8_t message_hash[uECC_BYTES], uint8_t signature[uECC_BYTES*2]) {
    return 0;
}

int uECC_verify(const uint8_t public_key[uECC_BYTES*2], const uint8_t hash[uECC_BYTES], const uint8_t signature[uECC_BYTES*2]) {
    return 0;
}

int32_t x509parse_key_ecc(wiced_tls_ecc_key_t * ecc, const unsigned char *key, uint32_t keylen, const unsigned char *pwd, uint32_t pwdlen) {
    return 1;
}

// DES
void des_setkey_enc( des_context *ctx, const unsigned char key[8] )
{
    mbedtls_des_setkey_enc((mbedtls_des_context*)ctx, key);
}

void des_crypt_ecb( des_context *ctx, const unsigned char input[8], unsigned char output[8] )
{
    mbedtls_des_crypt_ecb((mbedtls_des_context*)ctx, input, output);
}

void des3_set3key_enc( des3_context *ctx, const unsigned char key[24] )
{
    mbedtls_des3_set3key_enc((mbedtls_des3_context*)ctx, key);
}

void des3_set3key_dec( des3_context *ctx, const unsigned char key[24] )
{
    mbedtls_des3_set3key_dec((mbedtls_des3_context*)ctx, key);
}

void des3_crypt_cbc( des3_context *ctx, des_mode_t mode, int32_t length, unsigned char iv[8], const unsigned char *input, unsigned char *output )
{
    mbedtls_des3_crypt_cbc((mbedtls_des3_context*)ctx, mode, length, iv, input, output);
}

// AES

void aes_setkey_enc( aes_context_t *ctx, const unsigned char *key, uint32_t keysize_bits )
{
    mbedtls_aes_setkey_enc((mbedtls_aes_context*)ctx, key, keysize_bits);
}

void aes_setkey_dec( aes_context_t *ctx, const unsigned char *key, uint32_t keysize_bits )
{
    mbedtls_aes_setkey_dec((mbedtls_aes_context*)ctx, key, keysize_bits);
}

void aes_crypt_cbc( aes_context_t *ctx, aes_mode_type_t mode, uint32_t length, unsigned char iv[16], const unsigned char *input, unsigned char *output )
{
    mbedtls_aes_crypt_cbc((mbedtls_aes_context*)ctx, mode, length, iv, input, output);
}

void aes_crypt_ecb( aes_context_t *ctx, aes_mode_type_t mode, const unsigned char input[16], unsigned char output[16] )
{
    mbedtls_aes_crypt_ecb((mbedtls_aes_context*)ctx, mode, input, output);
}

// SHA1
void sha1_starts( sha1_context *ctx )
{
    mbedtls_sha1_starts((mbedtls_sha1_context*)ctx);
}

void sha1_update( sha1_context *ctx, const unsigned char *input, int32_t ilen )
{
    mbedtls_sha1_update((mbedtls_sha1_context*)ctx, input, ilen);
}

void sha1_finish( sha1_context *ctx, unsigned char output[20] )
{
    mbedtls_sha1_finish((mbedtls_sha1_context*)ctx, output);
}

void sha1( const unsigned char *input, int32_t ilen, unsigned char output[20] )
{
    mbedtls_sha1(input, ilen, output);
}

void sha1_hmac_starts( sha1_context *ctx, const unsigned char *key, uint32_t keylen )
{
    mbedtls_md_context_t* mdctx = (mbedtls_md_context_t*)ctx;
    mbedtls_md_init(mdctx);
    mbedtls_md_starts(mdctx);
    mbedtls_md_setup(mdctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA1), 1);
    mbedtls_md_hmac_starts(mdctx, key, keylen);
}

void sha1_hmac_update( sha1_context *ctx, const unsigned char *input, uint32_t ilen )
{
    mbedtls_md_context_t* mdctx = (mbedtls_md_context_t*)ctx;
    mbedtls_md_hmac_update(mdctx, input, ilen);
}

void sha1_hmac_finish( sha1_context *ctx, unsigned char output[20] )
{
    mbedtls_md_context_t* mdctx = (mbedtls_md_context_t*)ctx;
    mbedtls_md_hmac_finish(mdctx, output);
    mbedtls_md_free(mdctx);
}

void sha1_hmac( const unsigned char *key, int32_t keylen, const unsigned char *input, int32_t ilen, unsigned char output[20] )
{
    mbedtls_md_hmac(mbedtls_md_info_from_type(MBEDTLS_MD_SHA1), key, keylen, input, ilen, output);
}

// MD5
void md5_starts( md5_context *ctx )
{
    mbedtls_md5_starts((mbedtls_md5_context*)ctx);
}

void md5_update( md5_context *ctx, const unsigned char *input, int32_t ilen )
{
    mbedtls_md5_update((mbedtls_md5_context*)ctx, input, ilen);
}

void md5_finish( md5_context *ctx, unsigned char output[16] )
{
    mbedtls_md5_finish((mbedtls_md5_context*)ctx, output);
}

void md5( const unsigned char *input, int32_t ilen, unsigned char output[16] )
{
    mbedtls_md5(input, ilen, output);
}

void md5_hmac_starts( md5_context *ctx, const unsigned char *key, uint32_t keylen )
{
    mbedtls_md_context_t* mdctx = (mbedtls_md_context_t*)ctx;
    mbedtls_md_init(mdctx);
    mbedtls_md_starts(mdctx);
    mbedtls_md_setup(mdctx, mbedtls_md_info_from_type(MBEDTLS_MD_MD5), 1);
    mbedtls_md_hmac_starts(mdctx, key, keylen);
}

void md5_hmac_update( md5_context *ctx, const unsigned char *input, uint32_t ilen )
{
    mbedtls_md_context_t* mdctx = (mbedtls_md_context_t*)ctx;
    mbedtls_md_hmac_update(mdctx, input, ilen);
}

void md5_hmac_finish( md5_context *ctx, unsigned char output[16] )
{
    mbedtls_md_context_t* mdctx = (mbedtls_md_context_t*)ctx;
    mbedtls_md_hmac_finish(mdctx, output);
    mbedtls_md_free(mdctx);
}

void md5_hmac( const unsigned char *key, int32_t keylen, const unsigned char *input, int32_t ilen, unsigned char output[16] )
{
    mbedtls_md_hmac(mbedtls_md_info_from_type(MBEDTLS_MD_MD5), key, keylen, input, ilen, output);
}

// MD4
void md4_starts( md4_context *ctx )
{
    mbedtls_md4_starts((mbedtls_md4_context*)ctx);
}

void md4_update( md4_context *ctx, const unsigned char *input, int32_t ilen )
{
    mbedtls_md4_update((mbedtls_md4_context*)ctx, input, ilen);
}

void md4_finish( md4_context *ctx, unsigned char output[16] )
{
    mbedtls_md4_finish((mbedtls_md4_context*)ctx, output);
}

// SHA256
void sha2_starts( sha2_context *ctx, int32_t is224 )
{
    mbedtls_sha256_starts((mbedtls_sha256_context*)ctx, is224);
}

void sha2_update( sha2_context *ctx, const unsigned char *input, uint32_t ilen )
{
    mbedtls_sha256_update((mbedtls_sha256_context*)ctx, input, ilen);
}

void sha2_finish( sha2_context *ctx, unsigned char output[32] )
{
    mbedtls_sha256_finish((mbedtls_sha256_context*)ctx, output);
}

void sha2( const unsigned char *input, uint32_t ilen, unsigned char output[32], int32_t is224 )
{
    mbedtls_sha256(input, ilen, output, is224);
}

void sha2_hmac_starts( sha2_context *ctx, const unsigned char *key, uint32_t keylen, int32_t is224 )
{
    mbedtls_md_context_t* mdctx = (mbedtls_md_context_t*)ctx;
    mbedtls_md_init(mdctx);
    mbedtls_md_starts(mdctx);
    mbedtls_md_setup(mdctx, is224 ? mbedtls_md_info_from_type(MBEDTLS_MD_SHA224) : mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
    mbedtls_md_hmac_starts(mdctx, key, keylen);
}

void sha2_hmac_update( sha2_context *ctx, const unsigned char *input, uint32_t ilen )
{
    mbedtls_md_context_t* mdctx = (mbedtls_md_context_t*)ctx;
    mbedtls_md_hmac_update(mdctx, input, ilen);
}

void sha2_hmac_finish( sha2_context *ctx, unsigned char output[32] )
{
    mbedtls_md_context_t* mdctx = (mbedtls_md_context_t*)ctx;
    mbedtls_md_hmac_finish(mdctx, output);
    mbedtls_md_free(mdctx);
}

void sha2_hmac( const unsigned char *key, uint32_t keylen, const unsigned char *input, uint32_t ilen, unsigned char output[32], int32_t is224 )
{
    mbedtls_md_hmac(is224 ? mbedtls_md_info_from_type(MBEDTLS_MD_SHA224) : mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), key, keylen, input, ilen, output);
}

// Bignum
void mpi_init(mpi* X)
{
    mbedtls_mpi_init((mbedtls_mpi*)X);
}

int32_t mpi_read_binary(mpi *X, const unsigned char *buf, int32_t buflen)
{
    return mbedtls_mpi_read_binary((mbedtls_mpi*)X, buf, buflen);
}

int32_t mpi_write_binary(const mpi *X, unsigned char *buf, int32_t buflen)
{
    return mbedtls_mpi_write_binary((const mbedtls_mpi*)X, buf, buflen);
}

int32_t mpi_cmp_mpi(const mpi *X, const mpi *Y)
{
    return mbedtls_mpi_cmp_mpi((const mbedtls_mpi*)X, (const mbedtls_mpi*)Y);
}

void mpi_free(mpi* X)
{
    return mbedtls_mpi_free((mbedtls_mpi*)X);
}

int32_t mpi_exp_mod(mpi *X, const mpi *A, const mpi *E, const mpi *N, mpi *_RR)
{
    return mbedtls_mpi_exp_mod((mbedtls_mpi*)X, (const mbedtls_mpi*)A, (const mbedtls_mpi*)E, (const mbedtls_mpi*)N, (mbedtls_mpi*)_RR );
}

int32_t mpi_sub_mpi(mpi *X, const mpi *A, const mpi *B)
{
    return mbedtls_mpi_sub_mpi((mbedtls_mpi*)X, (const mbedtls_mpi*)A, (const mbedtls_mpi*)B);
}

int32_t mpi_mul_mpi(mpi *X, const mpi *A, const mpi *B)
{
    return mbedtls_mpi_mul_mpi((mbedtls_mpi*)X, (const mbedtls_mpi*)A, (const mbedtls_mpi*)B);
}

int32_t mpi_mod_mpi(mpi *X, const mpi *A, const mpi *B)
{
    return mbedtls_mpi_mod_mpi((mbedtls_mpi*)X, (const mbedtls_mpi*)A, (const mbedtls_mpi*)B);
}

int32_t mpi_add_mpi(mpi *X, const mpi *A, const mpi *B)
{
    return mbedtls_mpi_add_mpi((mbedtls_mpi*)X, (const mbedtls_mpi*)A, (const mbedtls_mpi*)B);
}

uint32_t mpi_msb(const mpi *X)
{
    return mbedtls_mpi_bitlen((const mbedtls_mpi*)X);
}

int32_t mpi_sub_int(mpi *X, const mpi *A, int32_t b)
{
    return mbedtls_mpi_sub_int((mbedtls_mpi*)X, (const mbedtls_mpi*)A, b);
}

int32_t mpi_gcd(mpi *G, const mpi *A, const mpi *B)
{
    return mbedtls_mpi_gcd((mbedtls_mpi*)G, (const mbedtls_mpi*)A, (const mbedtls_mpi*)B);
}

int32_t mpi_cmp_int(const mpi *X, int32_t z)
{
    return mbedtls_mpi_cmp_int((const mbedtls_mpi*)X, z);
}

uint32_t mpi_size(const mpi *X)
{
    return mbedtls_mpi_size((const mbedtls_mpi*)X);
}

int32_t mpi_grow(mpi *X, int32_t nblimbs)
{
    return mbedtls_mpi_grow((mbedtls_mpi*)X, nblimbs);
}

int32_t mpi_lset(mpi *X, int32_t z)
{
    return mbedtls_mpi_lset((mbedtls_mpi*)X, z);
}

int32_t mpi_shift_r(mpi *X, int32_t count)
{
    return mbedtls_mpi_shift_r((mbedtls_mpi*)X, count);
}

static mbedtls_md_type_t to_mbedtls_md_type(int32_t hash_id)
{
    mbedtls_md_type_t hashid = MBEDTLS_MD_NONE;
    switch (hash_id) {
        case RSA_RAW: hashid = MBEDTLS_MD_NONE; break;
        case RSA_MD2: hashid = MBEDTLS_MD_MD2; break;
        case RSA_MD4: hashid = MBEDTLS_MD_MD4; break;
        case RSA_MD5: hashid = MBEDTLS_MD_MD5; break;
        case RSA_SHA1: hashid = MBEDTLS_MD_SHA1; break;
        case RSA_SHA224: hashid = MBEDTLS_MD_SHA224; break;
        case RSA_SHA256: hashid = MBEDTLS_MD_SHA256; break;
        case RSA_SHA384: hashid = MBEDTLS_MD_SHA384; break;
        case RSA_SHA512: hashid = MBEDTLS_MD_SHA512; break;
    }
    return hashid;
}

static int32_t from_mbedtls_md_type(mbedtls_md_type_t hash_id)
{
    int32_t hashid = RSA_RAW;
    switch (hash_id) {
        case MBEDTLS_MD_NONE: hashid = RSA_RAW; break;
        case MBEDTLS_MD_MD2: hashid = RSA_MD2; break;
        case MBEDTLS_MD_MD4: hashid = RSA_MD4; break;
        case MBEDTLS_MD_MD5: hashid = RSA_MD5; break;
        case MBEDTLS_MD_SHA1: hashid = RSA_SHA1; break;
        case MBEDTLS_MD_SHA224: hashid = RSA_SHA224; break;
        case MBEDTLS_MD_SHA256: hashid = RSA_SHA256; break;
        case MBEDTLS_MD_SHA384: hashid = RSA_SHA384; break;
        case MBEDTLS_MD_SHA512: hashid = RSA_SHA512; break;
    }
    return hashid;
}

static mbedtls_rsa_context* rsa_wiced_to_mbedtls(const rsa_context* w, mbedtls_rsa_context* m)
{
    memset(m, 0, sizeof(mbedtls_rsa_context));
    m->ver = w->version;
    m->len = w->length;

    memcpy(&m->N, &w->N, sizeof(mbedtls_mpi));
    memcpy(&m->E, &w->E, sizeof(mbedtls_mpi));

    memcpy(&m->D, &w->D, sizeof(mbedtls_mpi));
    memcpy(&m->P, &w->P, sizeof(mbedtls_mpi));
    memcpy(&m->Q, &w->Q, sizeof(mbedtls_mpi));
    memcpy(&m->DP, &w->DP, sizeof(mbedtls_mpi));
    memcpy(&m->DQ, &w->DQ, sizeof(mbedtls_mpi));
    memcpy(&m->QP, &w->QP, sizeof(mbedtls_mpi));

    memcpy(&m->RN, &w->RN, sizeof(mbedtls_mpi));
    memcpy(&m->RP, &w->RP, sizeof(mbedtls_mpi));
    memcpy(&m->RQ, &w->RQ, sizeof(mbedtls_mpi));

    m->padding = w->padding;
    m->hash_id = to_mbedtls_md_type(w->hash_id);
    return m;
}

static rsa_context* rsa_mbedtls_to_wiced(mbedtls_rsa_context* m, rsa_context* w)
{
    w->version = m->ver;
    w->length = m->len;

    memcpy(&w->N, &m->N, sizeof(mbedtls_mpi));
    memcpy(&w->E, &m->E, sizeof(mbedtls_mpi));

    memcpy(&w->D, &m->D, sizeof(mbedtls_mpi));
    memcpy(&w->P, &m->P, sizeof(mbedtls_mpi));
    memcpy(&w->Q, &m->Q, sizeof(mbedtls_mpi));
    memcpy(&w->DP, &m->DP, sizeof(mbedtls_mpi));
    memcpy(&w->DQ, &m->DQ, sizeof(mbedtls_mpi));
    memcpy(&w->QP, &m->QP, sizeof(mbedtls_mpi));

    memcpy(&w->RN, &m->RN, sizeof(mbedtls_mpi));
    memcpy(&w->RP, &m->RP, sizeof(mbedtls_mpi));
    memcpy(&w->RQ, &m->RQ, sizeof(mbedtls_mpi));

    w->padding = (rsa_pkcs_padding_t)m->padding;
    w->hash_id = from_mbedtls_md_type((mbedtls_md_type_t)m->hash_id);

    // WICED rsa implementation has no Vf or Vi, we need to clean them here to avoid a memory leak
    mbedtls_mpi_free(&m->Vf);
    mbedtls_mpi_free(&m->Vi);
    return w;
}

static int32_t rsa_wiced_rng(void* ptr)
{
    int32_t val;
    mbedtls_default_rng(NULL, (unsigned char*)&val, 4);
    return val;
}

// RSA
void rsa_init( rsa_context *ctx, int32_t padding, int32_t hash_id, int32_t (*f_rng)( void * ), void *p_rng )
{
    ctx->type = TLS_RSA_KEY;

    mbedtls_rsa_context tmp;
    mbedtls_rsa_init(&tmp, padding, to_mbedtls_md_type(hash_id));
    rsa_mbedtls_to_wiced(&tmp, ctx);
    ctx->f_rng = rsa_wiced_rng;
    ctx->p_rng = NULL;
}

int32_t rsa_gen_key( rsa_context *ctx, int32_t nbits, int32_t exponent )
{
    mbedtls_rsa_context tmp;
    rsa_wiced_to_mbedtls(ctx, &tmp);
    int32_t ret = mbedtls_rsa_gen_key(&tmp, mbedtls_default_rng, NULL, nbits, exponent);
    rsa_mbedtls_to_wiced(&tmp, ctx);
    return ret;
}

int32_t rsa_check_pubkey( const rsa_context *ctx )
{
    mbedtls_rsa_context tmp;
    rsa_wiced_to_mbedtls(ctx, &tmp);
    int32_t ret = mbedtls_rsa_check_pubkey(&tmp);
    rsa_mbedtls_to_wiced(&tmp, (rsa_context*)ctx);
    return ret;
}

int32_t rsa_check_privkey( const rsa_context *ctx )
{
    mbedtls_rsa_context tmp;
    rsa_wiced_to_mbedtls(ctx, &tmp);
    int32_t ret = mbedtls_rsa_check_privkey(&tmp);
    rsa_mbedtls_to_wiced(&tmp, (rsa_context*)ctx);
    return ret;
}

int32_t rsa_public( const rsa_context *ctx, const unsigned char *input, unsigned char *output )
{
    mbedtls_rsa_context tmp;
    rsa_wiced_to_mbedtls(ctx, &tmp);
    int32_t ret = mbedtls_rsa_public(&tmp, input, output);
    rsa_mbedtls_to_wiced(&tmp, (rsa_context*)ctx);
    return ret;
}

int32_t rsa_private( const rsa_context *ctx, const unsigned char *input, unsigned char *output )
{
    mbedtls_rsa_context tmp;
    rsa_wiced_to_mbedtls(ctx, &tmp);
    int32_t ret = mbedtls_rsa_private(&tmp, mbedtls_default_rng, NULL, input, output);
    rsa_mbedtls_to_wiced(&tmp, (rsa_context*)ctx);
    return ret;
}

int32_t rsa_pkcs1_encrypt( const rsa_context *ctx, int32_t mode, int32_t ilen, const unsigned char *input, unsigned char *output )
{
    mbedtls_rsa_context tmp;
    rsa_wiced_to_mbedtls(ctx, &tmp);
    int32_t ret = mbedtls_rsa_pkcs1_encrypt(&tmp, mbedtls_default_rng, NULL, mode, ilen, input, output);
    rsa_mbedtls_to_wiced(&tmp, (rsa_context*)ctx);
    return ret;
}

int32_t rsa_pkcs1_decrypt( const rsa_context *ctx, int32_t mode, int32_t *olen, const unsigned char *input, unsigned char *output, int32_t output_max_len )
{
    mbedtls_rsa_context tmp;
    rsa_wiced_to_mbedtls(ctx, &tmp);
    int32_t ret = mbedtls_rsa_pkcs1_decrypt(&tmp, mbedtls_default_rng, NULL, mode, (size_t*)olen, input, output, output_max_len);
    rsa_mbedtls_to_wiced(&tmp, (rsa_context*)ctx);
    return ret;
}

int32_t rsa_pkcs1_sign( const rsa_context *ctx, int32_t mode, int32_t hash_id, int32_t hashlen, const unsigned char *hash, unsigned char *sig )
{
    mbedtls_md_type_t hashid = to_mbedtls_md_type(hash_id);
    mbedtls_rsa_context tmp;
    rsa_wiced_to_mbedtls(ctx, &tmp);
    int32_t ret = mbedtls_rsa_pkcs1_sign(&tmp, mbedtls_default_rng, NULL, mode, hashid, hashlen, hash, sig);
    rsa_mbedtls_to_wiced(&tmp, (rsa_context*)ctx);
    return ret;
}

int32_t rsa_pkcs1_verify( const rsa_context *ctx, rsa_mode_t mode, rsa_hash_id_t hash_id, int32_t hashlen, const unsigned char *hash, const unsigned char *sig )
{
    mbedtls_md_type_t hashid = to_mbedtls_md_type(hash_id);
    mbedtls_rsa_context tmp;
    rsa_wiced_to_mbedtls(ctx, &tmp);
    int32_t ret = mbedtls_rsa_pkcs1_verify(&tmp, mbedtls_default_rng, NULL, mode, hashid, hashlen, hash, sig);
    rsa_mbedtls_to_wiced(&tmp, (rsa_context*)ctx);
    return ret;
}

void rsa_free( rsa_context *ctx )
{
    mbedtls_rsa_context tmp;
    rsa_wiced_to_mbedtls(ctx, &tmp);
    mbedtls_rsa_free(&tmp);
    memset(ctx, 0, sizeof(rsa_context));
}

typedef struct {
    uint32_t header;
    mbedtls_x509_crt* crt;
    uint8_t* orig_ptr;
    uint32_t orig_len;
} x509_compat_t;

static mbedtls_x509_crt* x509_to_mbedtls(x509_cert* wcrt, bool noalloc = false)
{
    if (wcrt == NULL) {
        return NULL;
    }

    x509_compat_t* compat = (x509_compat_t*)wcrt;
    if (compat->header == 0xdeadbeef && compat->crt) {
        return compat->crt;
    }

    if (!noalloc) {
        memset(compat, 0, sizeof(*compat));
        compat->crt = (mbedtls_x509_crt*)tls_host_calloc(__FUNCTION__, 1, sizeof(mbedtls_x509_crt));
        if (compat->crt)  {
            compat->header = 0xdeadbeef;
            mbedtls_x509_crt_init(compat->crt);
            return compat->crt;
        }
    }

    return NULL;
}

static int mbedtls_to_x509(mbedtls_x509_crt* c, x509_cert* crt)
{
    int res = 0;
    // Fill subject and public_key
    mbedtls_rsa_context* pk = mbedtls_pk_rsa(c->pk);
    if (!crt->public_key && pk) {
        rsa_context* wpk = (rsa_context*)tls_host_calloc(__FUNCTION__, 1, sizeof(rsa_context));
        if (wpk) {
            // Make a copy
            mbedtls_rsa_context pkcopy;
            mbedtls_rsa_init(&pkcopy, 0, 0);
            mbedtls_rsa_copy(&pkcopy, pk);
            rsa_mbedtls_to_wiced(&pkcopy, wpk);
            crt->public_key = (wiced_tls_key_t*)wpk;
        } else {
            res = 1;
        }
    }
    memcpy(&crt->subject, &c->subject, sizeof(mbedtls_x509_name));

    return res;
}

static int mbedtls_to_x509_all(mbedtls_x509_crt* c, x509_cert* crt, int nonalloced)
{
    int res = 0;
    res = mbedtls_to_x509(c, crt);
    if (nonalloced) {
        x509_compat_t* compat = (x509_compat_t*)crt;
        compat->orig_ptr = c->raw.p;
        compat->orig_len = c->raw.len;
    }
    for(mbedtls_x509_crt* cc = c->next; cc != NULL; cc = cc->next) {
        crt->next = (x509_cert*)tls_host_calloc(__FUNCTION__, 1, sizeof(x509_cert));
        if (!crt->next) {
            if (nonalloced) {
                cc->raw.p = NULL;
                cc->raw.len = 0;
            }
            res = 1;
            break;
        }
        x509_compat_t* compat = (x509_compat_t*)crt->next;
        compat->header = 0xdeadbeef;
        compat->crt = cc;
        res = mbedtls_to_x509(cc, crt->next);
        if (nonalloced) {
            compat->orig_ptr = cc->raw.p;
            compat->orig_len = cc->raw.len;
        }
    }

    return res;
}

static int32_t x509_parse_certificate_data_impl(x509_cert* crt, const unsigned char* p, uint32_t len, uint8_t force_alloc)
{
    int32_t ret = -1;
    uint32_t total_len = 0;
    mbedtls_x509_crt* c = x509_to_mbedtls(crt);
    if (c) {
        mbedtls_x509_crt* cc = c;
        mbedtls_x509_crt* prev = NULL;
        do {
            if (cc->version != 0 && cc->next == NULL) {
                cc->next = (mbedtls_x509_crt*)tls_host_calloc(__FUNCTION__, 1, sizeof(mbedtls_x509_crt));
                if (cc->next == NULL) {
                    ret = -1;
                    break;
                }
                prev = cc;
                cc = cc->next;
                mbedtls_x509_crt_init(cc);
            }
            if (!force_alloc) {
                cc->raw.p = (uint8_t*)p + total_len;
                cc->raw.len = len - total_len;
            }
            ret = x509_crt_parse_der_core(cc, p + total_len, len - total_len);
            if (ret == 0) {
                total_len += c->raw.len;
            } else {
                if (prev) {
                    prev->next = NULL;
                }
                if (cc != c) {
                    free(cc);
                    cc = NULL;
                }
            }
        } while(ret == 0 && total_len < len && cc);
    }

    if (total_len > 0) {
        ret = mbedtls_to_x509_all(c, crt, !force_alloc);
    } else {
        ret = 1;
    }

    return ret;
}

int32_t x509_parse_certificate_data(x509_cert* crt, const unsigned char* p, uint32_t len)
{
    int ret = x509_parse_certificate_data_impl(crt, p, len, 0);
    return ret;
}


int32_t x509_cert_is_pem(const uint8_t* buf, size_t len)
{
    return !((len != 0 && buf[len - 1] == '\0' &&
             strstr((const char *)buf, "-----BEGIN CERTIFICATE-----") != NULL));
}

int32_t x509_parse_certificate(x509_cert* chain, const uint8_t* buf, uint32_t buflen)
{
    // PEM or DER
    mbedtls_x509_crt* c = x509_to_mbedtls(chain);
    int32_t ret = -1;
    if (c) {
        if (x509_cert_is_pem(buf, buflen) != 0) {
            // This is probably DER
            ret = x509_parse_certificate_data_impl(chain, buf, buflen, 0);
        } else {
            // PEM
            ret = mbedtls_x509_crt_parse(c, buf, buflen);
            if (ret == 0) {
                // Fill subject and public_key
                ret = mbedtls_to_x509_all(c, chain, 0);
            }
        }
        if (ret) {
            x509_free(chain);
        }
    }
    return ret;
}

int32_t x509_convert_pem_to_der(const unsigned char* pem_certificate, uint32_t pem_certificate_length, const uint8_t** der_certificate, uint32_t* total_der_bytes)
{
    return mbedtls_x509_crt_pem_to_der((const char*)pem_certificate, (size_t)pem_certificate_length, (uint8_t**)der_certificate, (size_t*)total_der_bytes);
}

uint32_t x509_read_cert_length(const uint8_t* der_certificate_data )
{
    return mbedtls_x509_read_length(der_certificate_data, CERTIFICATE_SIZE * 3 / 4, 0);
}

int32_t x509parse_verify(const x509_cert* crt, const x509_cert* trust_ca, const char* cn, int32_t* flags)
{
    mbedtls_x509_crt* c = x509_to_mbedtls((x509_cert*)crt, true);
    mbedtls_x509_crt* tca = x509_to_mbedtls((x509_cert*)trust_ca, true);

    int32_t ret = mbedtls_x509_crt_verify(c, tca, NULL, cn, (uint32_t*)flags, NULL, NULL);
    return ret;
}

int32_t x509parse_key(rsa_context* rsa, const unsigned char* key, uint32_t keylen, const unsigned char* pwd, uint32_t pwdlen)
{
    mbedtls_pk_context pkctx;
    mbedtls_pk_init(&pkctx);
    int32_t ret = mbedtls_pk_parse_key(&pkctx, key, keylen, pwd, pwdlen);
    if (ret == 0) {
        mbedtls_rsa_context* pk = mbedtls_pk_rsa(pkctx);
        if (pk) {
            mbedtls_rsa_context pkcopy;
            mbedtls_rsa_init(&pkcopy, 0, 0);
            mbedtls_rsa_copy(&pkcopy, pk);
            rsa_mbedtls_to_wiced(&pkcopy, rsa);
        }
    }
    mbedtls_pk_free(&pkctx);
    return ret;
}

void x509_free(x509_cert* crtm)
{
    x509_cert* next = NULL;
    for (x509_cert* crt = crtm; crt != NULL; crt = next) {
        next = crt->next;
        mbedtls_x509_crt* c = x509_to_mbedtls(crt, true);
        if (c) {
            if (crt->public_key) {
                rsa_free((rsa_context*)crt->public_key);
                free(crt->public_key);
            }
            x509_compat_t* compat = (x509_compat_t*)crt;
            if (compat->orig_ptr && compat->orig_ptr == c->raw.p) {
                c->raw.p = NULL;
                c->raw.len = 0;
            }
            memset(crt, 0, sizeof(x509_compat_t));
            if (next) {
                c->next = NULL;
            }
            mbedtls_x509_crt_free(c);
            tls_host_free(c);
        }
        if (crt != crtm) {
            free(crt);
        }
    }
}
