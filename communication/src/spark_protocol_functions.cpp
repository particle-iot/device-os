/**
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

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

#include "protocol_defs.h"
#include "protocol_selector.h"
#include "spark_protocol_functions.h"
#include "handshake.h"
#include "debug.h"
#include <stdlib.h>

using particle::CompletionHandler;

#ifdef USE_MBEDTLS
#include "mbedtls/rsa.h"
#include "mbedtls_util.h"
#include "mbedtls_compat.h"
#else
# if PLATFORM_ID == 6 || PLATFORM_ID == 8
#  include "wiced_security.h"
#  include "crypto_open/bignum.h"
# else
#  include "tropicssl/rsa.h"
#  include "tropicssl/sha1.h"
# endif
#endif

/**
 * Handle the cryptographically secure random seed from the cloud by using
 * it to seed the stdlib PRNG.
 * @param seed  A random value from a cryptographically secure random number generator.
 */
void default_random_seed_from_cloud(unsigned int seed)
{
    srand(seed);
}

int spark_protocol_to_system_error (int error_) {
    return toSystemError(static_cast<particle::protocol::ProtocolError>(error_));
}

#if !PARTICLE_PROTOCOL || HAL_PLATFORM_CLOUD_TCP
int decrypt_rsa(const uint8_t* ciphertext, const uint8_t* private_key, uint8_t* plaintext, int32_t plaintext_len)
{
    rsa_context rsa;
    init_rsa_context_with_private_key(&rsa, private_key);
#ifdef USE_MBEDTLS
    size_t size = 0; // mbedTLS wants size_t*
    int err = mbedtls_rsa_pkcs1_decrypt(&rsa, mbedtls_default_rng, nullptr, MBEDTLS_RSA_PRIVATE, &size, ciphertext, plaintext, plaintext_len);
    plaintext_len = size;
#else
# if PLATFORM_ID == 6 || PLATFORM_ID == 8
    int err = rsa_pkcs1_decrypt(&rsa, RSA_PRIVATE, &plaintext_len, ciphertext, plaintext, plaintext_len);
# else
    int err = rsa_pkcs1_decrypt(&rsa, RSA_PRIVATE, (int*)&plaintext_len, ciphertext, plaintext, (int)plaintext_len);
# endif
#endif // USE_MBEDTLS
    rsa_free(&rsa);
    return err ? -abs(err) : plaintext_len;
}
#endif // !PARTICLE_PROTOCOL || HAL_PLATFORM_CLOUD_TCP

#if PARTICLE_PROTOCOL
#include "hal_platform.h"

#if HAL_PLATFORM_CLOUD_TCP
#include "lightssl_protocol.h"
#endif

#if HAL_PLATFORM_CLOUD_UDP
#include "dtls_protocol.h"
#endif

void spark_protocol_communications_handlers(ProtocolFacade* protocol, CommunicationsHandlers* handlers)
{
    ASSERT_ON_SYSTEM_OR_MAIN_THREAD();
    protocol->set_handlers(*handlers);
}

void spark_protocol_init(ProtocolFacade* protocol, const char *id,
          const SparkKeys &keys,
          const SparkCallbacks &callbacks,
          const SparkDescriptor &descriptor, void* reserved)
{
    ASSERT_ON_SYSTEM_OR_MAIN_THREAD();
    (void)reserved;
    protocol->init(id, keys, callbacks, descriptor);
}

int spark_protocol_handshake(ProtocolFacade* protocol, void*) {
    ASSERT_ON_SYSTEM_THREAD();
    return protocol->begin();
}

bool spark_protocol_event_loop(ProtocolFacade* protocol, void*) {
    ASSERT_ON_SYSTEM_THREAD();
    return protocol->event_loop();
}

bool spark_protocol_is_initialized(ProtocolFacade* protocol) {
    ASSERT_ON_SYSTEM_OR_MAIN_THREAD();
    return protocol->is_initialized();
}

int spark_protocol_presence_announcement(ProtocolFacade* protocol, uint8_t *buf, const uint8_t *id, void*) {
    ASSERT_ON_SYSTEM_THREAD();
    return protocol->presence_announcement(buf, id);
}

int spark_protocol_post_description(ProtocolFacade* protocol, int desc_flags, void* reserved) {
    ASSERT_ON_SYSTEM_THREAD();
    (void)reserved;
    return protocol->post_description(desc_flags);
}

bool spark_protocol_send_event(ProtocolFacade* protocol, const char *event_name, const char *data,
                int ttl, uint32_t flags, void* reserved) {
    ASSERT_ON_SYSTEM_THREAD();
	CompletionHandler handler;
	if (reserved) {
		auto r = static_cast<const spark_protocol_send_event_data*>(reserved);
		handler = CompletionHandler(r->handler_callback, r->handler_data);
	}
	EventType::Enum event_type = EventType::extract_event_type(flags);
	return protocol->send_event(event_name, data, ttl, event_type, flags, std::move(handler));
}

bool spark_protocol_send_subscription_device(ProtocolFacade* protocol, const char *event_name, const char *device_id, void*) {
    ASSERT_ON_SYSTEM_THREAD();
    return protocol->send_subscription(event_name, device_id);
}

bool spark_protocol_send_subscription_scope(ProtocolFacade* protocol, const char *event_name, SubscriptionScope::Enum scope, void*) {
    ASSERT_ON_SYSTEM_THREAD();
    return protocol->send_subscription(event_name, scope);
}

bool spark_protocol_add_event_handler(ProtocolFacade* protocol, const char *event_name,
    EventHandler handler, SubscriptionScope::Enum scope, const char* device_id, void* handler_data) {
    ASSERT_ON_SYSTEM_OR_MAIN_THREAD();
    return protocol->add_event_handler(event_name, handler, handler_data, scope, device_id);
}

bool spark_protocol_send_time_request(ProtocolFacade* protocol, void* reserved) {
    ASSERT_ON_SYSTEM_THREAD();
    (void)reserved;
    return protocol->send_time_request();
}

void spark_protocol_send_subscriptions(ProtocolFacade* protocol, void* reserved) {
    ASSERT_ON_SYSTEM_THREAD();
    (void)reserved;
    protocol->send_subscriptions();
}

void spark_protocol_remove_event_handlers(ProtocolFacade* protocol, const char* event_name, void* reserved) {
    ASSERT_ON_SYSTEM_THREAD();
    (void)reserved;
    protocol->remove_event_handlers(event_name);
}

void spark_protocol_set_product_id(ProtocolFacade* protocol, product_id_t product_id, unsigned, void*) {
    ASSERT_ON_SYSTEM_OR_MAIN_THREAD();
    protocol->set_product_id(product_id);
}

void spark_protocol_set_product_firmware_version(ProtocolFacade* protocol, product_firmware_version_t product_firmware_version, unsigned, void*) {
    ASSERT_ON_SYSTEM_OR_MAIN_THREAD();
    protocol->set_product_firmware_version(product_firmware_version);
}

void spark_protocol_get_product_details(ProtocolFacade* protocol, product_details_t* details, void* reserved) {
    ASSERT_ON_SYSTEM_OR_MAIN_THREAD();
    (void)reserved;
    protocol->get_product_details(*details);
}

int spark_protocol_set_connection_property(ProtocolFacade* protocol, unsigned property_id,
                                           unsigned data, particle::protocol::connection_properties_t* conn_prop, void* reserved)
{
    ASSERT_ON_SYSTEM_THREAD();
    if (property_id == particle::protocol::Connection::PING)
    {
        protocol->set_keepalive(data, conn_prop->keepalive_source);
    } else if (property_id == particle::protocol::Connection::FAST_OTA)
    {
        protocol->set_fast_ota(data);
    }
    return 0;
}
int spark_protocol_command(ProtocolFacade* protocol, ProtocolCommands::Enum cmd, uint32_t data, void* reserved)
{
    ASSERT_ON_SYSTEM_THREAD();
    return protocol->command(cmd, data);
}

bool spark_protocol_time_request_pending(ProtocolFacade* protocol, void* reserved)
{
    (void)reserved;
    return protocol->time_request_pending();
}
system_tick_t spark_protocol_time_last_synced(ProtocolFacade* protocol, time_t* tm, void* reserved)
{
    (void)reserved;
    return protocol->time_last_synced(tm);
}

int spark_protocol_get_describe_data(ProtocolFacade* protocol, spark_protocol_describe_data* data, void* reserved)
{
	return protocol->get_describe_data(data, reserved);
}

#if HAL_PLATFORM_MESH
int spark_protocol_mesh_command(ProtocolFacade* protocol, MeshCommand::Enum cmd, uint32_t data, void* extraData, completion_handler_data* completion, void* reserved) {
	(void)reserved;
	return protocol->mesh_command(cmd, data, extraData, completion);
}
#endif // HAL_PLATFORM_MESH

#else // !PARTICLE_PROTOCOL

#include "core_protocol.h"

void spark_protocol_communications_handlers(CoreProtocol* protocol, CommunicationsHandlers* handlers)
{
    ASSERT_ON_SYSTEM_THREAD();
    protocol->set_handlers(*handlers);
}

void spark_protocol_init(CoreProtocol* protocol, const char *id,
          const SparkKeys &keys,
          const SparkCallbacks &callbacks,
          const SparkDescriptor &descriptor, void* reserved)
{
    ASSERT_ON_SYSTEM_THREAD();
    (void)reserved;
    protocol->init(id, keys, callbacks, descriptor);
}

int spark_protocol_handshake(CoreProtocol* protocol, void*) {
    ASSERT_ON_SYSTEM_THREAD();
    protocol->reset_updating();
    return protocol->handshake();
}

bool spark_protocol_event_loop(CoreProtocol* protocol, void*) {
    ASSERT_ON_SYSTEM_THREAD();
    CoAPMessageType::Enum msgtype;
    return protocol->event_loop(msgtype);
}

bool spark_protocol_is_initialized(CoreProtocol* protocol) {
    ASSERT_ON_SYSTEM_THREAD();
    return protocol->is_initialized();
}

int spark_protocol_presence_announcement(CoreProtocol* protocol, unsigned char *buf, const unsigned char *id, void*) {
    ASSERT_ON_SYSTEM_THREAD();
    return protocol->presence_announcement(buf, id);
}

int spark_protocol_post_description(CoreProtocol* protocol, int desc_flags, void* reserved) {
    ASSERT_ON_SYSTEM_THREAD();
    (void)reserved;
    return protocol->post_description(desc_flags);
}

bool spark_protocol_send_event(CoreProtocol* protocol, const char *event_name, const char *data,
                int ttl, uint32_t flags, void* reserved) {
    ASSERT_ON_SYSTEM_THREAD();
	CompletionHandler handler;
	if (reserved) {
		auto r = static_cast<const spark_protocol_send_event_data*>(reserved);
		handler = CompletionHandler(r->handler_callback, r->handler_data);
	}
	EventType::Enum event_type = EventType::extract_event_type(flags);
	return protocol->send_event(event_name, data, ttl, event_type, flags, std::move(handler));
}

bool spark_protocol_send_subscription_device(CoreProtocol* protocol, const char *event_name, const char *device_id, void*) {
    ASSERT_ON_SYSTEM_THREAD();
    return protocol->send_subscription(event_name, device_id);
}

bool spark_protocol_send_subscription_scope(CoreProtocol* protocol, const char *event_name, SubscriptionScope::Enum scope, void*) {
    ASSERT_ON_SYSTEM_THREAD();
    return protocol->send_subscription(event_name, scope);
}

bool spark_protocol_add_event_handler(CoreProtocol* protocol, const char *event_name,
    EventHandler handler, SubscriptionScope::Enum scope, const char* device_id, void* handler_data) {
    ASSERT_ON_SYSTEM_OR_MAIN_THREAD();
    return protocol->add_event_handler(event_name, handler, handler_data, scope, device_id);
}

bool spark_protocol_send_time_request(CoreProtocol* protocol, void* reserved) {
    ASSERT_ON_SYSTEM_THREAD();
    (void)reserved;
    return protocol->send_time_request();
}

void spark_protocol_send_subscriptions(CoreProtocol* protocol, void* reserved) {
    ASSERT_ON_SYSTEM_THREAD();
    (void)reserved;
    protocol->send_subscriptions();
}

void spark_protocol_remove_event_handlers(CoreProtocol* protocol, const char* event_name, void* reserved) {
    ASSERT_ON_SYSTEM_THREAD();
    (void)reserved;
    protocol->remove_event_handlers(event_name);
}

void spark_protocol_set_product_id(CoreProtocol* protocol, product_id_t product_id, unsigned, void*) {
    ASSERT_ON_SYSTEM_THREAD();
    protocol->set_product_id(product_id);
}

void spark_protocol_set_product_firmware_version(CoreProtocol* protocol, product_firmware_version_t product_firmware_version, unsigned, void*) {
    ASSERT_ON_SYSTEM_THREAD();
    protocol->set_product_firmware_version(product_firmware_version);
}

void spark_protocol_get_product_details(CoreProtocol* protocol, product_details_t* details, void* reserved) {
    ASSERT_ON_SYSTEM_THREAD();
    (void)reserved;
    protocol->get_product_details(*details);
}

int spark_protocol_set_connection_property(CoreProtocol* protocol, unsigned property_id,
                                           unsigned data, particle::protocol::connection_properties_t* conn_prop, void* reserved)
{
    return 0;
}

int spark_protocol_command(CoreProtocol* protocol, ProtocolCommands::Enum cmd, uint32_t data, void* reserved)
{
    (void)reserved;
	return protocol->command(cmd, data);
}

bool spark_protocol_time_request_pending(CoreProtocol* protocol, void* reserved)
{
    (void)reserved;
    return protocol->time_request_pending();
}
system_tick_t spark_protocol_time_last_synced(CoreProtocol* protocol, time_t* tm, void* reserved)
{
    (void)reserved;
    return protocol->time_last_synced(tm);
}

int spark_protocol_get_describe_data(ProtocolFacade* protocol, spark_protocol_describe_data* data, void* reserved) {
	return -1;
}

#endif
