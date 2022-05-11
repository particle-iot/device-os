/**
  ******************************************************************************
  * @file     crypto_dynalib.h
  * @authors  Andrey Tolstoy
  ******************************************************************************
  Copyright (c) 2017 Particle Industries, Inc.  All rights reserved.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program; if not, see <http://www.gnu.org/licenses/>.
  ******************************************************************************
  */

#ifndef CRYPTO_DYNALIB_H
#define CRYPTO_DYNALIB_H

#include "dynalib.h"

#if !defined(DYNALIB_IMPORT)
#include "mbedtls/aes.h"
#include "mbedtls/rsa.h"
#include "mbedtls/sha1.h"
#include "mbedtls/sha256.h"
#include "mbedtls/bignum.h"
#include "mbedtls_util.h"
#endif

#ifdef	__cplusplus
extern "C" {
#endif

DYNALIB_BEGIN(crypto)
DYNALIB_FN(0, crypto, mbedtls_set_callbacks, int(mbedtls_callbacks_t*, void*))
// AES
DYNALIB_FN(1, crypto, mbedtls_aes_init, void(mbedtls_aes_context*))
DYNALIB_FN(2, crypto, mbedtls_aes_free, void(mbedtls_aes_context*))
DYNALIB_FN(3, crypto, mbedtls_aes_setkey_enc, int(mbedtls_aes_context*, const unsigned char*, unsigned int))
DYNALIB_FN(4, crypto, mbedtls_aes_setkey_dec, int(mbedtls_aes_context*, const unsigned char*, unsigned int))
DYNALIB_FN(5, crypto, mbedtls_aes_crypt_cbc, int(mbedtls_aes_context*, int, size_t, unsigned char iv[16], const unsigned char*, unsigned char*))
DYNALIB_FN(6, crypto, mbedtls_aes_crypt_ecb, int(mbedtls_aes_context*, int, const unsigned char input[16], unsigned char output[16]))
// SHA1
DYNALIB_FN(7, crypto, mbedtls_sha1_init, void(mbedtls_sha1_context*))
DYNALIB_FN(8, crypto, mbedtls_sha1_free, void(mbedtls_sha1_context*))
DYNALIB_FN(9, crypto, mbedtls_sha1_starts, void(mbedtls_sha1_context*))
DYNALIB_FN(10, crypto, mbedtls_sha1_update, void(mbedtls_sha1_context*, const unsigned char*, size_t))
DYNALIB_FN(11, crypto, mbedtls_sha1_finish, void(mbedtls_sha1_context*, unsigned char[20]))
DYNALIB_FN(12, crypto, mbedtls_sha1, void(const unsigned char*, size_t, unsigned char[20]))
DYNALIB_FN(13, crypto, mbedtls_sha1_process, void(mbedtls_sha1_context*, const unsigned char data[64]))

DYNALIB_FN(14, crypto, mbedtls_sha1_clone, void(mbedtls_sha1_context*, const mbedtls_sha1_context*))
// RSA
DYNALIB_FN(15, crypto, mbedtls_rsa_init, void(mbedtls_rsa_context*, int, int))
DYNALIB_FN(16, crypto, mbedtls_rsa_set_padding, void(mbedtls_rsa_context*, int, int))
DYNALIB_FN(17, crypto, mbedtls_rsa_gen_key, int(mbedtls_rsa_context*, int (*f_rng)(void *, unsigned char *, size_t), void*, unsigned int, int))
DYNALIB_FN(18, crypto, mbedtls_rsa_public, int(mbedtls_rsa_context*, const unsigned char*, unsigned char*))
DYNALIB_FN(19, crypto, mbedtls_rsa_private, int(mbedtls_rsa_context*, int (*f_rng)(void *, unsigned char *, size_t), void*, const unsigned char*, unsigned char*))
DYNALIB_FN(20, crypto, mbedtls_rsa_pkcs1_encrypt, int(mbedtls_rsa_context*, int (*f_rng)(void *, unsigned char *, size_t), void*, int, size_t, const unsigned char*, unsigned char*))
DYNALIB_FN(21, crypto, mbedtls_rsa_pkcs1_decrypt, int(mbedtls_rsa_context*, int (*f_rng)(void *, unsigned char *, size_t), void*, int, size_t*, const unsigned char*, unsigned char*, size_t))
DYNALIB_FN(22, crypto, mbedtls_rsa_pkcs1_sign, int(mbedtls_rsa_context*, int (*f_rng)(void *, unsigned char *, size_t), void*, int, mbedtls_md_type_t, unsigned int, const unsigned char*, unsigned char*))
DYNALIB_FN(23, crypto, mbedtls_rsa_pkcs1_verify, int(mbedtls_rsa_context*, int (*f_rng)(void *, unsigned char *, size_t), void*, int, mbedtls_md_type_t, unsigned int, const unsigned char*, const unsigned char*))
DYNALIB_FN(24, crypto, mbedtls_rsa_free, void(mbedtls_rsa_context*))
// Bignum
DYNALIB_FN(25, crypto, mbedtls_mpi_init, void(mbedtls_mpi*))
DYNALIB_FN(26, crypto, mbedtls_mpi_free, void(mbedtls_mpi*))
DYNALIB_FN(27, crypto, mbedtls_mpi_grow, int(mbedtls_mpi*, size_t))
DYNALIB_FN(28, crypto, mbedtls_mpi_shrink, int(mbedtls_mpi*, size_t))
DYNALIB_FN(29, crypto, mbedtls_mpi_copy, int(mbedtls_mpi*, const mbedtls_mpi*))
DYNALIB_FN(30, crypto, mbedtls_mpi_swap, void(mbedtls_mpi*, mbedtls_mpi*))
DYNALIB_FN(31, crypto, mbedtls_mpi_lset, int(mbedtls_mpi*, mbedtls_mpi_sint))
DYNALIB_FN(32, crypto, mbedtls_mpi_get_bit, int(const mbedtls_mpi*, size_t))
DYNALIB_FN(33, crypto, mbedtls_mpi_set_bit, int(mbedtls_mpi*, size_t, unsigned char))
DYNALIB_FN(34, crypto, mbedtls_mpi_lsb, size_t(const mbedtls_mpi*))
DYNALIB_FN(35, crypto, mbedtls_mpi_bitlen, size_t(const mbedtls_mpi*))
DYNALIB_FN(36, crypto, mbedtls_mpi_size, size_t(const mbedtls_mpi*))
DYNALIB_FN(37, crypto, mbedtls_mpi_read_string, int(mbedtls_mpi*, int, const char *))
DYNALIB_FN(38, crypto, mbedtls_mpi_write_string, int(const mbedtls_mpi*, int, char *, size_t, size_t*))
DYNALIB_FN(39, crypto, mbedtls_mpi_read_binary, int(mbedtls_mpi*, const unsigned char *, size_t))
DYNALIB_FN(40, crypto, mbedtls_mpi_write_binary, int(const mbedtls_mpi*, unsigned char *, size_t))
DYNALIB_FN(41, crypto, mbedtls_mpi_shift_l, int(mbedtls_mpi*, size_t))
DYNALIB_FN(42, crypto, mbedtls_mpi_shift_r, int(mbedtls_mpi*, size_t))
DYNALIB_FN(43, crypto, mbedtls_mpi_cmp_abs, int(const mbedtls_mpi*, const mbedtls_mpi*))
DYNALIB_FN(44, crypto, mbedtls_mpi_cmp_mpi, int(const mbedtls_mpi*, const mbedtls_mpi*))
DYNALIB_FN(45, crypto, mbedtls_mpi_cmp_int, int(const mbedtls_mpi*, mbedtls_mpi_sint))
DYNALIB_FN(46, crypto, mbedtls_mpi_add_abs, int(mbedtls_mpi*, const mbedtls_mpi*, const mbedtls_mpi*))
DYNALIB_FN(47, crypto, mbedtls_mpi_sub_abs, int(mbedtls_mpi*, const mbedtls_mpi*, const mbedtls_mpi*))
DYNALIB_FN(48, crypto, mbedtls_mpi_add_mpi, int(mbedtls_mpi*, const mbedtls_mpi*, const mbedtls_mpi*))
DYNALIB_FN(49, crypto, mbedtls_mpi_sub_mpi, int(mbedtls_mpi*, const mbedtls_mpi*, const mbedtls_mpi*))
DYNALIB_FN(50, crypto, mbedtls_mpi_add_int, int(mbedtls_mpi*, const mbedtls_mpi*, mbedtls_mpi_sint))
DYNALIB_FN(51, crypto, mbedtls_mpi_sub_int, int(mbedtls_mpi*, const mbedtls_mpi*, mbedtls_mpi_sint))
DYNALIB_FN(52, crypto, mbedtls_mpi_mul_mpi, int(mbedtls_mpi*, const mbedtls_mpi*, const mbedtls_mpi*))
DYNALIB_FN(53, crypto, mbedtls_mpi_mul_int, int(mbedtls_mpi*, const mbedtls_mpi*, mbedtls_mpi_uint))
DYNALIB_FN(54, crypto, mbedtls_mpi_div_mpi, int(mbedtls_mpi*, mbedtls_mpi*, const mbedtls_mpi*, const mbedtls_mpi*))
DYNALIB_FN(55, crypto, mbedtls_mpi_div_int, int(mbedtls_mpi*, mbedtls_mpi*, const mbedtls_mpi*, mbedtls_mpi_sint))
DYNALIB_FN(56, crypto, mbedtls_mpi_mod_mpi, int(mbedtls_mpi*, const mbedtls_mpi*, const mbedtls_mpi*))
DYNALIB_FN(57, crypto, mbedtls_mpi_mod_int, int(mbedtls_mpi_uint *, const mbedtls_mpi*, mbedtls_mpi_sint))
DYNALIB_FN(58, crypto, mbedtls_mpi_exp_mod, int(mbedtls_mpi*, const mbedtls_mpi*, const mbedtls_mpi*, const mbedtls_mpi*, mbedtls_mpi*))
DYNALIB_FN(59, crypto, mbedtls_mpi_fill_random, int(mbedtls_mpi *X, size_t, int (*f_rng)(void *, unsigned char *, size_t), void *))
DYNALIB_FN(60, crypto, mbedtls_mpi_gcd, int(mbedtls_mpi*, const mbedtls_mpi*, const mbedtls_mpi*))
DYNALIB_FN(61, crypto, mbedtls_mpi_inv_mod, int(mbedtls_mpi*, const mbedtls_mpi*, const mbedtls_mpi*))
DYNALIB_FN(62, crypto, mbedtls_mpi_is_prime, int(const mbedtls_mpi*, int (*f_rng)(void *, unsigned char *, size_t), void *))
DYNALIB_FN(63, crypto, mbedtls_mpi_gen_prime, int(mbedtls_mpi*, size_t, int, int (*f_rng)(void *, unsigned char *, size_t), void *))
DYNALIB_FN(64, crypto, mbedtls_sha256_init, void(mbedtls_sha256_context*))
DYNALIB_FN(65, crypto, mbedtls_sha256_free, void(mbedtls_sha256_context*))
DYNALIB_FN(66, crypto, mbedtls_sha256_clone, void(mbedtls_sha256_context*, const mbedtls_sha256_context*))
DYNALIB_FN(67, crypto, mbedtls_sha256_starts, void(mbedtls_sha256_context*, int))
DYNALIB_FN(68, crypto, mbedtls_sha256_update, void(mbedtls_sha256_context*, const unsigned char*, size_t))
DYNALIB_FN(69, crypto, mbedtls_sha256_finish, void(mbedtls_sha256_context*, unsigned char output[32]))
DYNALIB_FN(70, crypto, mbedtls_sha256_process, void(mbedtls_sha256_context*, const unsigned char data[64]))
DYNALIB_FN(71, crypto, mbedtls_rsa_check_pubkey, int(const mbedtls_rsa_context*))
DYNALIB_FN(72, crypto, mbedtls_rsa_check_privkey, int(const mbedtls_rsa_context*))
DYNALIB_FN(73, crypto, mbedtls_rsa_check_pub_priv, int(const mbedtls_rsa_context*, const mbedtls_rsa_context*))
DYNALIB_FN(74, crypto, mbedtls_to_system_error, int(int))

DYNALIB_END(crypto)

#ifdef	__cplusplus
}
#endif

#endif // CRYPTO_DYNALIB_H

