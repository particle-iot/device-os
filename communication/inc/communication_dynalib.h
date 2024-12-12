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
#include "time_compat.h"

#ifdef DYNALIB_EXPORT
#include "coap_api.h"
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
DYNALIB_FN(11, communication, spark_protocol_send_subscription_device_deprecated, bool(ProtocolFacade*, const char*, const char*, void*))
DYNALIB_FN(12, communication, spark_protocol_send_subscription, bool(ProtocolFacade*, const char*, uint8_t, void*))
DYNALIB_FN(13, communication, spark_protocol_add_event_handler, bool(ProtocolFacade*, const char*, EventHandler, uint8_t, const char*, void*))
DYNALIB_FN(14, communication, spark_protocol_send_time_request, bool(ProtocolFacade*, void*))
DYNALIB_FN(15, communication, spark_protocol_send_subscriptions, void(ProtocolFacade*, void*))

#if HAL_PLATFORM_CLOUD_TCP
DYNALIB_FN(16, communication, decrypt_rsa, int(const uint8_t*, const uint8_t*, uint8_t*, int32_t))
DYNALIB_FN(17, communication, gen_rsa_key, int(uint8_t*, size_t, int32_t(*)(void*), void*))
DYNALIB_FN(18, communication, extract_public_rsa_key, void(uint8_t*, const uint8_t*))
#define BASE_IDX 19 // Base index for all subsequent functions
#else
#define BASE_IDX 16
#endif

DYNALIB_FN(BASE_IDX + 0, communication, spark_protocol_remove_event_handlers, void(ProtocolFacade*, const char*, void*))

#if HAL_PLATFORM_CLOUD_UDP
DYNALIB_FN(BASE_IDX + 1, communication, gen_ec_key, int(uint8_t*, size_t, int(*)(void*, uint8_t*, size_t), void*))
DYNALIB_FN(BASE_IDX + 2, communication, extract_public_ec_key, int(uint8_t*, size_t, const uint8_t*))
#define BASE_IDX2 (BASE_IDX + 3)
#else
#define BASE_IDX2 (BASE_IDX + 1)
#endif

DYNALIB_FN(BASE_IDX2 + 0, communication, spark_protocol_set_connection_property, int(ProtocolFacade*, unsigned, int, const void*, void*))
DYNALIB_FN(BASE_IDX2 + 1, communication, spark_protocol_command, int(ProtocolFacade*, ProtocolCommands::Enum, uint32_t, const void*))
DYNALIB_FN(BASE_IDX2 + 2, communication, spark_protocol_time_request_pending, bool(ProtocolFacade*, void*))
DYNALIB_FN(BASE_IDX2 + 3, communication, spark_protocol_time_last_synced, system_tick_t(ProtocolFacade*, time32_t*, time_t*))
DYNALIB_FN(BASE_IDX2 + 4, communication, spark_protocol_get_describe_data, int(ProtocolFacade*, spark_protocol_describe_data*, void*))
DYNALIB_FN(BASE_IDX2 + 5, communication, spark_protocol_post_description, int(ProtocolFacade*, int, void*))
DYNALIB_FN(BASE_IDX2 + 6, communication, spark_protocol_to_system_error, int(int))
DYNALIB_FN(BASE_IDX2 + 7, communication, spark_protocol_get_status, int(ProtocolFacade*, protocol_status*, void*))
DYNALIB_FN(BASE_IDX2 + 8, communication, spark_protocol_get_connection_property, int(ProtocolFacade*, unsigned, void*, size_t*, void*))

DYNALIB_FN(BASE_IDX2 + 9, communication, coap_add_connection_handler, int(coap_connection_callback, void*, void*))
DYNALIB_FN(BASE_IDX2 + 10, communication, coap_remove_connection_handler, void(coap_connection_callback, void*))
DYNALIB_FN(BASE_IDX2 + 11, communication, coap_add_request_handler, int(const char*, int, int, coap_request_callback, void*, void*))
DYNALIB_FN(BASE_IDX2 + 12, communication, coap_remove_request_handler, void(const char*, int, void*))
DYNALIB_FN(BASE_IDX2 + 13, communication, coap_begin_request, int(coap_message**, const char*, int, int, int, void*))
DYNALIB_FN(BASE_IDX2 + 14, communication, coap_end_request, int(coap_message*, coap_response_callback, coap_ack_callback, coap_error_callback, void*, void*))
DYNALIB_FN(BASE_IDX2 + 15, communication, coap_begin_response, int(coap_message**, int, int, int, void*))
DYNALIB_FN(BASE_IDX2 + 16, communication, coap_end_response, int(coap_message*, coap_ack_callback, coap_error_callback, void*, void*))
DYNALIB_FN(BASE_IDX2 + 17, communication, coap_destroy_message, void(coap_message*, void*))
DYNALIB_FN(BASE_IDX2 + 18, communication, coap_cancel_request, int(int, void*))
DYNALIB_FN(BASE_IDX2 + 19, communication, coap_write_block, int(coap_message*, const char*, size_t*, coap_block_callback, coap_error_callback, void*, void*))
DYNALIB_FN(BASE_IDX2 + 20, communication, coap_read_block, int(coap_message*, char*, size_t*, coap_block_callback, coap_error_callback, void*, void*))
DYNALIB_FN(BASE_IDX2 + 21, communication, coap_peek_block, int(coap_message*, char*, size_t, void*))
DYNALIB_FN(BASE_IDX2 + 22, communication, coap_create_payload, int(coap_payload**, size_t, void*))
DYNALIB_FN(BASE_IDX2 + 23, communication, coap_destroy_payload, void(coap_payload*, void*))
DYNALIB_FN(BASE_IDX2 + 24, communication, coap_write_payload, int(coap_payload*, const char*, size_t, size_t, void*))
DYNALIB_FN(BASE_IDX2 + 25, communication, coap_read_payload, int(coap_payload*, char*, size_t, size_t, void*))
DYNALIB_FN(BASE_IDX2 + 26, communication, coap_set_payload_size, int(coap_payload*, size_t, void*))
DYNALIB_FN(BASE_IDX2 + 27, communication, coap_get_payload_size, int(coap_payload*, void*))
DYNALIB_FN(BASE_IDX2 + 28, communication, coap_set_payload, int(coap_message*, coap_payload*, void*))
DYNALIB_FN(BASE_IDX2 + 29, communication, coap_get_payload, int(coap_message*, coap_payload**, void*))
DYNALIB_FN(BASE_IDX2 + 30, communication, coap_get_option, int(coap_message*, coap_option**, int, void*))
DYNALIB_FN(BASE_IDX2 + 31, communication, coap_get_next_option, int(coap_message*, coap_option**, int*, void*))
DYNALIB_FN(BASE_IDX2 + 32, communication, coap_get_uint_option_value, int(coap_option*, unsigned*, void*))
DYNALIB_FN(BASE_IDX2 + 33, communication, coap_get_string_option_value, int(coap_option*, char*, size_t, void*))
DYNALIB_FN(BASE_IDX2 + 34, communication, coap_get_opaque_option_value, int(coap_option*, char*, size_t, void*))
DYNALIB_FN(BASE_IDX2 + 35, communication, coap_add_empty_option, int(coap_message*, int, void*))
DYNALIB_FN(BASE_IDX2 + 36, communication, coap_add_uint_option, int(coap_message*, int, unsigned, void*))
DYNALIB_FN(BASE_IDX2 + 37, communication, coap_add_string_option, int(coap_message*, int, const char*, void*))
DYNALIB_FN(BASE_IDX2 + 38, communication, coap_add_opaque_option, int(coap_message*, int, const char*, size_t, void*))

DYNALIB_END(communication)

#undef BASE_IDX
#undef BASE_IDX2
#undef BASE_IDX3

#ifdef	__cplusplus
}
#endif
