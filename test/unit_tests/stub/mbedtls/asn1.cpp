#include "asn1.h"
#include "mbedtls_mock.h"

using namespace particle::test;

int mbedtls_asn1_get_tag(unsigned char** p, const unsigned char* end, size_t* len, int tag) {
    auto mock = MbedtlsMock::instance();
    if (mock) {
        return mock->get().asn1GetTag(p, end, len, tag);
    }
    return 0;
}
