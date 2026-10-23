#pragma once

// The tests run a DTLS server using the same mbedtls build as the device
#define MBEDTLS_SSL_SRV_C

#undef MBEDTLS_SSL_CID_IN_LEN_MAX
#define MBEDTLS_SSL_CID_IN_LEN_MAX 8
