#include "spark_protocol.h"
#include "handshake.h"
#include <string.h>

int SparkProtocol::init(const unsigned char *private_key,
                        const unsigned char *pubkey,
                        const unsigned char *encrypted_credentials,
                        const unsigned char *signature)
{
  unsigned char credentials[40];
  unsigned char hmac[20];

  if (0 != decipher_aes_credentials(private_key, encrypted_credentials, credentials))
    return 1;

  calculate_ciphertext_hmac(encrypted_credentials, credentials, hmac);

  if (0 == verify_signature(signature, pubkey, hmac))
  {
    memcpy(key,  credentials,      16);
    memcpy(iv,   credentials + 16, 16);
    memcpy(salt, credentials + 32,  8);
    _message_id = *(credentials + 32) << 8 | *(credentials + 33);
    return 0;
  }
  else return 2;
}

void SparkProtocol::hello(unsigned char *buf)
{
  unsigned short message_id = next_message_id();
  buf[0] = 0x50; // non-confirmable, no token
  buf[1] = 0x02; // POST
  buf[2] = message_id >> 8;
  buf[3] = message_id & 0xff;
  buf[4] = 0xb1; // Uri-Path option of length 1
  buf[5] = 'h';
  for (int i = 6; i < 16; ++i)
    buf[i] = 0x0a; // PKCS #7 padding
  
  aes_setkey_enc(&aes, key, 128);
  aes_crypt_cbc(&aes, AES_ENCRYPT, 16, iv, buf, buf);
}

void SparkProtocol::hello(unsigned char *buf, unsigned char token)
{
  unsigned short message_id = next_message_id();
  buf[0] = 0x51; // non-confirmable, one-byte token
  buf[1] = 0x44; // response code 2.04 CHANGED
  buf[2] = message_id >> 8;
  buf[3] = message_id & 0xff;
  buf[4] = token;
  for (int i = 5; i < 16; ++i)
    buf[i] = 0x0b; // PKC #7 padding

  aes_setkey_enc(&aes, key, 128);
  aes_crypt_cbc(&aes, AES_ENCRYPT, 16, iv, buf, buf);
}

CoAPMessageType::Enum
  SparkProtocol::received_message(unsigned char *buf)
{
  aes_setkey_dec(&aes, key, 128);
  aes_crypt_cbc(&aes, AES_DECRYPT, 32, iv, buf, buf);

  switch (CoAP::code(buf))
  {
    case CoAPCode::GET:
      return CoAPMessageType::VARIABLE_REQUEST;
    case CoAPCode::POST:
      switch (buf[6])
      {
        case 'f': return CoAPMessageType::FUNCTION_CALL;
        case 'u': return CoAPMessageType::UPDATE_BEGIN;
        case 'c': return CoAPMessageType::CHUNK;
        default: return CoAPMessageType::ERROR;
      }
    case CoAPCode::PUT:
      switch (buf[6])
      {
        case 'k': return CoAPMessageType::KEY_CHANGE;
        case 'u': return CoAPMessageType::UPDATE_DONE;
        default: return CoAPMessageType::ERROR;
      }
    default:
      return CoAPMessageType::ERROR;
  }
}

unsigned short SparkProtocol::next_message_id()
{
  return ++_message_id;
}
