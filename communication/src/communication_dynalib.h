/**
  ******************************************************************************
  * @file    communication_dynalib.h
  * @authors  Matthew McGowan
  ******************************************************************************
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

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

#pragma once

#include "dynalib.h"
#include "protocol_selector.h"
#include "hal_platform.h"

#if PLATFORM_ID == 6 || PLATFORM_ID == 8
#include "crypto/crypto_structures.h"
#endif

#ifdef	__cplusplus
extern "C" {
#endif

DYNALIB_BEGIN(communication)

DYNALIB_FN(0, communication, spark_protocol_instance, ProtocolFacade*(void))
DYNALIB_FN(1, communication, spark_protocol_set_product_id, void(ProtocolFacade*, product_id_t, unsigned, void*))
DYNALIB_FN(2, communication, spark_protocol_set_product_firmware_version, void(ProtocolFacade*, product_firmware_version_t, unsigned, void*))
DYNALIB_FN(3, communication, spark_protocol_get_product_details, void(ProtocolFacade*, product_details_t*, void*))
DYNALIB_FN(4, communication, spark_protocol_communications_handlers, void(ProtocolFacade*, CommunicationsHandlers*))
DYNALIB_FN(5, communication, spark_protocol_init, void(ProtocolFacade*, const char*, const SparkKeys&, const SparkCallbacks&, const SparkDescriptor&, void*))
DYNALIB_FN(6, communication, spark_protocol_handshake, int(ProtocolFacade*, void*))
DYNALIB_FN(7, communication, spark_protocol_event_loop, bool(ProtocolFacade* protocol, void*))
DYNALIB_FN(8, communication, spark_protocol_is_initialized, bool(ProtocolFacade*))
DYNALIB_FN(9, communication, spark_protocol_presence_announcement, int(ProtocolFacade*, uint8_t*, const uint8_t*, void*))
DYNALIB_FN(10, communication, spark_protocol_send_event, bool(ProtocolFacade*, const char*, const char*, int, uint32_t, void*))
DYNALIB_FN(11, communication, spark_protocol_send_subscription_device, bool(ProtocolFacade*, const char*, const char*, void*))
DYNALIB_FN(12, communication, spark_protocol_send_subscription_scope, bool(ProtocolFacade*, const char*, SubscriptionScope::Enum, void*))
DYNALIB_FN(13, communication, spark_protocol_add_event_handler, bool(ProtocolFacade*, const char*, EventHandler, SubscriptionScope::Enum, const char*, void*))
DYNALIB_FN(14, communication, spark_protocol_send_time_request, bool(ProtocolFacade*, void*))
DYNALIB_FN(15, communication, spark_protocol_send_subscriptions, void(ProtocolFacade*, void*))

#if !defined(PARTICLE_PROTOCOL) || HAL_PLATFORM_CLOUD_TCP
DYNALIB_FN(16, communication, decrypt_rsa, int(const uint8_t*, const uint8_t*, uint8_t*, int32_t))
DYNALIB_FN(17, communication, gen_rsa_key, int(uint8_t*, size_t, int32_t(*)(void*), void*))
DYNALIB_FN(18, communication, extract_public_rsa_key, void(uint8_t*, const uint8_t*))
#define BASE_IDX 19 // Base index for all subsequent functions
#else
#define BASE_IDX 16
#endif

DYNALIB_FN(BASE_IDX + 0, communication, spark_protocol_remove_event_handlers, void(ProtocolFacade*, const char*, void*))

#if defined(PARTICLE_PROTOCOL) && HAL_PLATFORM_CLOUD_UDP
DYNALIB_FN(BASE_IDX + 1, communication, gen_ec_key, int(uint8_t*, size_t, int(*)(void*, uint8_t*, size_t), void*))
DYNALIB_FN(BASE_IDX + 2, communication, extract_public_ec_key, int(uint8_t*, size_t, const uint8_t*))
#define BASE_IDX2 (BASE_IDX + 3)
#else
#define BASE_IDX2 (BASE_IDX + 1)
#endif

DYNALIB_FN(BASE_IDX2 + 0, communication, spark_protocol_set_connection_property,
           int(ProtocolFacade*, unsigned, unsigned, void*, void*))
DYNALIB_FN(BASE_IDX2 + 1, communication, spark_protocol_command, int(ProtocolFacade* protocol, ProtocolCommands::Enum cmd, uint32_t data, void* reserved))
DYNALIB_FN(BASE_IDX2 + 2, communication, spark_protocol_time_request_pending, bool(ProtocolFacade*, void*))
DYNALIB_FN(BASE_IDX2 + 3, communication, spark_protocol_time_last_synced, system_tick_t(ProtocolFacade*, time_t*, void*))

#if PLATFORM_ID == 6 || PLATFORM_ID == 8
DYNALIB_FN(BASE_IDX2 + 4, communication, aes_setkey_enc, void(aes_context_t*, const unsigned char*, uint32_t))
DYNALIB_FN(BASE_IDX2 + 5, communication, aes_setkey_dec, void(aes_context_t*, const unsigned char*, uint32_t))
DYNALIB_FN(BASE_IDX2 + 6, communication, aes_crypt_cbc, void(aes_context_t*, aes_mode_type_t, uint32_t, unsigned char iv[16], const unsigned char*, unsigned char*))
DYNALIB_FN(BASE_IDX2 + 7, communication, sha1_starts, void(sha1_context*))
DYNALIB_FN(BASE_IDX2 + 8, communication, sha1_update, void(sha1_context*, const unsigned char*, int32_t))
DYNALIB_FN(BASE_IDX2 + 9, communication, sha1_finish, void(sha1_context*, unsigned char[20]))
DYNALIB_FN(BASE_IDX2 + 10, communication, sha1, void(const unsigned char*, int32_t, unsigned char[20]))
DYNALIB_FN(BASE_IDX2 + 11, communication, sha1_hmac_starts, void(sha1_context*, const unsigned char*, uint32_t))
DYNALIB_FN(BASE_IDX2 + 12, communication, sha1_hmac_update, void(sha1_context*, const unsigned char*, uint32_t))
DYNALIB_FN(BASE_IDX2 + 13, communication, sha1_hmac_finish, void(sha1_context*, unsigned char[20]))
DYNALIB_FN(BASE_IDX2 + 14, communication, sha1_hmac, void(const unsigned char*, int32_t, const unsigned char*, int32_t, unsigned char[20]))
DYNALIB_FN(BASE_IDX2 + 15, communication, mpi_init, void(mpi *))
DYNALIB_FN(BASE_IDX2 + 16, communication, mpi_free, void(mpi *))
DYNALIB_FN(BASE_IDX2 + 17, communication, mpi_grow, int32_t(mpi *, int32_t))
DYNALIB_FN(BASE_IDX2 + 18, communication, mpi_copy, int32_t(mpi *, const mpi *))
DYNALIB_FN(BASE_IDX2 + 19, communication, mpi_swap, void(mpi *, mpi *))
DYNALIB_FN(BASE_IDX2 + 20, communication, mpi_lset, int32_t(mpi *, int32_t))
DYNALIB_FN(BASE_IDX2 + 21, communication, mpi_lsb, uint32_t(const mpi *))
DYNALIB_FN(BASE_IDX2 + 22, communication, mpi_msb, uint32_t(const mpi *))
DYNALIB_FN(BASE_IDX2 + 23, communication, mpi_size, uint32_t(const mpi *))
DYNALIB_FN(BASE_IDX2 + 24, communication, mpi_read_string, int32_t(mpi *, int32_t, const char *))
DYNALIB_FN(BASE_IDX2 + 25, communication, mpi_write_string, int32_t(const mpi *, int32_t, char *, int32_t *))
DYNALIB_FN(BASE_IDX2 + 26, communication, mpi_read_binary, int32_t(mpi *, const unsigned char *, int32_t))
DYNALIB_FN(BASE_IDX2 + 27, communication, mpi_write_binary, int32_t(const mpi *, unsigned char *, int32_t))
DYNALIB_FN(BASE_IDX2 + 28, communication, mpi_shift_l, int32_t(mpi *, int32_t))
DYNALIB_FN(BASE_IDX2 + 29, communication, mpi_shift_r, int32_t(mpi *, int32_t))
DYNALIB_FN(BASE_IDX2 + 30, communication, mpi_cmp_abs, int32_t(const mpi *, const mpi *))
DYNALIB_FN(BASE_IDX2 + 31, communication, mpi_cmp_mpi, int32_t(const mpi *, const mpi *))
DYNALIB_FN(BASE_IDX2 + 32, communication, mpi_cmp_int, int32_t(const mpi *, int32_t))
DYNALIB_FN(BASE_IDX2 + 33, communication, mpi_add_abs, int32_t(mpi *, const mpi *, const mpi *))
DYNALIB_FN(BASE_IDX2 + 34, communication, mpi_sub_abs, int32_t(mpi *, const mpi *, const mpi *))
DYNALIB_FN(BASE_IDX2 + 35, communication, mpi_add_mpi, int32_t(mpi *, const mpi *, const mpi *))
DYNALIB_FN(BASE_IDX2 + 36, communication, mpi_sub_mpi, int32_t(mpi *, const mpi *, const mpi *))
DYNALIB_FN(BASE_IDX2 + 37, communication, mpi_add_int, int32_t(mpi *, const mpi *, int32_t))
DYNALIB_FN(BASE_IDX2 + 38, communication, mpi_sub_int, int32_t(mpi *, const mpi *, int32_t))
DYNALIB_FN(BASE_IDX2 + 39, communication, mpi_mul_mpi, int32_t(mpi *, const mpi *, const mpi *))
DYNALIB_FN(BASE_IDX2 + 40, communication, mpi_mul_int, int32_t(mpi *, const mpi *, t_int))
DYNALIB_FN(BASE_IDX2 + 41, communication, mpi_div_mpi, int32_t(mpi *, mpi *, const mpi *, const mpi *))
DYNALIB_FN(BASE_IDX2 + 42, communication, mpi_div_int, int32_t(mpi *, mpi *, const mpi *, int32_t))
DYNALIB_FN(BASE_IDX2 + 43, communication, mpi_mod_mpi, int32_t(mpi *, const mpi *, const mpi *))
DYNALIB_FN(BASE_IDX2 + 44, communication, mpi_mod_int, int32_t(t_int *, const mpi *, int32_t))
DYNALIB_FN(BASE_IDX2 + 45, communication, mpi_exp_mod, int32_t(mpi *, const mpi *, const mpi *, const mpi *, mpi *))
DYNALIB_FN(BASE_IDX2 + 46, communication, mpi_gcd, int32_t(mpi *, const mpi *, const mpi *))
DYNALIB_FN(BASE_IDX2 + 47, communication, mpi_inv_mod, int32_t(mpi *, const mpi *, const mpi *))
DYNALIB_FN(BASE_IDX2 + 48, communication, mpi_is_prime, int32_t(mpi *, int32_t (*f_rng)(void *), void *))
DYNALIB_FN(BASE_IDX2 + 49, communication, mpi_gen_prime, int32_t(mpi *, int32_t, int32_t, int32_t (*f_rng)(void *), void *))
DYNALIB_FN(BASE_IDX2 + 50, communication, tls_set_callbacks, int(TlsCallbacks))
DYNALIB_FN(BASE_IDX2 + 51, communication, rsa_init, void(rsa_context*,int32_t, int32_t, int32_t (*f_rng)( void * ), void *))
DYNALIB_FN(BASE_IDX2 + 52, communication, rsa_gen_key, int32_t(rsa_context*, int32_t, int32_t))
DYNALIB_FN(BASE_IDX2 + 53, communication, rsa_public, int32_t(const rsa_context*, const unsigned char*, unsigned char*))
DYNALIB_FN(BASE_IDX2 + 54, communication, rsa_private, int32_t(const rsa_context*, const unsigned char*, unsigned char*))
DYNALIB_FN(BASE_IDX2 + 55, communication, rsa_pkcs1_encrypt, int32_t(const rsa_context*, int32_t, int32_t, const unsigned char*, unsigned char*))
DYNALIB_FN(BASE_IDX2 + 56, communication, rsa_pkcs1_decrypt, int32_t(const rsa_context*, int32_t, int32_t*, const unsigned char*, unsigned char*, int32_t))
DYNALIB_FN(BASE_IDX2 + 57, communication, rsa_pkcs1_sign, int32_t(const rsa_context*, int32_t, int32_t, int32_t, const unsigned char*, unsigned char*))
DYNALIB_FN(BASE_IDX2 + 58, communication, rsa_pkcs1_verify, int32_t(const rsa_context*, rsa_mode_t, rsa_hash_id_t, int32_t, const unsigned char*, const unsigned char*))
DYNALIB_FN(BASE_IDX2 + 59, communication, rsa_free, void(rsa_context*))
#define BASE_IDX3 (BASE_IDX2 + 60)
#else
#define BASE_IDX3 (BASE_IDX2)
#endif

DYNALIB_END(communication)

#undef BASE_IDX
#undef BASE_IDX2
#undef BASE_IDX3

#ifdef	__cplusplus
}
#endif
