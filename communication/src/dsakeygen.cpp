/**
 ******************************************************************************
 * @file    dsakeygen.cpp
 * @authors Matthew McGowan
 * @date    24 February 2015
 ******************************************************************************
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

#include <cstdlib>
#include <cstdint>

#include "dsakeygen.h"
#include "hal_platform.h"
#include "protocol_selector.h"

#if HAL_PLATFORM_CLOUD_TCP

#ifdef USE_MBEDTLS
#include "mbedtls/rsa.h"
#include "mbedtls_compat.h"
#else
# if PLATFORM_ID == 6 || PLATFORM_ID == 8
#  include "wiced_security.h"
#  include "crypto_open/bignum.h"
# else
#  include "tropicssl/rsa.h"
# endif
#endif

using namespace std;

class RSADERCommon {
private:
    uint8_t* buffer_;
    int length_;
    int written_;

protected:

    int written() const {
        return written_;
    }

    void write_length(int len) {
        uint8_t len_field = 0x00;

        if (len < 128) {
            // Length can be encoded as a single byte
            len_field = (uint8_t)len;
            write(&len_field, sizeof(len_field));
        } else {
            // Length has to be encoded as:
            // (0x80 | number of length bytes) (length byte 0) ... (length byte N)
            len_field = 0x80;
            mpi intlen;
            mpi_init(&intlen);
            mpi_lset(&intlen, len);
            int bytelen = mpi_size(&intlen);
            len_field |= (uint8_t)bytelen;

            write(&len_field, sizeof(len_field));
            write_mpi(&intlen);

            mpi_free(&intlen);
        }
    }

    void write_mpi(const mpi* data, int fixedLength = -1) {
        int len = fixedLength == -1 ? mpi_size(data) : fixedLength;
        if (length_ >= len) {
            mpi_write_binary(data, buffer_, len);
            write(nullptr, len);
        }
    }

    void write_integer_immediate(int value, int fixedLength = -1) {
        mpi integer;
        mpi_init(&integer);
        mpi_lset(&integer, value);
        write_integer(&integer, fixedLength);
        mpi_free(&integer);
    }

    void write_integer(const mpi* data, int fixedLength = -1) {
        int len = fixedLength == -1 ? mpi_size(data) : fixedLength;
        uint8_t tmp = 0x02; // INTEGER

        // Write type
        write(&tmp, sizeof(tmp));

        write_length(len);

        write_mpi(data, fixedLength);
    }

    void write(const uint8_t* data, size_t length) {
        while (length && length_ > 0) {
          if (buffer_) {
            if (data)
              *buffer_++ = *data++;
            else
              buffer_++;
          }
          length--;
          length_--;
          written_++;
        }
    }

    void set_buffer(void* buffer, size_t size) {
        buffer_ = (uint8_t*)buffer;
        length_ = size;
        written_ = 0;
    }

};

class RSAPrivateKeyWriter : RSADERCommon {

    void write_sequence_header(int len) {
        uint8_t tag = 0x30; // SEQUENCE
        write(&tag, sizeof(tag));
        write_length(len);
    }

public:

    /**
     * Writes a DER
     * @param modulus
     * @param D
     * @param P
     * @param Q
     * @param DP
     * @param DQ
     * @param QP
     */
    void write_private_key(uint8_t* buf, size_t length, rsa_context& ctx) {
        // Dummy run to calculate sequence length
        set_buffer(nullptr, length);
        write_private_key_parts(ctx);
        int seq_len = written();

        set_buffer(buf, length);
        write_sequence_header(seq_len);
        write_private_key_parts(ctx);
    }

    void write_private_key_parts(rsa_context& ctx) {
        write_integer_immediate(0, 1);
        write_integer(&ctx.N, 129);
        write_integer(&ctx.E, 3);
        write_integer(&ctx.D, 129);
        write_integer(&ctx.P, 65);
        write_integer(&ctx.Q, 65);
        write_integer(&ctx.DP, 65);
        write_integer(&ctx.DQ, 65);
        write_integer(&ctx.QP, 65);
    }
};

#ifdef USE_MBEDTLS
// mbedTLS-compatible wrapper for RNG callback exposed via API
struct RNGCallbackData {
    int32_t (*f_rng)(void*);
    void *p_rng;
};

static int rngCallback(void* p, unsigned char* data, size_t size) {
    RNGCallbackData* d = (RNGCallbackData*)p;
    while (size >= 4) {
        *((uint32_t*)data) = d->f_rng(d->p_rng);
        data += 4;
        size -= 4;
    }
    while (size-- > 0) {
        *data++ = d->f_rng(d->p_rng);
    }
    return 0;
}
#endif

/**
 * Returns 0 on success.
 * @param f_rng     The random number generator function.
 * @param p_rng     The argument to the RNG function.
 * @return
 */
int gen_rsa_key(uint8_t* buffer, size_t max_length, int32_t (*f_rng) (void *), void *p_rng)
{
    rsa_context rsa;

#ifdef USE_MBEDTLS
    mbedtls_rsa_init(&rsa, MBEDTLS_RSA_PKCS_V15, 0);
    RNGCallbackData d = { f_rng, p_rng };
    int failure = mbedtls_rsa_gen_key(&rsa, rngCallback, &d, 1024, 65537);
#else
# if PLATFORM_ID == 6 || PLATFORM_ID == 8
    rsa_init(&rsa, RSA_PKCS_V15, RSA_RAW, f_rng, p_rng);
# else
    rsa_init(&rsa, RSA_PKCS_V15, RSA_RAW, (int(*)(void*))f_rng, p_rng);
# endif
    int failure = rsa_gen_key(&rsa, 1024, 65537);
#endif // USE_MBEDTLS

    if (!failure)
    {
        RSAPrivateKeyWriter().write_private_key(buffer, max_length, rsa);
    }

    rsa_free(&rsa);
    return failure;
}

#endif
