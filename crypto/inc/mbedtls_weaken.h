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
#pragma weak mbedtls_sha256_init
#pragma weak mbedtls_sha256_free
#pragma weak mbedtls_sha256_clone
#pragma weak mbedtls_sha256_starts
#pragma weak mbedtls_sha256_update
#pragma weak mbedtls_sha256_finish
#pragma weak mbedtls_sha256_process

// RSA
#pragma weak mbedtls_rsa_check_pubkey
#pragma weak mbedtls_rsa_check_privkey
#pragma weak mbedtls_rsa_check_pub_priv

#pragma weak mbedtls_to_system_error

#if PLATFORM_ID == 6 || PLATFORM_ID == 8
// MD4
#pragma weak mbedtls_md4_init
#pragma weak mbedtls_md4_free
#pragma weak mbedtls_md4_clone
#pragma weak mbedtls_md4_starts
#pragma weak mbedtls_md4_update
#pragma weak mbedtls_md4_finish
#pragma weak mbedtls_md4
#pragma weak mbedtls_md4_process

// MD5
#pragma weak mbedtls_md5_init
#pragma weak mbedtls_md5_free
#pragma weak mbedtls_md5_clone
#pragma weak mbedtls_md5_starts
#pragma weak mbedtls_md5_update
#pragma weak mbedtls_md5_finish
#pragma weak mbedtls_md5_process
#pragma weak mbedtls_md5

// DES
#pragma weak mbedtls_des_init
#pragma weak mbedtls_des_free
#pragma weak mbedtls_des3_init
#pragma weak mbedtls_des3_free
// #pragma weak mbedtls_des_key_set_parity
#pragma weak mbedtls_des_setkey_enc
#pragma weak mbedtls_des_setkey_dec
// #pragma weak mbedtls_des3_set3key_enc
#pragma weak mbedtls_des3_set3key_dec
#pragma weak mbedtls_des_crypt_ecb
#pragma weak mbedtls_des_crypt_cbc
#pragma weak mbedtls_des3_crypt_ecb
#pragma weak mbedtls_des3_crypt_cbc
#pragma weak mbedtls_des_setkey

// MD
#pragma weak mbedtls_md_list
#pragma weak mbedtls_md_info_from_string
#pragma weak mbedtls_md_info_from_type
#pragma weak mbedtls_md_init
#pragma weak mbedtls_md_free
#pragma weak mbedtls_md_setup
// #pragma weak mbedtls_md_clone
#pragma weak mbedtls_md_starts
// #pragma weak mbedtls_md_finish
#pragma weak mbedtls_md
#pragma weak mbedtls_md_hmac_starts
#pragma weak mbedtls_md_hmac_update
#pragma weak mbedtls_md_hmac_finish
// #pragma weak mbedtls_md_hmac_reset
#pragma weak mbedtls_md_hmac

#pragma weak mbedtls_sha512_init
#pragma weak mbedtls_sha512_free
#pragma weak mbedtls_sha512_clone
#pragma weak mbedtls_sha512_starts
#pragma weak mbedtls_sha512_update
#pragma weak mbedtls_sha512_finish
#pragma weak mbedtls_sha512_process

// MD5 with return value
#pragma weak mbedtls_md5_starts_ret
#pragma weak mbedtls_md5_update_ret
#pragma weak mbedtls_md5_finish_ret
#pragma weak mbedtls_internal_md5_process

#endif

#endif // MBEDTLS_WEAKEN
