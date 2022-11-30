#pragma once

#include <stdexcept>
#include <string>

#include <fakeit.hpp>

#include "mbedtls/pk.h"
#include "mbedtls/asn1.h"

namespace particle::test {

class Mbedtls {
public:
    virtual ~Mbedtls() = default;

    // md.h
    virtual int mdStarts(mbedtls_md_context_t* ctx) = 0;
    virtual int mdFinish(mbedtls_md_context_t* ctx, unsigned char* output) = 0;
    virtual int mdUpdate(mbedtls_md_context_t* ctx, std::string input) = 0;

    // pk.h
    virtual int pkParsePublicKey(mbedtls_pk_context* ctx, const unsigned char* key, size_t keylen) = 0;
    virtual int pkVerify(mbedtls_pk_context* ctx, mbedtls_md_type_t md_alg, const unsigned char* hash,
            size_t hash_len, const unsigned char* sig, size_t sig_len) = 0;

    // asn1.h
    virtual int asn1GetTag(unsigned char** p, const unsigned char* end, size_t* len, int tag) = 0;
};

class MbedtlsMock: public fakeit::Mock<Mbedtls> {
public:
    MbedtlsMock() {
        if (s_instance) {
            throw std::runtime_error("MbedtlsMock is already instantiated");
        }
        s_instance = this;
    }

    ~MbedtlsMock() {
        s_instance = nullptr;
    }

    static MbedtlsMock* instance() {
        return s_instance;
    }

private:
    static MbedtlsMock* s_instance;
};

} // namespace particle::test
