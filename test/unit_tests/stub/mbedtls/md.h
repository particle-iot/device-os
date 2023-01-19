#pragma once

#include <stddef.h>

typedef enum mbedtls_md_type_t {
    MBEDTLS_MD_SHA256 = 6
} mbedtls_md_type_t;

typedef struct mbedtls_md_info_t {
} mbedtls_md_info_t;

typedef struct mbedtls_md_context_t {
} mbedtls_md_context_t;

#ifdef __cplusplus
extern "C" {
#endif

const mbedtls_md_info_t* mbedtls_md_info_from_type(mbedtls_md_type_t md_type);
void mbedtls_md_init(mbedtls_md_context_t* ctx);
int mbedtls_md_setup(mbedtls_md_context_t* ctx, const mbedtls_md_info_t* md_info, int hmac);
void mbedtls_md_free(mbedtls_md_context_t* ctx);
int mbedtls_md_starts(mbedtls_md_context_t* ctx);
int mbedtls_md_finish(mbedtls_md_context_t* ctx, unsigned char* output);
int mbedtls_md_update(mbedtls_md_context_t* ctx, const unsigned char* input, size_t ilen);
int mbedtls_md_clone(mbedtls_md_context_t* dst, const mbedtls_md_context_t* src);
int mbedtls_md_hmac_starts(mbedtls_md_context_t* ctx, const unsigned char* key, size_t keylen);
int mbedtls_md_hmac_finish(mbedtls_md_context_t* ctx, unsigned char* output);
int mbedtls_md_hmac_update(mbedtls_md_context_t* ctx, const unsigned char* input, size_t ilen);
int mbedtls_md_hmac_reset(mbedtls_md_context_t* ctx);

#ifdef __cplusplus
} // extern "C"
#endif
