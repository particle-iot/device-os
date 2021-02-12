/*
 * Copyright (c) 2020 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include <stddef.h>
#include <mbedtls_config.h>

#ifndef MBEDTLS_ECP_DP_SECP192R1_ENABLED
const void* SaSi_ECPKI_GetSecp192r1DomainP(void) {
    return NULL;
}
#endif // MBEDTLS_ECP_DP_SECP192R1_ENABLED

#ifndef MBEDTLS_ECP_DP_SECP224R1_ENABLED
const void* SaSi_ECPKI_GetSecp224r1DomainP(void) {
    return NULL;
}
#endif // MBEDTLS_ECP_DP_SECP224R1_ENABLED

#ifndef MBEDTLS_ECP_DP_SECP256R1_ENABLED
const void* SaSi_ECPKI_GetSecp256r1DomainP(void) {
    return NULL;
}
#endif // MBEDTLS_ECP_DP_SECP256R1_ENABLED

#ifndef MBEDTLS_ECP_DP_SECP384R1_ENABLED
const void* SaSi_ECPKI_GetSecp384r1DomainP(void) {
    return NULL;
}
#endif // MBEDTLS_ECP_DP_SECP384R1_ENABLED

#ifndef MBEDTLS_ECP_DP_SECP521R1_ENABLED
const void* SaSi_ECPKI_GetSecp521r1DomainP(void) {
    return NULL;
}
#endif // MBEDTLS_ECP_DP_SECP521R1_ENABLED

#ifndef MBEDTLS_ECP_DP_BP512R1_ENABLED
const void* SaSi_ECPKI_GetSecp512r1DomainP(void) {
    return NULL;
}
#endif // MBEDTLS_ECP_DP_BP512R1_ENABLED

#ifndef MBEDTLS_ECP_DP_SECP192K1_ENABLED
const void* SaSi_ECPKI_GetSecp192k1DomainP(void) {
    return NULL;
}
#endif // MBEDTLS_ECP_DP_SECP192K1_ENABLED

#ifndef MBEDTLS_ECP_DP_SECP224K1_ENABLED
const void* SaSi_ECPKI_GetSecp224k1DomainP(void) {
    return NULL;
}
#endif // MBEDTLS_ECP_DP_SECP224K1_ENABLED

#ifndef MBEDTLS_ECP_DP_SECP256K1_ENABLED
const void* SaSi_ECPKI_GetSecp256k1DomainP(void) {
    return NULL;
}
#endif // MBEDTLS_ECP_DP_SECP256K1_ENABLED

// Not supported by MbedTLS as of right now
#ifndef MBEDTLS_ECP_DP_SECP160R1_ENABLED
const void* SaSi_ECPKI_GetSecp160r1DomainP(void) {
    return NULL;
}
#endif // MBEDTLS_ECP_DP_SECP256K1_ENABLED

#ifndef MBEDTLS_ECP_DP_SECP160R2_ENABLED
const void* SaSi_ECPKI_GetSecp160r2DomainP(void) {
    return NULL;
}
#endif // MBEDTLS_ECP_DP_SECP256K1_ENABLED


#ifndef MBEDTLS_ECP_DP_SECP160K1_ENABLED
const void* SaSi_ECPKI_GetSecp160k1DomainP(void) {
    return NULL;
}
#endif // MBEDTLS_ECP_DP_SECP256K1_ENABLED
