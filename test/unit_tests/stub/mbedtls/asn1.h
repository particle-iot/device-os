#pragma once

#include <stddef.h>

#define MBEDTLS_ASN1_SEQUENCE 0x10
#define MBEDTLS_ASN1_CONSTRUCTED 0x20

#ifdef __cplusplus
extern "C" {
#endif

int mbedtls_asn1_get_tag(unsigned char** p, const unsigned char* end, size_t* len, int tag);

#ifdef __cplusplus
} // extern "C"
#endif
