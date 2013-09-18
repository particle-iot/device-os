#include "spark_protocol.h"
#include "handshake.h"
#include <string.h>
#include <stdlib.h>

SparkProtocol::SparkProtocol(const unsigned char *id,
                             const SparkKeys &keys,
                             const SparkCallbacks &callbacks,
                             SparkDescriptor *descriptor) : QUEUE_SIZE(640)
{
  queue_front = queue_back = queue = (unsigned char *) malloc(QUEUE_SIZE);
  queue_mem_boundary = queue + QUEUE_SIZE;
  memcpy(queue + 40, id, 12);
  rsa_keys = &keys;

  callback_send = callbacks.send;
  callback_receive = callbacks.receive;
  this->descriptor = descriptor;
}

SparkProtocol::SparkProtocol() : QUEUE_SIZE(640)
{
  queue_front = queue_back = queue = (unsigned char *) malloc(QUEUE_SIZE);
  queue_mem_boundary = queue + QUEUE_SIZE;
}

SparkProtocol::~SparkProtocol()
{
  free(queue);
}

int SparkProtocol::handshake(void)
{
  callback_receive(queue, 40);

  rsa_context rsa;
  init_rsa_context_with_public_key(&rsa, rsa_keys->server_public);
  int err = rsa_pkcs1_encrypt(&rsa, RSA_PUBLIC, 52, queue, queue + 52);
  rsa_free(&rsa);

  if (err) return err;

  callback_send(queue + 52, 256);
  callback_receive(queue, 512);

  err = set_key(queue);
  if (err) return err;

  queue[0] = 0x00;
  queue[1] = 0x10;
  hello(queue + 2);
  callback_send(queue, 18);

  return 0;
}

int SparkProtocol::init(const unsigned char *private_key,
                        const unsigned char *pubkey,
                        const unsigned char *signed_encrypted_credentials)
{
  unsigned char credentials[40];
  unsigned char hmac[20];

  if (0 != decipher_aes_credentials(private_key,
                                    signed_encrypted_credentials,
                                    credentials))
    return 1;

  calculate_ciphertext_hmac(signed_encrypted_credentials, credentials, hmac);

  if (0 == verify_signature(signed_encrypted_credentials + 256, pubkey, hmac))
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
  SparkProtocol::received_message(unsigned char *buf, int length)
{
  aes_setkey_dec(&aes, key, 128);
  aes_crypt_cbc(&aes, AES_DECRYPT, length, iv, buf, buf);

  char path = buf[ 5 + (buf[0] & 0x0F) ];

  switch (CoAP::code(buf))
  {
    case CoAPCode::GET:
      switch (path)
      {
        case 'v': return CoAPMessageType::VARIABLE_REQUEST;
        case 'd': return CoAPMessageType::DESCRIBE;
        default: return CoAPMessageType::ERROR;
      }
    case CoAPCode::POST:
      switch (path)
      {
        case 'h': return CoAPMessageType::HELLO;
        case 'f': return CoAPMessageType::FUNCTION_CALL;
        case 'u': return CoAPMessageType::UPDATE_BEGIN;
        case 'c': return CoAPMessageType::CHUNK;
        default: return CoAPMessageType::ERROR;
      }
    case CoAPCode::PUT:
      switch (path)
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

void SparkProtocol::key_changed(unsigned char *buf, unsigned char token)
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

void SparkProtocol::chunk_received(unsigned char *buf,
                                   unsigned char token,
                                   ChunkReceivedCode::Enum code)
{
  separate_response(buf, token, code);
}

void SparkProtocol::update_ready(unsigned char *buf, unsigned char token)
{
  separate_response(buf, token, 0x44);
}

void SparkProtocol::description(unsigned char *buf, unsigned char token,
                                const char **function_names, int num_functions)
{
  unsigned short message_id = next_message_id();

  buf[0] = 0x61; // acknowledgment, one-byte token
  buf[1] = 0x45; // response code 2.05 CONTENT
  buf[2] = message_id >> 8;
  buf[3] = message_id & 0xff;
  buf[4] = token;
  buf[5] = 0xff; // payload marker

  memcpy(buf + 6, "{\"f\":[", 6);

  unsigned char *buf_ptr = buf + 12;
  for (int i = 0; i < num_functions; ++i)
  {
    if (i)
    {
      *buf_ptr = ',';
      ++buf_ptr;
    }
    *buf_ptr = '"';
    ++buf_ptr;
    int function_name_length = strlen(function_names[i]);
    memcpy(buf_ptr, function_names[i], function_name_length);
    buf_ptr += function_name_length;
    *buf_ptr = '"';
    ++buf_ptr;
  }
  memcpy(buf_ptr, "],\"v\":[", 7);
  buf_ptr += 7;
  // handle variables later
  memcpy(buf_ptr, "]}", 2);
  buf_ptr += 2;

  int msglen = buf_ptr - buf;
  int buflen = (msglen & ~15) + 16;
  char pad = buflen - msglen;
  memset(buf_ptr, pad, pad); // PKCS #7 padding

  encrypt(buf, 32);
}


/********** Queue **********/

int SparkProtocol::queue_bytes_available()
{
  int unoccupied = queue_front - queue_back - 1;
  if (unoccupied < 0)
    return unoccupied + QUEUE_SIZE;
  else
    return unoccupied;
}

int SparkProtocol::queue_push(const char *src, int length)
{
  int available = queue_bytes_available();
  if (queue_back >= queue_front)
  {
    int tail_available = queue_mem_boundary - queue_back;
    if (length <= available)
    {
      if (length <= tail_available)
      {
        memcpy(queue_back, src, length);
        queue_back += length;
      }
      else
      {
        int head_needed = length - tail_available;
        memcpy(queue_back, src, tail_available);
        memcpy(queue, src + tail_available, head_needed);
        queue_back = queue + head_needed;
      }
      return length;
    }
    else
    {
      // queue_back is greater than or equal to queue_front
      // and length is greater than available
      if (available < tail_available)
      {
        // queue_front is equal to queue, so don't fill the last bucket
        memcpy(queue_back, src, available);
        queue_back += available;
      }
      else
      {
        int head_available = available - tail_available;
        memcpy(queue_back, src, tail_available);
        memcpy(queue, src + tail_available, head_available);
        queue_back = queue + head_available;
      }
      return available;
    }
  }
  else
  {
    // queue_back is less than queue_front
    int count = length < available ? length : available;
    memcpy(queue_back, src, count);
    queue_back += count;
    return count;
  }
}

int SparkProtocol::queue_pop(char *dst, int length)
{
  if (queue_back >= queue_front)
  {
    int filled = queue_back - queue_front;
    int count = length <= filled ? length : filled;

    memcpy(dst, queue_front, count);
    queue_front += count;
    return count;
  }
  else
  {
    int tail_filled = queue_mem_boundary - queue_front;
    int head_requested = length - tail_filled;
    int head_filled = queue_back - queue;
    int head_count = head_requested < head_filled ? head_requested : head_filled;
    
    memcpy(dst, queue_front, tail_filled);
    memcpy(dst + tail_filled, queue, head_count);
    queue_front = queue + head_count;
    return tail_filled + head_count;
  }
}

ProtocolState::Enum SparkProtocol::state()
{
  return ProtocolState::READ_NONCE;
}


/********** Private methods **********/

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

int SparkProtocol::set_key(const unsigned char *signed_encrypted_credentials)
{
  unsigned char credentials[40];
  unsigned char hmac[20];

  if (0 != decipher_aes_credentials(rsa_keys->core_private,
                                    signed_encrypted_credentials,
                                    credentials))
    return 1;

  calculate_ciphertext_hmac(signed_encrypted_credentials, credentials, hmac);

  if (0 == verify_signature(signed_encrypted_credentials + 256,
                            rsa_keys->server_public,
                            hmac))
  {
    memcpy(key,  credentials,      16);
    memcpy(iv,   credentials + 16, 16);
    memcpy(salt, credentials + 32,  8);
    _message_id = *(credentials + 32) << 8 | *(credentials + 33);

    return 0;
  }
  else return 2;
}
