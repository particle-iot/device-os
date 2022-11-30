#include "pk.h"
#include "mbedtls_mock.h"

using namespace particle::test;

void mbedtls_pk_init(mbedtls_pk_context* ctx) {
}

void mbedtls_pk_free(mbedtls_pk_context* ctx) {
}

int mbedtls_pk_parse_public_key(mbedtls_pk_context* ctx, const unsigned char* key, size_t keylen) {
    auto mock = MbedtlsMock::instance();
    if (mock) {
        return mock->get().pkParsePublicKey(ctx, key, keylen);
    }
    return 0;
}

int mbedtls_pk_verify(mbedtls_pk_context* ctx, mbedtls_md_type_t md_alg, const unsigned char* hash, size_t hash_len,
        const unsigned char* sig, size_t sig_len) {
    auto mock = MbedtlsMock::instance();
    if (mock) {
        return mock->get().pkVerify(ctx, md_alg, hash, hash_len, sig, sig_len);
    }
    return 0;
}
