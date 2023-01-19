#include "md.h"
#include "mbedtls_mock.h"

using namespace particle::test;

namespace {

const mbedtls_md_info_t DUMMY_MD_INFO;

} // namespace

const mbedtls_md_info_t* mbedtls_md_info_from_type(mbedtls_md_type_t md_type) {
    return &DUMMY_MD_INFO;
}

void mbedtls_md_init(mbedtls_md_context_t* ctx) {
}

int mbedtls_md_setup(mbedtls_md_context_t* ctx, const mbedtls_md_info_t* md_info, int hmac) {
    return 0;
}

void mbedtls_md_free(mbedtls_md_context_t* ctx) {
}

int mbedtls_md_starts(mbedtls_md_context_t* ctx) {
    auto mock = MbedtlsMock::instance();
    if (mock) {
        return mock->get().mdStarts(ctx);
    }
    return 0;
}

int mbedtls_md_finish(mbedtls_md_context_t* ctx, unsigned char* output) {
    auto mock = MbedtlsMock::instance();
    if (mock) {
        return mock->get().mdFinish(ctx, output);
    }
    return 0;
}

int mbedtls_md_update(mbedtls_md_context_t* ctx, const unsigned char* input, size_t ilen) {
    auto mock = MbedtlsMock::instance();
    if (mock) {
        return mock->get().mdUpdate(ctx, std::string((const char*)input, ilen));
    }
    return 0;
}

int mbedtls_md_clone(mbedtls_md_context_t* dst, const mbedtls_md_context_t* src) {
    return 0;
}

int mbedtls_md_hmac_starts(mbedtls_md_context_t* ctx, const unsigned char* key, size_t keylen) {
    return 0;
}

int mbedtls_md_hmac_finish(mbedtls_md_context_t* ctx, unsigned char* output) {
    return 0;
}

int mbedtls_md_hmac_update(mbedtls_md_context_t* ctx, const unsigned char* input, size_t ilen) {
    return 0;
}

int mbedtls_md_hmac_reset(mbedtls_md_context_t* ctx) {
    return 0;
}
