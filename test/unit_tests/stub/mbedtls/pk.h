#pragma once

#include <stddef.h>

#include "md.h"

typedef struct mbedtls_pk_context {
} mbedtls_pk_context;

#ifdef __cplusplus
extern "C" {
#endif

void mbedtls_pk_init(mbedtls_pk_context* ctx);
void mbedtls_pk_free(mbedtls_pk_context* ctx);
int mbedtls_pk_parse_public_key(mbedtls_pk_context* ctx, const unsigned char* key, size_t keylen);
int mbedtls_pk_verify(mbedtls_pk_context* ctx, mbedtls_md_type_t md_alg, const unsigned char* hash, size_t hash_len,
        const unsigned char* sig, size_t sig_len);

#ifdef __cplusplus
} // extern "C"
#endif
