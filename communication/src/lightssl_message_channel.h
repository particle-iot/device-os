#include "service_debug.h"
#include "handshake.h"
#include "device_keys.h"
#include "message_channel.h"
#include "buffer_message_channel.h"

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
class LightStreamSecureChannel: public BufferMessageChannel<
		PROTOCOL_BUFFER_SIZE, 2, 16>
{
public:

	struct Callbacks
	{
		system_tick_t (*millis)();
		void (*handle_seed)(const uint8_t* seed, size_t length);
		int (*send)(const unsigned char *buf, uint32_t buflen);
		int (*receive)(unsigned char *buf, uint32_t buflen);
	};

private:
	const uint8_t* core_private_key;
	const uint8_t* server_public_key;
	uint8_t device_id[12];

	unsigned char key[16];
	unsigned char iv_send[16];
	unsigned char iv_receive[16];
	unsigned char salt[8];
	aes_context aes;

	Callbacks callbacks;

public:

	LightStreamSecureChannel(const uint8_t* core_private,
			const uint8_t* server_public, const uint8_t* device_id) :
			core_private_key(core_private), server_public_key(server_public)
	{
		memcpy(this->device_id, device_id, sizeof(this->device_id));
	}

	void init(Callbacks& callbacks)
	{
		this->callbacks = callbacks;
	}

	virtual ProtocolError establish() override
	{
		return handshake();
	}

	ProtocolError receive(Message& message) override
	{
		return NO_ERROR;
	}

	ProtocolError send(Message& message) override
	{
		return NO_ERROR;
	}

protected:

	ProtocolError set_key(const unsigned char *signed_encrypted_credentials)
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

	size_t wrap(unsigned char *buf, size_t msglen)
	{
		size_t buflen = (msglen & ~15) + 16;
		char pad = buflen - msglen;
		memset(buf + 2 + msglen, pad, pad); // PKCS #7 padding

		encrypt(buf + 2, buflen);

		buf[0] = (buflen >> 8) & 0xff;
		buf[1] = buflen & 0xff;

		return buflen + 2;
	}

	void encrypt(unsigned char *buf, int length)
	{
		aes_setkey_enc(&aes, key, 128);
		aes_crypt_cbc(&aes, AES_ENCRYPT, length, iv_send, buf, buf);
		memcpy(iv_send, buf, 16);
	}

	ProtocolError handshake()
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
	int blocking_send(const unsigned char *buf, int length)
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
	int blocking_receive(unsigned char *buf, int length)
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

};

}
}
