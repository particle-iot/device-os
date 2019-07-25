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

#if PARTICLE_PROTOCOL && HAL_PLATFORM_CLOUD_UDP
DYNALIB_FN(BASE_IDX + 1, communication, gen_ec_key, int(uint8_t*, size_t, int(*)(void*, uint8_t*, size_t), void*))
DYNALIB_FN(BASE_IDX + 2, communication, extract_public_ec_key, int(uint8_t*, size_t, const uint8_t*))
#define BASE_IDX2 (BASE_IDX + 3)
#else
#define BASE_IDX2 (BASE_IDX + 1)
#endif

DYNALIB_FN(BASE_IDX2 + 0, communication, spark_protocol_set_connection_property, int(ProtocolFacade*, unsigned, unsigned, particle::protocol::connection_properties_t*, void*))
DYNALIB_FN(BASE_IDX2 + 1, communication, spark_protocol_command, int(ProtocolFacade* protocol, ProtocolCommands::Enum cmd, uint32_t data, void* reserved))
DYNALIB_FN(BASE_IDX2 + 2, communication, spark_protocol_time_request_pending, bool(ProtocolFacade*, void*))
DYNALIB_FN(BASE_IDX2 + 3, communication, spark_protocol_time_last_synced, system_tick_t(ProtocolFacade*, time_t*, void*))

#if HAL_PLATFORM_MESH
DYNALIB_FN(BASE_IDX2 + 4, communication, spark_protocol_mesh_command, int(ProtocolFacade* protocol, MeshCommand::Enum cmd, uint32_t data, void* extraData, completion_handler_data* completion, void* reserved))
#define BASE_IDX3 (BASE_IDX2 + 5)
#else // !HAL_PLATFORM_MESH
#define BASE_IDX3 (BASE_IDX2 + 4)
#endif // HAL_PLATFORM_MESH

DYNALIB_FN(BASE_IDX3 + 0, communication, spark_protocol_get_describe_data, int(ProtocolFacade*, spark_protocol_describe_data*, void*))
DYNALIB_FN(BASE_IDX3 + 1, communication, spark_protocol_post_description, int(ProtocolFacade*, int, void*))
DYNALIB_FN(BASE_IDX3 + 2, communication, spark_protocol_to_system_error, int(int))

DYNALIB_END(communication)

#undef BASE_IDX
#undef BASE_IDX2
#undef BASE_IDX3

#ifdef	__cplusplus
}
#endif
