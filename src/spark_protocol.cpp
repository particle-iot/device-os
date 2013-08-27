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

void SparkProtocol::hello(unsigned char *buf)
{
  unsigned short message_id = next_message_id();

  buf[0] = 0x50; // non-confirmable, no token
  buf[1] = 0x02; // POST
  buf[2] = message_id >> 8;
  buf[3] = message_id & 0xff;
  buf[4] = 0xb1; // Uri-Path option of length 1
  buf[5] = 'h';

  memset(buf + 6, 10, 10); // PKCS #7 padding
  
  encrypt(buf, 16);
}

void SparkProtocol::hello(unsigned char *buf, unsigned char token)
{
  separate_response(buf, token, 0x44);
}

void SparkProtocol::function_return(unsigned char *buf, unsigned char token)
{
  separate_response(buf, token, 0x44);
}

void SparkProtocol::function_return(unsigned char *buf,
                                    unsigned char token,
                                    bool return_value)
{
  unsigned short message_id = next_message_id();

  buf[0] = 0x51; // non-confirmable, one-byte token
  buf[1] = 0x44; // response code 2.04 CHANGED
  buf[2] = message_id >> 8;
  buf[3] = message_id & 0xff;
  buf[4] = token;
  buf[5] = 0xff; // payload marker
  buf[6] = 0x01; // ASN.1 BOOLEAN type tag
  buf[7] = return_value ? 1 : 0;

  memset(buf + 8, 8, 8); // PKCS #7 padding

  encrypt(buf, 16);
}

void SparkProtocol::function_return(unsigned char *buf,
                                    unsigned char token,
                                    int return_value)
{
  unsigned short message_id = next_message_id();

  buf[0] = 0x51; // non-confirmable, one-byte token
  buf[1] = 0x44; // response code 2.04 CHANGED
  buf[2] = message_id >> 8;
  buf[3] = message_id & 0xff;
  buf[4] = token;
  buf[5] = 0xff; // payload marker
  buf[6] = 0x02; // ASN.1 INTEGER type tag
  buf[7] = return_value >> 24;
  buf[8] = return_value >> 16 & 0xff;
  buf[9] = return_value >> 8 & 0xff;
  buf[10] = return_value & 0xff;

  memset(buf + 11, 5, 5); // PKCS #7 padding

  encrypt(buf, 16);
}

void SparkProtocol::function_return(unsigned char *buf,
                                    unsigned char token,
                                    double return_value)
{
  unsigned short message_id = next_message_id();

  buf[0] = 0x51; // non-confirmable, one-byte token
  buf[1] = 0x44; // response code 2.04 CHANGED
  buf[2] = message_id >> 8;
  buf[3] = message_id & 0xff;
  buf[4] = token;
  buf[5] = 0xff; // payload marker
  buf[6] = 0x09; // ASN.1 REAL type tag, here meaning 8-byte double

  memcpy(buf + 7, &return_value, 8);

  buf[15] = 0x01; // PKCS #7 padding

  encrypt(buf, 16);
}

void SparkProtocol::function_return(unsigned char *buf,
                                    unsigned char token,
                                    const void *return_value,
                                    int length)
{
  unsigned short message_id = next_message_id();

  buf[0] = 0x51; // non-confirmable, one-byte token
  buf[1] = 0x44; // response code 2.04 CHANGED
  buf[2] = message_id >> 8;
  buf[3] = message_id & 0xff;
  buf[4] = token;
  buf[5] = 0xff; // payload marker
  buf[6] = 0x04; // ASN.1 OCTET STRING type tag

  memcpy(buf + 7, return_value, length);

  int msglen = 7 + length;
  int buflen = (msglen & ~15) + 16;
  char pad = buflen - msglen;
  memset(buf + msglen, pad, pad); // PKCS #7 padding

  encrypt(buf, buflen);
}

void SparkProtocol::variable_value(unsigned char *buf,
                                   unsigned char token,
                                   bool return_value)
{
  unsigned short message_id = next_message_id();

  buf[0] = 0x61; // acknowledgment, one-byte token
  buf[1] = 0x45; // response code 2.05 CONTENT
  buf[2] = message_id >> 8;
  buf[3] = message_id & 0xff;
  buf[4] = token;
  buf[5] = 0xff; // payload marker
  buf[6] = 0x01; // ASN.1 BOOLEAN type tag
  buf[7] = return_value ? 1 : 0;

  memset(buf + 8, 8, 8); // PKCS #7 padding

  encrypt(buf, 16);
}

void SparkProtocol::variable_value(unsigned char *buf,
                                   unsigned char token,
                                   int return_value)
{
  unsigned short message_id = next_message_id();

  buf[0] = 0x61; // acknowledgment, one-byte token
  buf[1] = 0x45; // response code 2.05 CONTENT
  buf[2] = message_id >> 8;
  buf[3] = message_id & 0xff;
  buf[4] = token;
  buf[5] = 0xff; // payload marker
  buf[6] = 0x02; // ASN.1 INTEGER type tag
  buf[7] = return_value >> 24;
  buf[8] = return_value >> 16 & 0xff;
  buf[9] = return_value >> 8 & 0xff;
  buf[10] = return_value & 0xff;

  memset(buf + 11, 5, 5); // PKCS #7 padding

  encrypt(buf, 16);
}

void SparkProtocol::variable_value(unsigned char *buf,
                                   unsigned char token,
                                   double return_value)
{
  unsigned short message_id = next_message_id();

  buf[0] = 0x61; // acknowledgment, one-byte token
  buf[1] = 0x45; // response code 2.05 CONTENT
  buf[2] = message_id >> 8;
  buf[3] = message_id & 0xff;
  buf[4] = token;
  buf[5] = 0xff; // payload marker
  buf[6] = 0x09; // ASN.1 REAL type tag, here meaning 8-byte double

  memcpy(buf + 7, &return_value, 8);

  buf[15] = 0x01; // PKCS #7 padding

  encrypt(buf, 16);
}

void SparkProtocol::variable_value(unsigned char *buf,
                                   unsigned char token,
                                   const void *return_value,
                                   int length)
{
  unsigned short message_id = next_message_id();

  buf[0] = 0x61; // acknowledgment, one-byte token
  buf[1] = 0x45; // response code 2.05 CONTENT
  buf[2] = message_id >> 8;
  buf[3] = message_id & 0xff;
  buf[4] = token;
  buf[5] = 0xff; // payload marker
  buf[6] = 0x04; // ASN.1 OCTET STRING type tag

  memcpy(buf + 7, return_value, length);

  int msglen = 7 + length;
  int buflen = (msglen & ~15) + 16;
  char pad = buflen - msglen;
  memset(buf + msglen, pad, pad); // PKCS #7 padding

  encrypt(buf, buflen);
}

void SparkProtocol::event(unsigned char *buf,
                          const char *event_name,
                          int event_name_length)
{
  // truncate event names that are too long for 4-bit CoAP option length
  if (event_name_length > 12)
    event_name_length = 12;

  unsigned short message_id = next_message_id();

  buf[0] = 0x50; // non-confirmable, no token
  buf[1] = 0x02; // code 0.02 POST request
  buf[2] = message_id >> 8;
  buf[3] = message_id & 0xff;
  buf[4] = 0xb1; // one-byte Uri-Path option
  buf[5] = 'e';
  buf[6] = event_name_length;
  
  memcpy(buf + 7, event_name, event_name_length);

  int msglen = 7 + event_name_length;
  int buflen = (msglen & ~15) + 16;
  char pad = buflen - msglen;
  memset(buf + msglen, pad, pad); // PKCS #7 padding

  encrypt(buf, buflen);
}

void SparkProtocol::event(unsigned char *buf,
                          const char *event_name,
                          int event_name_length,
                          const char *data,
                          int data_length)
{
  // truncate event names that are too long for 4-bit CoAP option length
  if (event_name_length > 12)
    event_name_length = 12;

  // truncate data to fit in one network packet
  if (data_length > 1024)
    data_length = 1024;

  unsigned short message_id = next_message_id();

  buf[0] = 0x50; // non-confirmable, no token
  buf[1] = 0x02; // code 0.02 POST request
  buf[2] = message_id >> 8;
  buf[3] = message_id & 0xff;
  buf[4] = 0xb1; // one-byte Uri-Path option
  buf[5] = 'e';
  buf[6] = event_name_length;
  
  memcpy(buf + 7, event_name, event_name_length);

  buf[7 + event_name_length] = 0xff; // payload marker

  memcpy(buf + 8 + event_name_length, data, data_length);

  int msglen = 8 + event_name_length + data_length;
  int buflen = (msglen & ~15) + 16;
  char pad = buflen - msglen;
  memset(buf + msglen, pad, pad); // PKCS #7 padding

  encrypt(buf, buflen);
}

unsigned short SparkProtocol::next_message_id()
{
  return ++_message_id;
}

void SparkProtocol::encrypt(unsigned char *buf, int length)
{
  aes_setkey_enc(&aes, key, 128);
  aes_crypt_cbc(&aes, AES_ENCRYPT, length, iv, buf, buf);
}

void SparkProtocol::separate_response(unsigned char *buf,
                                      unsigned char token,
                                      unsigned char code)
{
  unsigned short message_id = next_message_id();

  buf[0] = 0x51; // non-confirmable, one-byte token
  buf[1] = code;
  buf[2] = message_id >> 8;
  buf[3] = message_id & 0xff;
  buf[4] = token;

  memset(buf + 5, 11, 11); // PKCS #7 padding

  encrypt(buf, 16);
}
