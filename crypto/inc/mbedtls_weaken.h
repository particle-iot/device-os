#ifndef MBEDTLS_WEAKEN
#define MBEDTLS_WEAKEN

// Weaken these functions
#ifndef MBEDTLS_OVERRIDE_MD_LIST
#pragma weak mbedtls_md_list
#pragma weak mbedtls_md_info_from_string
#pragma weak mbedtls_md_info_from_type
#endif // MBEDTLS_OVERRIDE_MD_LIST

#pragma weak mbedtls_aes_init
#pragma weak mbedtls_aes_free
#pragma weak mbedtls_aes_setkey_enc
#pragma weak mbedtls_aes_setkey_dec
#pragma weak mbedtls_aes_crypt_cbc
#pragma weak mbedtls_aes_crypt_ecb
// SHA1
#pragma weak mbedtls_sha1_init
#pragma weak mbedtls_sha1_free
#pragma weak mbedtls_sha1_starts
#pragma weak mbedtls_sha1_update
#pragma weak mbedtls_sha1_finish
#pragma weak mbedtls_sha1
#pragma weak mbedtls_sha1_process
#pragma weak mbedtls_sha1_clone
// RSA
#pragma weak mbedtls_rsa_init
#pragma weak mbedtls_rsa_set_padding
#pragma weak mbedtls_rsa_gen_key
#pragma weak mbedtls_rsa_public
#pragma weak mbedtls_rsa_private
#pragma weak mbedtls_rsa_pkcs1_encrypt
#pragma weak mbedtls_rsa_pkcs1_decrypt
#pragma weak mbedtls_rsa_pkcs1_sign
#pragma weak mbedtls_rsa_pkcs1_verify
#pragma weak mbedtls_rsa_free
// Bignum
#pragma weak mbedtls_mpi_init
#pragma weak mbedtls_mpi_free
#pragma weak mbedtls_mpi_grow
#pragma weak mbedtls_mpi_shrink
#pragma weak mbedtls_mpi_copy
#pragma weak mbedtls_mpi_swap
#pragma weak mbedtls_mpi_lset
#pragma weak mbedtls_mpi_get_bit
#pragma weak mbedtls_mpi_set_bit
#pragma weak mbedtls_mpi_lsb
#pragma weak mbedtls_mpi_bitlen
#pragma weak mbedtls_mpi_size
#pragma weak mbedtls_mpi_read_string
#pragma weak mbedtls_mpi_write_string
#pragma weak mbedtls_mpi_read_binary
#pragma weak mbedtls_mpi_write_binary
#pragma weak mbedtls_mpi_shift_l
#pragma weak mbedtls_mpi_shift_r
#pragma weak mbedtls_mpi_cmp_abs
#pragma weak mbedtls_mpi_cmp_mpi
#pragma weak mbedtls_mpi_cmp_int
#pragma weak mbedtls_mpi_add_abs
#pragma weak mbedtls_mpi_sub_abs
#pragma weak mbedtls_mpi_add_mpi
#pragma weak mbedtls_mpi_sub_mpi
#pragma weak mbedtls_mpi_add_int
#pragma weak mbedtls_mpi_sub_int
#pragma weak mbedtls_mpi_mul_mpi
#pragma weak mbedtls_mpi_mul_int
#pragma weak mbedtls_mpi_div_mpi
#pragma weak mbedtls_mpi_div_int
#pragma weak mbedtls_mpi_mod_mpi
#pragma weak mbedtls_mpi_mod_int
#pragma weak mbedtls_mpi_exp_mod
#pragma weak mbedtls_mpi_fill_random
#pragma weak mbedtls_mpi_gcd
#pragma weak mbedtls_mpi_inv_mod
#pragma weak mbedtls_mpi_is_prime
#pragma weak mbedtls_mpi_gen_prime

// SHA256
#if PLATFORM_ID == 10
#pragma weak mbedtls_sha256_init
#pragma weak mbedtls_sha256_free
#pragma weak mbedtls_sha256_clone
#pragma weak mbedtls_sha256_starts
#pragma weak mbedtls_sha256_update
#pragma weak mbedtls_sha256_finish
#pragma weak mbedtls_sha256_process
#endif

#endif // MBEDTLS_WEAKEN
