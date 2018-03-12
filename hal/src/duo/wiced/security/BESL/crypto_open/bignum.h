/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/* Originally taken from TropicSSL
 * https://gitorious.org/tropicssl/
 * commit: 92bb3462dfbdb4568c92be19e8904129a17b1eed
 * Whitespace converted (Tab to 4 spaces, LF to CRLF)
 * int arguments/returns changed to int32_t
 * Remove mpi_read_file/mpi_write_file
 * Add const to arguments
 * Add t_dbl type for GCC + IAR
 */

/**
 * \file bignum.h
 *
 *  Based on XySSL: Copyright (C) 2006-2008  Christophe Devine
 *
 *  Copyright (C) 2009  Paul Bakker <polarssl_maintainer at polarssl dot org>
 *
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the names of PolarSSL or XySSL nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 *  TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef TROPICSSL_BIGNUM_H
#define TROPICSSL_BIGNUM_H

#include <stdint.h>
#include "crypto_structures.h"

#define TROPICSSL_ERR_MPI_FILE_IO_ERROR                     -0x0002
#define TROPICSSL_ERR_MPI_BAD_INPUT_DATA                    -0x0004
#define TROPICSSL_ERR_MPI_INVALID_CHARACTER                 -0x0006
#define TROPICSSL_ERR_MPI_BUFFER_TOO_SMALL                  -0x0008
#define TROPICSSL_ERR_MPI_NEGATIVE_VALUE                    -0x000A
#define TROPICSSL_ERR_MPI_DIVISION_BY_ZERO                  -0x000C
#define TROPICSSL_ERR_MPI_NOT_ACCEPTABLE                    -0x000E

#define MPI_CHK(f) if( ( ret = f ) != 0 ) goto cleanup

/*
 * Define the base integer type, architecture-wise
 */
#if defined(TROPICSSL_HAVE_INT8)
typedef unsigned char  t_int;
typedef unsigned short t_dbl;
#else
#if defined(TROPICSSL_HAVE_INT16)
typedef unsigned short t_int;
typedef uint32_t  t_dbl;
#else
  typedef uint32_t t_int;
  #if defined(_MSC_VER) && defined(_M_IX86)
  typedef unsigned __int64 t_dbl;
  #elif defined(__amd64__) || defined(__x86_64__)    || \
        defined(__ppc64__) || defined(__powerpc64__) || \
        defined(__ia64__)  || defined(__alpha__)
    typedef uint32_t t_dbl __attribute__((mode(TI)));
  #elif defined(__GNUC__) || defined(__IAR_SYSTEMS_ICC__)
    typedef unsigned long long t_dbl;
  #else
    typedef uint32_t t_dbl;
  #endif
#endif
#endif

/**
 * \brief          MPI structure
 */

#ifdef __cplusplus
extern "C" {
#endif

    /**
     * \brief          Initialize one MPI
     */
    void mpi_init(mpi * X);

    /**
     * \brief          Unallocate one MPI
     */
    void mpi_free(mpi * X);

    /**
     * \brief          Enlarge to the specified number of limbs
     *
     * \return         0 if successful,
     *                 1 if memory allocation failed
     */
    int32_t mpi_grow(mpi *X, int32_t nblimbs);

    /**
     * \brief          Copy the contents of Y into X
     *
     * \return         0 if successful,
     *                 1 if memory allocation failed
     */
    int32_t mpi_copy(mpi *X, const mpi *Y);

    /**
     * \brief          Swap the contents of X and Y
     */
    void mpi_swap(mpi * X, mpi * Y);

    /**
     * \brief          Set value from integer
     *
     * \return         0 if successful,
     *                 1 if memory allocation failed
     */
    int32_t mpi_lset(mpi *X, int32_t z);

    /**
     * \brief          Return the number of least significant bits
     */
    uint32_t mpi_lsb(const mpi *X);

    /**
     * \brief          Return the number of most significant bits
     */
    uint32_t mpi_msb(const mpi *X);

    /**
     * \brief          Return the total size in bytes
     */
    uint32_t mpi_size(const mpi *X);

    /**
     * \brief          Import from an ASCII string
     *
     * \param X        destination mpi
     * \param radix    input numeric base
     * \param s        null-terminated string buffer
     *
     * \return         0 if successful, or an TROPICSSL_ERR_MPI_XXX error code
     */
    int32_t mpi_read_string(mpi *X, int32_t radix, const char *s);

    /**
     * \brief          Export into an ASCII string
     *
     * \param X        source mpi
     * \param radix    output numeric base
     * \param s        string buffer
     * \param slen     string buffer size
     *
     * \return         0 if successful, or an TROPICSSL_ERR_MPI_XXX error code
     *
     * \note           Call this function with *slen = 0 to obtain the
     *                 minimum required buffer size in *slen.
     */
    int32_t mpi_write_string(const mpi *X, int32_t radix, char *s, int32_t *slen);

    /**
     * \brief          Import X from unsigned binary data, big endian
     *
     * \param X        destination mpi
     * \param buf      input buffer
     * \param buflen   input buffer size
     *
     * \return         0 if successful,
     *                 1 if memory allocation failed
     */
    int32_t mpi_read_binary(mpi *X, const unsigned char *buf, int32_t buflen);

    /**
     * \brief          Export X into unsigned binary data, big endian
     *
     * \param X        source mpi
     * \param buf      output buffer
     * \param buflen   output buffer size
     *
     * \return         0 if successful,
     *                 TROPICSSL_ERR_MPI_BUFFER_TOO_SMALL if buf isn't large enough
     *
     * \note           Call this function with *buflen = 0 to obtain the
     *                 minimum required buffer size in *buflen.
     */
    int32_t mpi_write_binary(const mpi *X, unsigned char *buf, int32_t buflen);

    /**
     * \brief          Left-shift: X <<= count
     *
     * \return         0 if successful,
     *                 1 if memory allocation failed
     */
    int32_t mpi_shift_l(mpi *X, int32_t count);

    /**
     * \brief          Right-shift: X >>= count
     *
     * \return         0 if successful,
     *                 1 if memory allocation failed
     */
    int32_t mpi_shift_r(mpi *X, int32_t count);

    /**
     * \brief          Compare unsigned values
     *
     * \return         1 if |X| is greater than |Y|,
     *                -1 if |X| is lesser  than |Y| or
     *                 0 if |X| is equal to |Y|
     */
    int32_t mpi_cmp_abs(const mpi *X, const mpi *Y);

    /**
     * \brief          Compare signed values
     *
     * \return         1 if X is greater than Y,
     *                -1 if X is lesser  than Y or
     *                 0 if X is equal to Y
     */
    int32_t mpi_cmp_mpi(const mpi *X, const mpi *Y);

    /**
     * \brief          Compare signed values
     *
     * \return         1 if X is greater than z,
     *                -1 if X is lesser  than z or
     *                 0 if X is equal to z
     */
    int32_t mpi_cmp_int(const mpi *X, int32_t z);

    /**
     * \brief          Unsigned addition: X = |A| + |B|
     *
     * \return         0 if successful,
     *                 1 if memory allocation failed
     */
    int32_t mpi_add_abs(mpi *X, const mpi *A, const mpi *B);

    /**
     * \brief          Unsigned substraction: X = |A| - |B|
     *
     * \return         0 if successful,
     *                 TROPICSSL_ERR_MPI_NEGATIVE_VALUE if B is greater than A
     */
    int32_t mpi_sub_abs(mpi *X, const mpi *A, const mpi *B);

    /**
     * \brief          Signed addition: X = A + B
     *
     * \return         0 if successful,
     *                 1 if memory allocation failed
     */
    int32_t mpi_add_mpi(mpi *X, const mpi *A, const mpi *B);

    /**
     * \brief          Signed substraction: X = A - B
     *
     * \return         0 if successful,
     *                 1 if memory allocation failed
     */
    int32_t mpi_sub_mpi(mpi *X, const mpi *A, const mpi *B);

    /**
     * \brief          Signed addition: X = A + b
     *
     * \return         0 if successful,
     *                 1 if memory allocation failed
     */
    int32_t mpi_add_int(mpi *X, const mpi *A, int32_t b);

    /**
     * \brief          Signed substraction: X = A - b
     *
     * \return         0 if successful,
     *                 1 if memory allocation failed
     */
    int32_t mpi_sub_int(mpi *X, const mpi *A, int32_t b);

    /**
     * \brief          Baseline multiplication: X = A * B
     *
     * \return         0 if successful,
     *                 1 if memory allocation failed
     */
    int32_t mpi_mul_mpi(mpi *X, const mpi *A, const mpi *B);

    /**
     * \brief          Baseline multiplication: X = A * b
     *
     * \return         0 if successful,
     *                 1 if memory allocation failed
     */
    int32_t mpi_mul_int(mpi *X, const mpi *A, t_int b);

    /**
     * \brief          Division by mpi: A = Q * B + R
     *
     * \return         0 if successful,
     *                 1 if memory allocation failed,
     *                 TROPICSSL_ERR_MPI_DIVISION_BY_ZERO if B == 0
     *
     * \note           Either Q or R can be NULL.
     */
    int32_t mpi_div_mpi(mpi *Q, mpi *R, const mpi *A, const mpi *B);

    /**
     * \brief          Division by int: A = Q * b + R
     *
     * \return         0 if successful,
     *                 1 if memory allocation failed,
     *                 TROPICSSL_ERR_MPI_DIVISION_BY_ZERO if b == 0
     *
     * \note           Either Q or R can be NULL.
     */
    int32_t mpi_div_int(mpi *Q, mpi *R, const mpi *A, int32_t b);

    /**
     * \brief          Modulo: R = A mod B
     *
     * \return         0 if successful,
     *                 1 if memory allocation failed,
     *                 TROPICSSL_ERR_MPI_DIVISION_BY_ZERO if B == 0
     */
    int32_t mpi_mod_mpi(mpi *R, const mpi *A, const mpi *B);

    /**
     * \brief          Modulo: r = A mod b
     *
     * \return         0 if successful,
     *                 1 if memory allocation failed,
     *                 TROPICSSL_ERR_MPI_DIVISION_BY_ZERO if b == 0
     */
    int32_t mpi_mod_int(t_int *r, const mpi *A, int32_t b);

    /**
     * \brief          Sliding-window exponentiation: X = A^E mod N
     *
     * \return         0 if successful,
     *                 1 if memory allocation failed,
     *                 TROPICSSL_ERR_MPI_BAD_INPUT_DATA if N is negative or even
     *
     * \note           _RR is used to avoid re-computing R*R mod N across
     *                 multiple calls, which speeds up things a bit. It can
     *                 be set to NULL if the extra performance is unneeded.
     */
    int32_t mpi_exp_mod(mpi *X, const mpi *A, const mpi *E, const mpi *N, mpi *_RR);

    /**
     * \brief          Greatest common divisor: G = gcd(A, B)
     *
     * \return         0 if successful,
     *                 1 if memory allocation failed
     */
    int32_t mpi_gcd(mpi *G, const mpi *A, const mpi *B);

    /**
     * \brief          Modular inverse: X = A^-1 mod N
     *
     * \return         0 if successful,
     *                 1 if memory allocation failed,
     *                 TROPICSSL_ERR_MPI_BAD_INPUT_DATA if N is negative or nil
     *                 TROPICSSL_ERR_MPI_NOT_ACCEPTABLE if A has no inverse mod N
     */
    int32_t mpi_inv_mod(mpi *X, const mpi *A, const mpi *N);

    /**
     * \brief          Miller-Rabin primality test
     *
     * \return         0 if successful (probably prime),
     *                 1 if memory allocation failed,
     *                 TROPICSSL_ERR_MPI_NOT_ACCEPTABLE if X is not prime
     */
    int32_t mpi_is_prime(mpi *X, int32_t (*f_rng)(void *), void *p_rng);

    /**
     * \brief          Prime number generation
     *
     * \param X        destination mpi
     * \param nbits    required size of X in bits
     * \param dh_flag  if 1, then (X-1)/2 will be prime too
     * \param f_rng    RNG function
     * \param p_rng    RNG parameter
     *
     * \return         0 if successful (probably prime),
     *                 1 if memory allocation failed,
     *                 TROPICSSL_ERR_MPI_BAD_INPUT_DATA if nbits is < 3
     */
    int32_t mpi_gen_prime(mpi *X, int32_t nbits, int32_t dh_flag,
              int32_t (*f_rng)(void *), void *p_rng);

    /**
     * \brief          Checkup routine
     *
     * \return         0 if successful, or 1 if the test failed
     */
    int32_t mpi_self_test(int32_t verbose);

#ifdef __cplusplus
}
#endif
#endif                /* bignum.h */
