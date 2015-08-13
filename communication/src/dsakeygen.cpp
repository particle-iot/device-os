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
#include "tropicssl/rsa.h"
#include "dsakeygen.h"

using namespace std;

class RSADERCommon {

    uint8_t* buffer;
    int length;

protected:

    void write_mpi(const mpi* data) {
        int size = mpi_size(data);
        if (length>=size) {
            mpi_write_binary(data, buffer, size);
            length -= size;
            buffer += size;
        }
    }

    void write_integer_1024(const mpi* data) {
        uint8_t header[] = { 0x02, 0x81, 0x81, 0 };  // padded with an extra 0 byte to ensure number is positive
        write(header, sizeof(header));
        write_mpi(data);
    }

    void write_integer_512(const mpi* data) {
        uint8_t header[] = { 0x02, 0x41, 0 };  // padded with an extra 0 byte to ensure number is positive
        write(header, sizeof(header));
        write_mpi(data);
    }

    void write_public_exponent() {
        uint8_t data[] = { 2, 3, 1, 0, 1 };
        write(data, 5);
    }

    void write(const uint8_t* data, size_t length) {
        while (length && this->length) {
            *this->buffer++ = *data++;
            length--;
            this->length--;
        }
    }

    void set_buffer(void* buffer, size_t size) {
        this->buffer = (uint8_t*)buffer;
        this->length = size;
    }

};

class RSAPrivateKeyWriter : RSADERCommon {

    void write_sequence_header() {
        // sequence tag and version
        uint8_t header[] = { 0x30, 0x82, 0x2, 0x5F, 2, 1, 0 };
        write(header, sizeof(header));
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
        set_buffer(buf, length);
        write_sequence_header();
        write_integer_1024(&ctx.N);
        write_public_exponent();        // 5
        write_integer_1024(&ctx.D);      // 132 * 2
        write_integer_512(&ctx.P);       // 67 * 5
        write_integer_512(&ctx.Q);
        write_integer_512(&ctx.DP);
        write_integer_512(&ctx.DQ);
        write_integer_512(&ctx.QP);      // total is 604 bytes, 0x25C
    }
};


/**
 * Returns 0 on success.
 * @param f_rng     The random number generator function.
 * @param p_rng     The argument to the RNG function.
 * @return
 */
int gen_rsa_key(uint8_t* buffer, size_t max_length, int (*f_rng) (void *), void *p_rng)
{
    rsa_context rsa;

    rsa_init(&rsa, RSA_PKCS_V15, RSA_RAW, f_rng, p_rng);

    int failure = rsa_gen_key(&rsa, 1024, 65537);
    if (!failure)
    {
        RSAPrivateKeyWriter().write_private_key(buffer, max_length, rsa);
    }

    rsa_free(&rsa);
    return failure;
}

