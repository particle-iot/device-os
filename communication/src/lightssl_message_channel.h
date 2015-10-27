
#include "service_debug.h"
#include "handshake.h"
#include "device_keys.h"
#include "message_channel.h"


namespace particle {
namespace protocol {

/**
 * This implements the lightweight and RSA encrypted handshake, AES session encryption over a TCP Stream.
 */
class LightStreamSecureChannel : public MessageChannel
{


protected:

	int set_key(const unsigned char *signed_encrypted_credentials)
	{
		unsigned char credentials[40];
		unsigned char hmac[20];

		if (0 != decipher_aes_credentials(core_private_key,
										signed_encrypted_credentials,
										credentials))
		return 1;

		calculate_ciphertext_hmac(signed_encrypted_credentials, credentials, hmac);

		if (0 == verify_signature(signed_encrypted_credentials + 128,
								server_public_key,
								hmac))
		{
			memcpy(key,        credentials,      16);
			memcpy(iv_send,    credentials + 16, 16);
			memcpy(iv_receive, credentials + 16, 16);
			memcpy(salt,       credentials + 32,  8);
			_message_id = *(credentials + 32) << 8 | *(credentials + 33);
			_token = *(credentials + 34);

			unsigned int seed;
			memcpy(&seed, credentials + 35, 4);
			if (random_seed_from_cloud)
				random_seed_from_cloud(seed);
			else
				default_random_seed_from_cloud(seed);

			return 0;
		}
		else return 2;
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

	int handshake()
	{
		memcpy(queue + 40, device_id, 12);
		int err = blocking_receive(queue, 40);
		if (0 > err) { ERROR("Handshake: could not receive nonce: %d", err);  return err; }

		parse_device_pubkey_from_privkey(queue+52, core_private_key);

		rsa_context rsa;
		init_rsa_context_with_public_key(&rsa, server_public_key);
		const int len = 52+MAX_DEVICE_PUBLIC_KEY_LENGTH;
		err = rsa_pkcs1_encrypt(&rsa, RSA_PUBLIC, len, queue, queue + len);
		rsa_free(&rsa);

		if (err) { ERROR("Handshake: rsa encrypt error %d", err); return err; }

		blocking_send(queue + len, 256);
		err = blocking_receive(queue, 384);
		if (0 > err) { ERROR("Handshake: Unable to receive key %d", err); return err; }

		err = set_key(queue);
		if (err) { ERROR("Handshake:  could not set key, %d"); return err; }

		return 0;
	}

	// Returns bytes sent or -1 on error
	int blocking_send(const unsigned char *buf, int length)
	{
	  int bytes_or_error;
	  int byte_count = 0;

	  system_tick_t _millis = callbacks.millis();

	  while (length > byte_count)
	  {
	    bytes_or_error = callbacks.send(buf + byte_count, length - byte_count);
	    if (0 > bytes_or_error)
	    {
	      // error, disconnected
	      serial_dump("blocking send error %d", bytes_or_error);
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
	        serial_dump("blocking send timeout");
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
	    bytes_or_error = callbacks.receive(buf + byte_count, length - byte_count);
	    if (0 > bytes_or_error)
	    {
	      // error, disconnected
	      serial_dump("receive error %d", bytes_or_error);
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
	          serial_dump("receive timeout");
	        return -1;
	      }
	    }
	  }
	  return byte_count;
	}

public:

	ProtocolError establish()
	{

	}

	ProtocolError create(Message& message)
	{
		return NO_ERROR;
	}

	ProtocolError response(const Message& original, Message& response, size_t minimum_size)
	{

	}

	ProtocolError receive(Message& message)
	{
		return NO_ERROR;
	}

	ProtocolError send(Message& message)
	{
		return NO_ERROR;
	}


};

}
}
