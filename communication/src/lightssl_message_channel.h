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

#if HAL_PLATFORM_CLOUD_TCP && PARTICLE_PROTOCOL

#include "service_debug.h"
#include "handshake.h"
#include "device_keys.h"
#include "message_channel.h"
#include "buffer_message_channel.h"
#include "mbedtls/aes.h"

namespace particle
{
namespace protocol
{

/**
 * This implements the lightweight and RSA encrypted handshake, AES session encryption over a TCP Stream.
 *
 * The buffer provided to the message starts at offset 2 to allow a 2-byte length to be added.
 * The buffer length extends to the maximum capacity minus 16 so there is room for PKCS#1v5 padding.
 */
class LightSSLMessageChannel: public BufferMessageChannel<
PROTOCOL_BUFFER_SIZE, 2, 16>
{
public:

	struct Callbacks
	{
		system_tick_t (*millis)();
		void (*handle_seed)(const uint8_t* seed, size_t length);
		int (*send)(const unsigned char *buf, uint32_t buflen, void* handle);
		int (*receive)(unsigned char *buf, uint32_t buflen, void* handle);
	};

private:
	unsigned char server_public_key[MAX_SERVER_PUBLIC_KEY_LENGTH];
	unsigned char core_private_key[MAX_DEVICE_PRIVATE_KEY_LENGTH];
	uint8_t device_id[12];

	unsigned char key[16];
	unsigned char iv_send[16];
	unsigned char iv_receive[16];
	unsigned char salt[8];
	mbedtls_aes_context aes;

	Callbacks callbacks;
	message_id_t* counter;

public:

	LightSSLMessageChannel()
	{
		mbedtls_aes_init(&aes);
	}

	~LightSSLMessageChannel()
	{
		mbedtls_aes_free(&aes);
	}

	virtual bool is_unreliable() override;


	void init(const uint8_t* core_private, const uint8_t* server_public,
			const uint8_t* device_id, Callbacks& callbacks, message_id_t* counter);

	virtual ProtocolError establish(uint32_t& flags, uint32_t app_crc) override
	{
		return handshake();
	}

	/**
	 * Retrieve first the 2 byte length from the stream, which determines
	 */
	ProtocolError receive(Message& message) override;

	/**
	 * Sends the given message. The message length is prepended to the message
	 * and the message padded with PKCS#1 padding before being sent using
	 * the send callback.
	 */
	ProtocolError send(Message& message) override;

	ProtocolError notify_established() override { return NO_ERROR; }

	virtual ProtocolError command(Command cmd, void* arg=nullptr) override {
		// not used presently
		return NO_ERROR;
	}

protected:

	ProtocolError set_key(const unsigned char *signed_encrypted_credentials);

	size_t wrap(unsigned char *buf, size_t msglen);
	void encrypt(unsigned char *buf, int length);

	ProtocolError handshake();

	// Returns bytes sent or -1 on error
	int blocking_send(const unsigned char *buf, int length);

	// Returns bytes received or -1 on error
	int blocking_receive(unsigned char *buf, int length);

};

}
}

#endif // HAL_PLATFORM_CLOUD_TCP && PARTICLE_PROTOCOL
