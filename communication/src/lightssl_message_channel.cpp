#include "service_debug.h"
#include "handshake.h"
#include "device_keys.h"
#include "message_channel.h"
#include "buffer_message_channel.h"
#include "tropicssl/rsa.h"
#include "tropicssl/aes.h"
#include "lightssl_message_channel.h"

namespace particle
{
namespace protocol
{

	void LightSSLMessageChannel::init(const uint8_t* core_private, const uint8_t* server_public,
			const uint8_t* device_id, Callbacks& callbacks)
	{
		memcpy(core_private_key, core_private, sizeof(core_private_key));
		memcpy(server_public_key, server_public, sizeof(server_public_key));
		memcpy(this->device_id, device_id, sizeof(this->device_id));
		this->callbacks = callbacks;
	}

	/**
	 * Sends the given message. The message length is prepended to the message
	 * and the message padded with PKCS#1 padding before being sent using
	 * the send callback.
	 */
	ProtocolError LightSSLMessageChannel::send(Message& message)
	{
		uint8_t* buf = message.buf()-2;
		size_t to_write = wrap(buf, message.length());
		return blocking_send(buf, to_write)<0 ? IO_ERROR : NO_ERROR;
	}

	ProtocolError LightSSLMessageChannel::receive(Message& message)
	{
		ProtocolError error = NO_ERROR;
		int bytes_received = blocking_receive(queue, 2);
		if (2 == bytes_received)
		{
			size_t packet_size = queue[0] << 8 | queue[1];
			error = create(message, packet_size);
			if (!error)
			{
				if (blocking_receive(message.buf(), packet_size) < 0)
					error = IO_ERROR;
			}
		}
		else
		{
			error = create(message, 0);
		}
		return error;
	}


	ProtocolError LightSSLMessageChannel::set_key(const unsigned char *signed_encrypted_credentials)
	{
		unsigned char credentials[40];
		unsigned char hmac[20];

		int error = decipher_aes_credentials(core_private_key,
				signed_encrypted_credentials, credentials);
		if (error)
		{
			WARN("decryption failed with code %d", error);
			return DECRYPTION_ERROR;
		}

		calculate_ciphertext_hmac(signed_encrypted_credentials, credentials,
				hmac);

		error = verify_signature(signed_encrypted_credentials + 128,
				server_public_key, hmac);
		if (error)
		{
			WARN("signature validation failed with code %d", error);
			return AUTHENTICATION_ERROR;
		}

		memcpy(key, credentials, 16);
		memcpy(iv_send, credentials + 16, 16);
		memcpy(iv_receive, credentials + 16, 16);
		memcpy(salt, credentials + 32, 8);

		if (callbacks.handle_seed)
			callbacks.handle_seed(credentials + 32, 8);

		return NO_ERROR;
	}

	size_t LightSSLMessageChannel::wrap(unsigned char *buf, size_t msglen)
	{
		size_t buflen = (msglen & ~15) + 16;
		char pad = buflen - msglen;
		memset(buf + 2 + msglen, pad, pad); // PKCS #7 padding

		encrypt(buf + 2, buflen);

		buf[0] = (buflen >> 8) & 0xff;
		buf[1] = buflen & 0xff;

		return buflen + 2;
	}

	void LightSSLMessageChannel::encrypt(unsigned char *buf, int length)
	{
		aes_setkey_enc(&aes, key, 128);
		aes_crypt_cbc(&aes, AES_ENCRYPT, length, iv_send, buf, buf);
		memcpy(iv_send, buf, 16);
	}

	ProtocolError LightSSLMessageChannel::handshake()
	{
		memcpy(queue + 40, device_id, 12);
		int err = blocking_receive(queue, 40);
		if (0 > err)
		{
			ERROR("Handshake: could not receive nonce: %d", err);
			return IO_ERROR;
		}

		parse_device_pubkey_from_privkey(queue + 52, core_private_key);

		rsa_context rsa;
		init_rsa_context_with_public_key(&rsa, server_public_key);
		const int len = 52 + MAX_DEVICE_PUBLIC_KEY_LENGTH;
		err = rsa_pkcs1_encrypt(&rsa, RSA_PUBLIC, len, queue, queue + len);
		rsa_free(&rsa);

		if (err)
		{
			ERROR("Handshake: rsa encrypt error %d", err);
			return ENCRYPTION_ERROR;
		}

		blocking_send(queue + len, 256);
		err = blocking_receive(queue, 384);
		if (0 > err)
		{
			ERROR("Handshake: Unable to receive key %d", err);
			return IO_ERROR;
		}

		ProtocolError error = set_key(queue);
		if (error)
		{
			ERROR("Handshake:  could not set key, %d", error);
			return error;
		}

		return NO_ERROR;
	}

	// Returns bytes sent or -1 on error
	int LightSSLMessageChannel::blocking_send(const unsigned char *buf, int length)
	{
		int bytes_or_error;
		int byte_count = 0;

		system_tick_t _millis = callbacks.millis();

		while (length > byte_count)
		{
			bytes_or_error = callbacks.send(buf + byte_count,
					length - byte_count);
			if (0 > bytes_or_error)
			{
				// error, disconnected
				WARN("blocking send error %d", bytes_or_error);
				return bytes_or_error;
			}
			else if (0 < bytes_or_error)
			{
				byte_count += bytes_or_error;
			}
			else
			{
				if (20000 < (callbacks.millis() - _millis))
				{
					// timed out, disconnect
					WARN("blocking send timeout");
					return -1;
				}
			}
		}
		return byte_count;
	}

	// Returns bytes received or -1 on error
	int LightSSLMessageChannel::blocking_receive(unsigned char *buf, int length)
	{
		int bytes_or_error;
		int byte_count = 0;

		system_tick_t _millis = callbacks.millis();

		while (length > byte_count)
		{
			bytes_or_error = callbacks.receive(buf + byte_count,
					length - byte_count);
			if (0 > bytes_or_error)
			{
				// error, disconnected
				WARN("receive error %d", bytes_or_error);
				return bytes_or_error;
			}
			else if (0 < bytes_or_error)
			{
				byte_count += bytes_or_error;
			}
			else
			{
				if (20000 < (callbacks.millis() - _millis))
				{
					// timed out, disconnect
					WARN("receive timeout");
					return -1;
				}
			}
		}
		return byte_count;
	}

}
}
