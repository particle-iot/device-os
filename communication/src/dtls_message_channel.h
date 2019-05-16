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
#pragma once

#include "protocol_selector.h"

#if HAL_PLATFORM_CLOUD_UDP && PARTICLE_PROTOCOL

#include "service_debug.h"
#include "device_keys.h"
#include "message_channel.h"
#include "buffer_message_channel.h"

#include "mbedtls/ssl.h"
#include "mbedtls/ssl_internal.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/pk.h"
#include "mbedtls/timing.h"
#include "mbedtls/debug.h"

namespace particle
{
namespace protocol
{

/**
 * Please centralize this somewhere else!
 */
const size_t DEVICE_ID_LEN = 12;

/**
 * This implements the lightweight and RSA encrypted handshake, AES session encryption over a TCP Stream.
 *
 * The buffer provided to the message starts at offset 2 to allow a 2-byte length to be added.
 * The buffer length extends to the maximum capacity minus 16 so there is room for PKCS#1v5 padding.
 */
class DTLSMessageChannel: public BufferMessageChannel<PROTOCOL_BUFFER_SIZE>
{
public:

	struct Callbacks
	{
		/**
		 * An opaque handle for the send/receive context.
		 */
		void* tx_context;

		system_tick_t (*millis)();
		void (*handle_seed)(const uint8_t* seed, size_t length);
		int (*send)(const unsigned char *buf, uint32_t buflen, void* handle);
		int (*receive)(unsigned char *buf, uint32_t buflen, void* handle);

		// persistence
		/**
		 * Saves the given buffer.
		 * Returns 0 on success.
		 */
		int (*save)(const void* data, size_t length, uint8_t type, void* reserved);
		/**
		 * Restore to the given buffer. Returns the number of bytes restored.
		 */
		int (*restore)(void* data, size_t max_length, uint8_t type, void* reserved);

		uint32_t (*calculate_crc)(const uint8_t* data, uint32_t length);
	};

private:

	friend int dtls_rng(void* handle, uint8_t* data, size_t len);

	mbedtls_ssl_context ssl_context;
	mbedtls_ssl_config conf;
	mbedtls_x509_crt clicert;
	mbedtls_pk_context pkey;
	mbedtls_timing_delay_context timer;
	Callbacks callbacks;
	uint8_t* server_public;
	uint16_t server_public_len;
	uint32_t keys_checksum;

	/**
	 * The next message ID for new messages over this channel.
	 */
	message_id_t* coap_state;
	bool move_session;
	const uint8_t* device_id;

    void init();
    void dispose();

    /**
     * C function to call the send/recv methods on a DTLSMessageChannel instance.
     */
    static int send_(void* ctx, const uint8_t* data, size_t len);
    static int recv_(void* ctx, uint8_t* data, size_t len);

    int send(const uint8_t* data, size_t len);
    int recv(uint8_t* data, size_t len);

	ProtocolError setup_context();

	void cancel_move_session();

	void reset_session();

 public:
	DTLSMessageChannel() : coap_state(nullptr), move_session(false) {}

	ProtocolError init(const uint8_t* core_private, size_t core_private_len,
		const uint8_t* core_public, size_t core_public_len,
		const uint8_t* server_public, size_t server_public_len,
		const uint8_t* device_id, Callbacks& callbacks,
		message_id_t* coap_state);

	virtual bool is_unreliable() override;

	virtual ProtocolError establish(uint32_t& flags, uint32_t app_crc) override;

	/**
	 * Retrieve first the 2 byte length from the stream, which determines
	 */
	virtual ProtocolError receive(Message& message) override;

	/**
	 * Sends the given message. The message length is prepended to the message
	 * and the message padded with PKCS#1 padding before being sent using
	 * the send callback.
	 */
	virtual ProtocolError send(Message& message) override;


	virtual ProtocolError notify_established() override;


	virtual ProtocolError command(Command cmd, void* arg=nullptr) override;

};


}}

#endif // HAL_PLATFORM_CLOUD_UDP && PARTICLE_PROTOCOL
