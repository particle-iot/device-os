/**
  ******************************************************************************
  * @file    core_protocol.cpp
  * @authors  Zachary Crockett, Matthew McGowan
  * @version V1.0.0
  * @date    15-Nov-2013
  * @brief   CORE PROTOCOL
  ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

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

#include "logging.h"
LOG_SOURCE_CATEGORY("comm.coreprotocol")

#include "core_protocol.h"
#include "protocol_selector.h"

#if !PARTICLE_PROTOCOL

#include "protocol_defs.h"
#include "handshake.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "device_keys.h"
#include "service_debug.h"
#include "messages.h"

#ifdef USE_MBEDTLS
#include "mbedtls_compat.h"
#include "mbedtls_util.h"
#endif

#include "communication_diagnostic.h"

using namespace particle::protocol;

#if 0
extern void serial_dump(const char* msg, ...);
#else
#define serial_dump(x, ...)
#endif

static inline size_t round_to_16(size_t len)
{
  if (len == 0)
    return len;
  size_t rem = len % 16;
  if (rem != 0) {
    len += 16 - rem;
  }
  return len;
}

static inline int message_padding_strip(uint8_t* buf, int len)
{
  if (len > 0) {
    int nopadlen = len - (int)buf[len - 1];
    if (nopadlen < 0)
      nopadlen = 0;
    return nopadlen;
  }

  return 0;
}

/**
 * Handle the cryptographically secure random seed from the cloud by using
 * it to seed the stdlib PRNG.
 * @param seed  A random value from a cryptographically secure random number generator.
 */
inline void default_random_seed_from_cloud(unsigned int seed)
{
    srand(seed);
}
bool CoreProtocol::is_initialized(void)
{
  return initialized;
}

void CoreProtocol::reset_updating(void)
{
  updating = false;
  last_chunk_millis = 0;    // this is used for the time latency also
  timesync_.reset();
}

CoreProtocol::CoreProtocol() :
    QUEUE_SIZE(sizeof(queue)),
    handlers({sizeof(handlers), NULL}),
    last_ack_handlers_update(0),
    expecting_ping_ack(false),
    initialized(false),
    updating(false),
    product_id(PRODUCT_ID),
    product_firmware_version(PRODUCT_FIRMWARE_VERSION)
{
    queue_init();
}


void CoreProtocol::init(const char *id,
                         const SparkKeys &keys,
                         const SparkCallbacks &callbacks,
                         const SparkDescriptor &descriptor)
{
  memcpy(server_public_key, keys.server_public, MAX_SERVER_PUBLIC_KEY_LENGTH);
  memcpy(core_private_key, keys.core_private, MAX_DEVICE_PRIVATE_KEY_LENGTH);
  memcpy(device_id, id, 12);

  // when using this lib in C, constructor is never called
  queue_init();

  this->callbacks = callbacks;
  this->descriptor = descriptor;

  memset(event_handlers, 0, sizeof(event_handlers));

  initialized = true;
}

int CoreProtocol::handshake(void)
{
  LOG_CATEGORY("comm.CoreProtocol.handshake");

  // FIXME: Pending completion handlers should be cancelled at the end of a previous session
  ack_handlers.clear();
  last_ack_handlers_update = callbacks.millis();

  LOG(INFO,"Started: Receive nonce");
  memcpy(queue + 40, device_id, 12);
  int err = blocking_receive(queue, 40);
  if (0 > err) { LOG(ERROR,"Could not receive nonce: %d", err);  return err; }

  LOG(INFO,"Encrypting handshake nonce");
  extract_public_rsa_key(queue+52, core_private_key);

  rsa_context rsa;
  init_rsa_context_with_public_key(&rsa, server_public_key);
  const int len = 52+MAX_DEVICE_PUBLIC_KEY_LENGTH;
#ifdef USE_MBEDTLS
  err = mbedtls_rsa_pkcs1_encrypt(&rsa, mbedtls_default_rng, nullptr, MBEDTLS_RSA_PUBLIC, len, queue, queue + len);
#else
  err = rsa_pkcs1_encrypt(&rsa, RSA_PUBLIC, len, queue, queue + len);
#endif
  rsa_free(&rsa);

  if (err) { LOG(ERROR,"RSA encrypt error %d", err); return err; }

  LOG(INFO,"Sending encrypted nonce");
  blocking_send(queue + len, 256);
  LOG(INFO,"Receive key");
  err = blocking_receive(queue, 384);
  if (0 > err) { LOG(ERROR,"Unable to receive key %d", err); return err; }

  LOG(INFO,"Setting key");
  err = set_key(queue);
  if (err) { LOG(ERROR,"Could not set key, %d"); return err; }

  LOG(INFO,"Sending HELLO message");
  hello(queue, descriptor.was_ota_upgrade_successful());
  err = blocking_send(queue, 18);
  if (0 > err) { LOG(ERROR,"Could not send HELLO message: %d", err); return err; }

  LOG(INFO,"Receiving HELLO response");
  if (!event_loop(CoAPMessageType::HELLO, 2000))        // read the hello message from the server
  {
      LOG(ERROR,"Could not receive HELLO response");
      return -1;
  }
  LOG(INFO,"Completed");
  return 0;
}

bool CoreProtocol::event_loop(CoAPMessageType::Enum message_type, system_tick_t timeout)
{
    system_tick_t start = callbacks.millis();
    do
    {
        CoAPMessageType::Enum msgtype;
        if (!event_loop(msgtype))
            return false;
        if (msgtype==message_type)
            return true;
        // todo - ideally need a delay here
    }
    while ((callbacks.millis()-start) < timeout);
    return false;
}


// Returns true if no errors and still connected.
// Returns false if there was an error, and we are probably disconnected.
bool CoreProtocol::event_loop(CoAPMessageType::Enum& message_type)
{
  // Process expired completion handlers
  const system_tick_t t = callbacks.millis();
  ack_handlers.update(t - last_ack_handlers_update);
  last_ack_handlers_update = t;

    message_type = CoAPMessageType::NONE;
  int bytes_received = callbacks.receive(queue, 2, nullptr);
  if (2 <= bytes_received)
  {
    message_type = handle_received_message();
    if (message_type==CoAPMessageType::ERROR)
    {
      LOG(WARN,"received ERROR CoAPMessage");
      if (updating) {      // was updating but had an error, inform the client
        serial_dump("handle received message failed - aborting transfer");
        callbacks.finish_firmware_update(file, UpdateFlag::ERROR, NULL);
        updating = false;
      }

      // bail if and only if there was an error
      return false;
    }
  }
  else
  {
    if (0 > bytes_received)
    {
      LOG(WARN,"bytes recieved error %d", bytes_received);
      // error, disconnected
      return false;
    }

    if (updating)
    {
      system_tick_t millis_since_last_chunk = callbacks.millis() - last_chunk_millis;
      if (3000 < millis_since_last_chunk)
      {
          if (updating==2) {    // send missing chunks
              serial_dump("timeout - resending missing chunks");
              if (!send_missing_chunks(MISSED_CHUNKS_TO_SEND))
                  return false;
          }
          /* Do not resend chunks since this can cause duplicates on the server.
          else
          {
            queue[0] = 0;
            queue[1] = 16;
            chunk_missed(queue + 2, chunk_index);
            if (0 > blocking_send(queue, 18))
            {
              // error
              return false;
            }
          }
          */
          last_chunk_millis = callbacks.millis();
      }
    }
    else
    {
      system_tick_t millis_since_last_message = callbacks.millis() - last_message_millis;
      if (expecting_ping_ack)
      {
        if (10000 < millis_since_last_message)
        {
          // timed out, disconnect
          expecting_ping_ack = false;
          last_message_millis = callbacks.millis();
          LOG(WARN,"ping ACK not received");
          return false;
        }
      }
      else
      {
        if (15000 < millis_since_last_message)
        {
          queue[0] = 0;
          queue[1] = 16;
          ping(queue + 2);
          blocking_send(queue, 18);

          expecting_ping_ack = true;
          last_message_millis = callbacks.millis();
        }
      }
    }
  }

  // no errors, still connected
  return true;
}

// Returns bytes sent or -1 on error
int CoreProtocol::blocking_send(const unsigned char *buf, int length)
{
  int bytes_or_error;
  int byte_count = 0;

  system_tick_t _millis = callbacks.millis();

  while (length > byte_count)
  {
    bytes_or_error = callbacks.send(buf + byte_count, length - byte_count, nullptr);
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
int CoreProtocol::blocking_receive(unsigned char *buf, int length)
{
  int bytes_or_error;
  int byte_count = 0;

  system_tick_t _millis = callbacks.millis();

  while (length > byte_count)
  {
    bytes_or_error = callbacks.receive(buf + byte_count, length - byte_count, nullptr);
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

CoAPMessageType::Enum
  CoreProtocol::received_message(unsigned char *buf, size_t length)
{
  unsigned char next_iv[16];
  memcpy(next_iv, buf, 16);

#ifdef USE_MBEDTLS
  mbedtls_aes_setkey_dec(&aes, key, 128);
  mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_DECRYPT, round_to_16(length), iv_receive, buf, buf);
#else
  aes_setkey_dec(&aes, key, 128);
  aes_crypt_cbc(&aes, AES_DECRYPT, length, iv_receive, buf, buf);
#endif

  memcpy(iv_receive, next_iv, 16);

  return Messages::decodeType(buf, length);
}

void CoreProtocol::hello(unsigned char *buf, bool newly_upgraded)
{
  unsigned short message_id = next_message_id();
  uint8_t flags = newly_upgraded ? 1 : 0;
  // diagnostics are not supported in this protocol implementation.
  size_t len = Messages::hello(buf+2, message_id, flags, PLATFORM_ID, product_id, product_firmware_version, false, nullptr, 0);
  wrap(buf, len);
}

void CoreProtocol::key_changed(unsigned char *buf, unsigned char token)
{
  separate_response(buf, token, 0x44);
}

void CoreProtocol::function_return(unsigned char *buf,
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
  buf[6] = return_value >> 24;
  buf[7] = return_value >> 16 & 0xff;
  buf[8] = return_value >> 8 & 0xff;
  buf[9] = return_value & 0xff;

  memset(buf + 10, 6, 6); // PKCS #7 padding

  encrypt(buf, 16);
}

void CoreProtocol::variable_value(unsigned char *buf,
                                   unsigned char token,
                                   unsigned char message_id_msb,
                                   unsigned char message_id_lsb,
                                   bool return_value)
{
  buf[0] = 0x61; // acknowledgment, one-byte token
  buf[1] = 0x45; // response code 2.05 CONTENT
  buf[2] = message_id_msb;
  buf[3] = message_id_lsb;
  buf[4] = token;
  buf[5] = 0xff; // payload marker
  buf[6] = return_value ? 1 : 0;

  memset(buf + 7, 9, 9); // PKCS #7 padding

  encrypt(buf, 16);
}

void CoreProtocol::variable_value(unsigned char *buf,
                                   unsigned char token,
                                   unsigned char message_id_msb,
                                   unsigned char message_id_lsb,
                                   int return_value)
{
  buf[0] = 0x61; // acknowledgment, one-byte token
  buf[1] = 0x45; // response code 2.05 CONTENT
  buf[2] = message_id_msb;
  buf[3] = message_id_lsb;
  buf[4] = token;
  buf[5] = 0xff; // payload marker
  buf[6] = return_value >> 24;
  buf[7] = return_value >> 16 & 0xff;
  buf[8] = return_value >> 8 & 0xff;
  buf[9] = return_value & 0xff;

  memset(buf + 10, 6, 6); // PKCS #7 padding

  encrypt(buf, 16);
}

void CoreProtocol::variable_value(unsigned char *buf,
                                   unsigned char token,
                                   unsigned char message_id_msb,
                                   unsigned char message_id_lsb,
                                   double return_value)
{
  buf[0] = 0x61; // acknowledgment, one-byte token
  buf[1] = 0x45; // response code 2.05 CONTENT
  buf[2] = message_id_msb;
  buf[3] = message_id_lsb;
  buf[4] = token;
  buf[5] = 0xff; // payload marker

  memcpy(buf + 6, &return_value, 8);

  memset(buf + 14, 2, 2); // PKCS #7 padding

  encrypt(buf, 16);
}

// Returns the length of the buffer to send
int CoreProtocol::variable_value(unsigned char *buf,
                                  unsigned char token,
                                  unsigned char message_id_msb,
                                  unsigned char message_id_lsb,
                                  const void *return_value,
                                  int length)
{
  buf[0] = 0x61; // acknowledgment, one-byte token
  buf[1] = 0x45; // response code 2.05 CONTENT
  buf[2] = message_id_msb;
  buf[3] = message_id_lsb;
  buf[4] = token;
  buf[5] = 0xff; // payload marker

  memcpy(buf + 6, return_value, length);

  int msglen = 6 + length;
  int buflen = (msglen & ~15) + 16;
  char pad = buflen - msglen;
  memset(buf + msglen, pad, pad); // PKCS #7 padding

  encrypt(buf, buflen);

  return buflen;
}

inline bool is_system(const char* event_name) {
    return !strncmp(event_name, "spark/", 6);
}

// Returns true on success, false on sending timeout or rate-limiting failure
bool CoreProtocol::send_event(const char *event_name, const char *data, int ttl, EventType::Enum event_type,
                               int flags, CompletionHandler handler)
{
  if (updating)
  {
    handler.setError(SYSTEM_ERROR_BUSY);
    return false;
  }

  bool is_system_event = is_system(event_name);

  if (is_system_event) {
      static uint16_t lastMinute = 0;
      static uint8_t eventsThisMinute = 0;

      uint16_t currentMinute = uint16_t(callbacks.millis()>>16);
      if (currentMinute==lastMinute) {      // == handles millis() overflow
          if (eventsThisMinute==255) {
              g_rateLimitedEventsCounter++;
              handler.setError(SYSTEM_ERROR_LIMIT_EXCEEDED);
              return false;
          }
      }
      else {
          lastMinute = currentMinute;
          eventsThisMinute = 0;
      }
      eventsThisMinute++;
  }
  else {
    static system_tick_t recent_event_ticks[5] = {
      (system_tick_t) -1000, (system_tick_t) -1000,
      (system_tick_t) -1000, (system_tick_t) -1000,
      (system_tick_t) -1000 };
    static int evt_tick_idx = 0;

    system_tick_t now = recent_event_ticks[evt_tick_idx] = callbacks.millis();
    evt_tick_idx++;
    evt_tick_idx %= 5;
    if (now - recent_event_ticks[evt_tick_idx] < 1000)
    {
      // exceeded allowable burst of 4 events per second
      g_rateLimitedEventsCounter++;
      handler.setError(SYSTEM_ERROR_LIMIT_EXCEEDED);
      return false;
    }
  }
  uint16_t msg_id = next_message_id();
  const bool confirmable = flags & EventType::WITH_ACK;
  size_t msglen = Messages::event(queue + 2, msg_id, event_name, data, ttl, event_type, confirmable);
  size_t wrapped_len = wrap(queue, msglen);
  const int n = blocking_send(queue, wrapped_len);
  if (n < 0) {
    handler.setError(SYSTEM_ERROR_IO);
    return false;
  }
  // Currently, the server sends acknowledgements for all published events, regardless of whether
  // original request has been sent as confirmable or non-confirmable CoAP message. Here we register
  // completion handler only if acknowledgement was requested explicitly
  if (flags & EventType::WITH_ACK) {
    ack_handlers.addHandler(msg_id, std::move(handler), SEND_EVENT_ACK_TIMEOUT);
  } else {
    handler.setResult();
  }
  return true;
}

size_t CoreProtocol::time_request(unsigned char *buf)
{
	  uint16_t msg_id = next_message_id();
	  uint8_t token = next_token();
	  return Messages::time_request(buf, msg_id, token);
}

// returns true on success, false on failure
bool CoreProtocol::send_time_request(void)
{
  if (updating)
  {
    return false;
  }

  return timesync_.send_request(callbacks.millis(), [&]() {
    size_t msglen = time_request(queue + 2);
    size_t wrapped_len = wrap(queue, msglen);
    last_chunk_millis = callbacks.millis();

    return (0 <= blocking_send(queue, wrapped_len));
  });
}

bool CoreProtocol::send_subscription(const char *event_name, const char *device_id)
{
  uint16_t msg_id = next_message_id();
  size_t msglen = subscription(queue + 2, msg_id, event_name, device_id);

  size_t buflen = (msglen & ~15) + 16;
  char pad = buflen - msglen;
  memset(queue + 2 + msglen, pad, pad); // PKCS #7 padding

  encrypt(queue + 2, buflen);

  queue[0] = (buflen >> 8) & 0xff;
  queue[1] = buflen & 0xff;

  return (0 <= blocking_send(queue, buflen + 2));
}

bool CoreProtocol::send_subscription(const char *event_name,
                                      SubscriptionScope::Enum scope)
{
  uint16_t msg_id = next_message_id();
  size_t msglen = subscription(queue + 2, msg_id, event_name, scope);

  size_t buflen = (msglen & ~15) + 16;
  char pad = buflen - msglen;
  memset(queue + 2 + msglen, pad, pad); // PKCS #7 padding

  encrypt(queue + 2, buflen);

  queue[0] = (buflen >> 8) & 0xff;
  queue[1] = buflen & 0xff;

  return (0 <= blocking_send(queue, buflen + 2));
}

void CoreProtocol::send_subscriptions()
{
  const int NUM_HANDLERS = sizeof(event_handlers) / sizeof(FilteringEventHandler);
  for (int i = 0; i < NUM_HANDLERS; i++)
  {
    if (NULL != event_handlers[i].handler)
    {
        if (event_handlers[i].device_id[0])
        {
            send_subscription(event_handlers[i].filter, event_handlers[i].device_id);
        }
        else
        {
            send_subscription(event_handlers[i].filter, event_handlers[i].scope);
        }
    }
  }
}

void CoreProtocol::remove_event_handlers(const char* event_name)
{
    if (NULL == event_name)
    {
        memset(event_handlers, 0, sizeof(event_handlers));
    }
    else
    {
        const int NUM_HANDLERS = sizeof(event_handlers) / sizeof(FilteringEventHandler);
        int dest = 0;
        for (int i = 0; i < NUM_HANDLERS; i++)
        {
          if (!strcmp(event_name, event_handlers[i].filter))
          {
              memset(&event_handlers[i], 0, sizeof(event_handlers[i]));
          }
          else
          {
              if (dest!=i) {
                memcpy(event_handlers+dest, event_handlers+i, sizeof(event_handlers[i]));
                memset(event_handlers+i, 0, sizeof(event_handlers[i]));
              }
              dest++;
          }
        }
    }
}

bool CoreProtocol::event_handler_exists(const char *event_name, EventHandler handler,
    void *handler_data, SubscriptionScope::Enum scope, const char* id)
{
  const int NUM_HANDLERS = sizeof(event_handlers) / sizeof(FilteringEventHandler);
  for (int i = 0; i < NUM_HANDLERS; i++)
  {
      if (event_handlers[i].handler==handler &&
          event_handlers[i].handler_data==handler_data &&
          event_handlers[i].scope==scope) {
        const size_t MAX_FILTER_LEN = sizeof(event_handlers[i].filter);
        const size_t FILTER_LEN = strnlen(event_name, MAX_FILTER_LEN);
        if (!strncmp(event_handlers[i].filter, event_name, FILTER_LEN)) {
            const size_t MAX_ID_LEN = sizeof(event_handlers[i].device_id)-1;
            const size_t id_len = id ? strnlen(id, MAX_ID_LEN) : 0;
            if (id_len)
                return !strncmp(event_handlers[i].device_id, id, id_len);
            else
                return !event_handlers[i].device_id[0];
        }
      }
  }
  return false;
}

bool CoreProtocol::add_event_handler(const char *event_name, EventHandler handler,
    void *handler_data, SubscriptionScope::Enum scope, const char* id)
{
    if (event_handler_exists(event_name, handler, handler_data, scope, id))
        return true;

  const int NUM_HANDLERS = sizeof(event_handlers) / sizeof(FilteringEventHandler);
  for (int i = 0; i < NUM_HANDLERS; i++)
  {
    if (NULL == event_handlers[i].handler)
    {
      const size_t MAX_FILTER_LEN = sizeof(event_handlers[i].filter);
      const size_t FILTER_LEN = strnlen(event_name, MAX_FILTER_LEN);
      memcpy(event_handlers[i].filter, event_name, FILTER_LEN);
      memset(event_handlers[i].filter + FILTER_LEN, 0, MAX_FILTER_LEN - FILTER_LEN);
      event_handlers[i].handler = handler;
      event_handlers[i].handler_data = handler_data;
      event_handlers[i].device_id[0] = 0;
        const size_t MAX_ID_LEN = sizeof(event_handlers[i].device_id)-1;
        const size_t id_len = id ? strnlen(id, MAX_ID_LEN) : 0;
        memcpy(event_handlers[i].device_id, id, id_len);
        event_handlers[i].device_id[id_len] = 0;
        event_handlers[i].scope = scope;
      return true;
    }
  }
  return false;
}

void CoreProtocol::chunk_received(unsigned char *buf,
                                   unsigned char token,
                                   ChunkReceivedCode::Enum code)
{
  separate_response(buf, token, code);
}


int CoreProtocol::send_missing_chunks(int count)
{
    int sent = 0;
    chunk_index_t idx = 0;

    uint8_t* buf = queue+2;
    unsigned short message_id = next_message_id();
    buf[0] = 0x40; // confirmable, no token
    buf[1] = 0x01; // code 0.01 GET
    buf[2] = message_id >> 8;
    buf[3] = message_id & 0xff;
    buf[4] = 0xb1; // one-byte Uri-Path option
    buf[5] = 'c';
    buf[6] = 0xff; // payload marker

    while ((idx=next_chunk_missing(chunk_index_t(idx)))!=NO_CHUNKS_MISSING && sent<count)
    {
        buf[(sent*2)+7] = idx >> 8;
        buf[(sent*2)+8] = idx & 0xFF;

        missed_chunk_index = idx;
        idx++;
        sent++;
    }

    if (sent>0) {
        LOG(WARN,"Sent %d missing chunks", sent);

        size_t message_size = 7+(sent*2);
        message_size = wrap(queue, message_size);
        if (0 > blocking_send(queue, message_size))
            return -1;
    }
    return sent;
}

void CoreProtocol::chunk_missed(unsigned char *buf, unsigned short chunk_index)
{
  unsigned short message_id = next_message_id();

  buf[0] = 0x40; // confirmable, no token
  buf[1] = 0x01; // code 0.01 GET
  buf[2] = message_id >> 8;
  buf[3] = message_id & 0xff;
  buf[4] = 0xb1; // one-byte Uri-Path option
  buf[5] = 'c';
  buf[6] = 0xff; // payload marker
  buf[7] = chunk_index >> 8;
  buf[8] = chunk_index & 0xff;

  memset(buf + 9, 7, 7); // PKCS #7 padding

  encrypt(buf, 16);
}

void CoreProtocol::update_ready(unsigned char *buf, unsigned char token)
{
    separate_response_with_payload(buf, token, 0x44, NULL, 0);
}

void CoreProtocol::update_ready(unsigned char *buf, unsigned char token, uint8_t flags)
{
    separate_response_with_payload(buf, token, 0x44, &flags, 1);
}

int CoreProtocol::description(unsigned char *buf, unsigned char token,
                               unsigned char message_id_msb, unsigned char message_id_lsb, int desc_flags)
{
    buf[0] = 0x61; // message type, token length
    buf[1] = 0x45; // response code
    buf[2] = message_id_msb;
    buf[3] = message_id_lsb;
    buf[4] = token;
    buf[5] = 0xff; // payload marker
    return build_describe_message(buf, 6, desc_flags);
}

int CoreProtocol::build_post_description(unsigned char *buf, int desc_flags) {
    const unsigned short msg_id = next_message_id();
    buf[0] = 0x40; // message type, token length
    buf[1] = 0x02; // response code
    buf[2] = ((msg_id >> 8) & 0xFF);
    buf[3] = (msg_id & 0xFF);
    buf[4] = 0xb1; // Uri-Path option of length 1
    buf[5] = 'd';
    buf[6] = 0x41; // Uri-Query option of length 1
    buf[7] = (desc_flags & 0xFF);
    buf[8] = 0xff; // payload marker
    return build_describe_message(buf, 9, desc_flags);
}

int CoreProtocol::build_describe_message(unsigned char *buf, unsigned char offset, int desc_flags)
{
    BufferAppender appender(buf+offset, QUEUE_SIZE-(offset + 2));
    // diagnostics must be requested in isolation to be a binary packet
    if (descriptor.append_metrics && (desc_flags == DESCRIBE_METRICS))
    {
        appender.append(char(0));	// null byte means binary data
        appender.append(char(DESCRIBE_METRICS)); 	// uint16 describes the type of binary packet
        appender.append(char(0));	//
        const int flags = 1;		// binary
        const int page = 0;
        descriptor.append_metrics(append_instance, &appender, flags, page, nullptr);
    }
    else
    {
        appender.append("{");
        bool has_content = false;

        if (desc_flags & DESCRIBE_APPLICATION)
        {
            has_content = true;
            appender.append("\"f\":[");

            int num_keys = descriptor.num_functions();
            int i;
            for (i = 0; i < num_keys; ++i)
            {
                if (i)
                {
                    appender.append(',');
                }
                appender.append('"');

                const char* key = descriptor.get_function_key(i);
                size_t function_name_length = strlen(key);
                if (MAX_FUNCTION_KEY_LENGTH < function_name_length)
                {
                    function_name_length = MAX_FUNCTION_KEY_LENGTH;
                }
                appender.append((const uint8_t*)key, function_name_length);
                appender.append('"');
            }

            appender.append("],\"v\":{");

            num_keys = descriptor.num_variables();
            for (i = 0; i < num_keys; ++i)
            {
                if (i)
                {
                    appender.append(',');
                }
                appender.append('"');
                const char* key = descriptor.get_variable_key(i);
                size_t variable_name_length = strlen(key);
                SparkReturnType::Enum t = descriptor.variable_type(key);
                if (MAX_VARIABLE_KEY_LENGTH < variable_name_length)
                {
                    variable_name_length = MAX_VARIABLE_KEY_LENGTH;
                }
                appender.append((const uint8_t*)key, variable_name_length);
                appender.append("\":");
                appender.append('0' + (char)t);
            }
            appender.append('}');
        }

        if (descriptor.append_system_info && (desc_flags & DESCRIBE_SYSTEM))
        {
            if (has_content)
            {
                appender.append(',');
            }
            descriptor.append_system_info(append_instance, &appender, NULL);
        }
        appender.append('}');
    }
    int msglen = appender.next() - (uint8_t *)buf;


    int buflen = (msglen & ~15) + 16;
    char pad = buflen - msglen;
    memset(buf+msglen, pad, pad); // PKCS #7 padding

    encrypt(buf, buflen);
    return buflen;
}

void CoreProtocol::ping(unsigned char *buf)
{
  unsigned short message_id = next_message_id();

  buf[0] = 0x40; // Confirmable, no token
  buf[1] = 0x00; // code signifying empty message
  buf[2] = message_id >> 8;
  buf[3] = message_id & 0xff;

  memset(buf + 4, 12, 12); // PKCS #7 padding

  encrypt(buf, 16);
}

int CoreProtocol::presence_announcement(unsigned char *buf, const unsigned char *id)
{
  buf[0] = 0x50; // Confirmable, no token
  buf[1] = 0x02; // Code POST
  buf[2] = 0x00; // message id ignorable in this context
  buf[3] = 0x00;
  buf[4] = 0xb1; // Uri-Path option of length 1
  buf[5] = 'h';
  buf[6] = 0xff; // payload marker

  memcpy(buf + 7, id, 12);

  return 19;
}


/********** Queue **********/


#if 0

int CoreProtocol::queue_bytes_available()
{
  int unoccupied = queue_front - queue_back - 1;
  if (unoccupied < 0)
    return unoccupied + QUEUE_SIZE;
  else
    return unoccupied;
}

// these methods are unused
int CoreProtocol::queue_push(const char *src, int length)
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

int CoreProtocol::queue_pop(char *dst, int length)
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
#endif

ProtocolState::Enum CoreProtocol::state()
{
  return ProtocolState::READ_NONCE;
}

/********** Private methods **********/

/**
 * Pads and encrypts the buffer, and prepends the buffer length.
 * @param buf
 * @param msglen
 * @return
 */
size_t CoreProtocol::wrap(unsigned char *buf, size_t msglen)
{
  size_t buflen = (msglen & ~15) + 16;
  char pad = buflen - msglen;
  memset(buf + 2 + msglen, pad, pad); // PKCS #7 padding

  encrypt(buf + 2, buflen);

  buf[0] = (buflen >> 8) & 0xff;
  buf[1] = buflen & 0xff;

  return buflen + 2;
}

bool CoreProtocol::handle_update_begin(msg& message)
{
    // send ACK
    uint8_t* msg_to_send = message.response;
    *msg_to_send = 0;
    *(msg_to_send + 1) = 16;

    uint8_t flags = 0;
    int actual_len = message.len - queue[message.len-1];
    if (actual_len>=20 && queue[7]==0xFF) {
        flags = decode_uint8(queue+8);
        file.chunk_size = decode_uint16(queue+9);
        file.file_length = decode_uint32(queue+11);
        file.store = FileTransfer::Store::Enum(decode_uint8(queue+15));
        file.file_address = decode_uint32(queue+16);
        file.chunk_address = file.file_address;
    }
    else {
        file.chunk_size = 0;
        file.file_length = 0;
        file.store = FileTransfer::Store::FIRMWARE;
        file.file_address = 0;
        file.chunk_address = 0;
    }

    // check the parameters only
    bool success = !callbacks.prepare_for_firmware_update(file, 1, NULL);
    if (success) {
        success = file.chunk_count(file.chunk_size) < MAX_CHUNKS;
    }

    coded_ack(msg_to_send+2, success ? 0x00 : RESPONSE_CODE(4,00), queue[2], queue[3]);
    if (0 > blocking_send(msg_to_send, 18))
    {
      // error
      return false;
    }
    if (success)
    {
        if (!callbacks.prepare_for_firmware_update(file, 0, NULL))
        {
            LOG(WARN,"starting file length %d chunks %d chunk_size %d",
                    file.file_length, file.chunk_count(file.chunk_size), file.chunk_size);
            last_chunk_millis = callbacks.millis();
            chunk_index = 0;
            chunk_size = file.chunk_size;   // save chunk size since the descriptor size is overwritten
            this->updating = 1;
            // when not in fast OTA mode, the chunk missing buffer is set to 1 since the protocol
            // handles missing chunks one by one. Also we don't know the actual size of the file to
            // know the correct size of the bitmap.
            set_chunks_received(flags & 1 ? 0 : 0xFF);

            // send update_reaady - use fast OTA if available
            update_ready(msg_to_send + 2, message.token, (flags & 0x1));
            if (0 > blocking_send(msg_to_send, 18))
            {
              // error
              return false;
            }
        }
    }

    return true;
}

bool CoreProtocol::handle_chunk(msg& message)
{
    last_chunk_millis = callbacks.millis();

    uint8_t* msg_to_send = message.response;

    LOG(INFO,"chunk");
    if (!this->updating) {
        //LOG(WARN,"got chunk when not updating");
        return true;
    }

    bool fast_ota = false;
    uint8_t payload = 7;

    unsigned option = 0;
    uint32_t given_crc = 0;
    while (queue[payload]!=0xFF) {
        switch (option) {
            case 0:
                given_crc = decode_uint32(queue+payload+1);
                break;
            case 1:
                this->chunk_index = decode_uint16(queue+payload+1);
                fast_ota = true;
                break;
        }
        option++;
        payload += (queue[payload]&0xF)+1;  // increase by the size. todo handle > 11
    }

    if (!fast_ota) {
        // Send ACK
        // Issue #1240: The firmware should not ACK every Chunk message in Fast OTA mode
        *msg_to_send = 0;
        *(msg_to_send + 1) = 16;
        empty_ack(msg_to_send + 2, queue[2], queue[3]);
        if (0 > blocking_send(msg_to_send, 18))
        {
          // error
          return false;
        }
    }

    if (0xFF==queue[payload])
    {
        payload++;
        const uint8_t* chunk = queue+payload;
        file.chunk_size = message.len - payload - queue[message.len - 1];   // remove length added due to pkcs #7 padding?
        file.chunk_address  = file.file_address + (chunk_index * chunk_size);
        if (chunk_index>=MAX_CHUNKS) {
            LOG(WARN,"invalid chunk index %d", chunk_index);
            return false;
        }
        uint32_t crc = callbacks.calculate_crc(chunk, file.chunk_size);
        size_t response_size = 0;
        bool crc_valid = (crc == given_crc);
        LOG(INFO,"chunk idx=%d crc=%d fast=%d updating=%d", chunk_index, crc_valid, fast_ota, updating);
        if (crc_valid)
        {
            callbacks.save_firmware_chunk(file, chunk, NULL);
            //if (!fast_ota || (updating!=2 && (true || (chunk_index & 32)==0))) {
            // Issue #1240:
            // In Fast OTA mode, the firmware should not respond to every Chunk message
            // (especially because they are NON confirmable messages).
            if (!fast_ota) {
                chunk_received(msg_to_send + 2, message.token, ChunkReceivedCode::OK);
                response_size = 18;
            }
            flag_chunk_received(chunk_index);
            if (updating==2) {                      // clearing up missed chunks at the end of fast OTA
                chunk_index_t next_missed = next_chunk_missing(0);
                if (next_missed==NO_CHUNKS_MISSING) {
                    LOG(INFO,"received all chunks");
                    reset_updating();
                    response_size = notify_update_done(msg_to_send, 0, 0);
                    callbacks.finish_firmware_update(file, UpdateFlag::SUCCESS, NULL);
                }
                else {
                    if (response_size && 0 > blocking_send(msg_to_send, 18)) {
                        LOG(WARN,"send chunk response failed");
                        return false;
                    }
                    response_size = 0;

                    if (next_missed>missed_chunk_index)
                        send_missing_chunks(MISSED_CHUNKS_TO_SEND);
                }
            }
            chunk_index++;
        }
        else if (!fast_ota)
        {
            chunk_received(msg_to_send + 2, message.token, ChunkReceivedCode::BAD);
            response_size = 18;
            LOG(WARN,"chunk bad %d", chunk_index);
        }
        // fast OTA will request the chunk later

        if (response_size && 0 > blocking_send(msg_to_send, response_size))
        {
          // error
          return false;
        }
    }

    return true;
}


inline void CoreProtocol::flag_chunk_received(chunk_index_t idx)
{
    chunk_bitmap()[idx>>3] |= uint8_t(1<<(idx&7));
}

inline bool CoreProtocol::is_chunk_received(chunk_index_t idx)
{
    return (chunk_bitmap()[idx>>3] & uint8_t(1<<(idx&7)));
}

chunk_index_t CoreProtocol::next_chunk_missing(chunk_index_t start)
{
    chunk_index_t chunk = NO_CHUNKS_MISSING;
    chunk_index_t chunks = file.chunk_count(chunk_size);
    chunk_index_t idx = start;
    for (;idx<chunks; idx++)
    {
        if (!is_chunk_received(idx))
        {
            //TRACE("next missing chunk %d from %d", idx, start);
            chunk = idx;
            break;
        }
    }
    return chunk;
}

void CoreProtocol::set_chunks_received(uint8_t value)
{
    size_t bytes = chunk_bitmap_size();
    if (bytes)
    	memset(queue+QUEUE_SIZE-bytes, value, bytes);
}

size_t CoreProtocol::notify_update_done(uint8_t* msg, token_t token, uint8_t code)
{
    size_t msgsz = 0;
    char buf[255];
    size_t data_len = 0;

    memset(buf, 0, sizeof(buf));
    if (code != ChunkReceivedCode::BAD) {
        callbacks.finish_firmware_update(file, UpdateFlag::SUCCESS | UpdateFlag::VALIDATE_ONLY, buf);
        data_len = strnlen(buf, sizeof(buf) - 1);
    }

    if (code) {
        // Send as ACK
        msgsz = Messages::coded_ack(msg + 2, token, code, queue[2], queue[3], (uint8_t*)buf, data_len);
    } else {
        // Send as UpdateDone
        unsigned short message_id = next_message_id();
        msgsz = Messages::update_done(msg + 2, message_id, (uint8_t*)buf, data_len, false);
    }

    LOG(INFO, "Update done %02x", code);
    LOG_DEBUG(TRACE, "%s", buf);

    return wrap(msg, msgsz);
}

bool CoreProtocol::handle_update_done(msg& message)
{
    // send ACK 2.04
    uint8_t* msg_to_send = message.response;

    *msg_to_send = 0;
    *(msg_to_send + 1) = 16;
    chunk_index_t index = next_chunk_missing(0);
    bool missing = index!=NO_CHUNKS_MISSING;
    LOG(WARN,"update done: received, has missing chunks %d", missing);

    size_t response_size = notify_update_done(msg_to_send, message.token,
                                              missing ? ChunkReceivedCode::BAD : ChunkReceivedCode::OK);
    if (0 > blocking_send(msg_to_send, response_size))
    {
        // error
        return false;
    }

    if (!missing) {
        LOG(INFO,"update done: all done!");
        reset_updating();
        callbacks.finish_firmware_update(file, UpdateFlag::SUCCESS, NULL);
    }
    else {
        updating = 2;       // flag that we are sending missing chunks.
        serial_dump("update done: missing chunks starting at %d", index);
        send_missing_chunks(MISSED_CHUNKS_TO_SEND);
        last_chunk_millis = callbacks.millis();
    }
    return true;
}

bool CoreProtocol::function_result(const void* result, SparkReturnType::Enum, uint8_t token)
{
    // send return value
    queue[0] = 0;
    queue[1] = 16;
    function_return(queue + 2, token, long(result));
    if (0 > blocking_send(queue, 18))
    {
      // error
      return false;
    }
    return true;
}

bool CoreProtocol::handle_function_call(msg& message)
{
    // copy the function key
    char function_key[MAX_FUNCTION_KEY_LENGTH+1];  // add one for null terminator
    memset(function_key, 0, sizeof(function_key));

    uint8_t queue_offset = 8;
    size_t function_key_length = queue[7] & 0x0F;
    if (function_key_length == MAX_OPTION_DELTA_LENGTH+1)
    {
        function_key_length = MAX_OPTION_DELTA_LENGTH+1 + queue[8];
        queue_offset++;
    }
    // else if (function_key_length == MAX_OPTION_DELTA_LENGTH+2)
    // {
    //     // MAX_OPTION_DELTA_LENGTH+2 not supported and not required for function_key_length
    // }
    if (function_key_length > MAX_FUNCTION_KEY_LENGTH)
    {
        function_key_length = MAX_FUNCTION_KEY_LENGTH;
        // already memset to 0, so null terminator padded to end
    }
    memcpy(function_key, queue + queue_offset, function_key_length);

    // How long is the argument?
    size_t q_index = queue_offset + function_key_length;
    size_t function_arg_length = queue[q_index] & 0x0F;
    if (function_arg_length == MAX_OPTION_DELTA_LENGTH+1)
    {
        ++q_index;
        function_arg_length = MAX_OPTION_DELTA_LENGTH+1 + queue[q_index];
    }
    else if (function_arg_length == MAX_OPTION_DELTA_LENGTH+2)
    {
        ++q_index;
        function_arg_length = queue[q_index] << 8;
        ++q_index;
        function_arg_length |= queue[q_index];
        function_arg_length += 269;
    }

    bool has_function = true;
    // allocated memory bounds check
    if (function_arg_length > MAX_FUNCTION_ARG_LENGTH)
    {
        function_arg_length = MAX_FUNCTION_ARG_LENGTH;
        has_function = false;
        // in case we got here due to inconceivable error, memset with null terminators
        memset(function_arg, 0, sizeof(function_arg));
    }
    // save a copy of the argument
    memcpy(function_arg, queue + q_index + 1, function_arg_length);
    function_arg[function_arg_length] = 0; // null terminate string

    uint8_t* msg_to_send = message.response;
    // send ACK
    msg_to_send[0] = 0;
    msg_to_send[1] = 16;
    coded_ack(msg_to_send + 2, has_function ? 0x00 : RESPONSE_CODE(4,00), queue[2], queue[3]);
    if (0 > blocking_send(msg_to_send, 18))
    {
        // error
        return false;
    }

    // call the given user function
    auto callback = [=] (const void* result, SparkReturnType::Enum resultType ) { return this->function_result(result, resultType, message.token); };
    descriptor.call_function(function_key, function_arg, callback, NULL);
    return true;
}

void CoreProtocol::handle_event(msg& message)
{
    const unsigned len = message.len;

    // fist decode the event data before looking for a handler
    unsigned char pad = queue[len - 1];
    if (0 == pad || 16 < pad)
    {
        // ignore bad message, PKCS #7 padding must be 1-16
        return;
    }
    // end of CoAP message
    unsigned char *end = queue + len - pad;

    unsigned char *event_name = queue + 6;
    size_t event_name_length = CoAP::option_decode(&event_name);
    if (0 == event_name_length)
    {
        // error, malformed CoAP option
        return;
    }

    unsigned char *next_src = event_name + event_name_length;
    unsigned char *next_dst = next_src;
    while (next_src < end && 0x00 == (*next_src & 0xf0))
    {
      // there's another Uri-Path option, i.e., event name with slashes
      size_t option_len = CoAP::option_decode(&next_src);
      *next_dst++ = '/';
      if (next_dst != next_src)
      {
        // at least one extra byte has been used to encode a CoAP Uri-Path option length
        memmove(next_dst, next_src, option_len);
      }
      next_src += option_len;
      next_dst += option_len;
    }
    event_name_length = next_dst - event_name;

    if (next_src < end && 0x30 == (*next_src & 0xf0))
    {
      // Max-Age option is next, which we ignore
      size_t next_len = CoAP::option_decode(&next_src);
      next_src += next_len;
    }

    unsigned char *data = NULL;
    if (next_src < end && 0xff == *next_src)
    {
      // payload is next
      data = next_src + 1;
      // null terminate data string
      *end = 0;
    }
    // null terminate event name string
    event_name[event_name_length] = 0;

  const int NUM_HANDLERS = sizeof(event_handlers) / sizeof(FilteringEventHandler);
  for (int i = 0; i < NUM_HANDLERS; i++)
  {
    if (NULL == event_handlers[i].handler)
    {
       break;
    }
    const size_t MAX_FILTER_LENGTH = sizeof(event_handlers[i].filter);
    const size_t filter_length = strnlen(event_handlers[i].filter, MAX_FILTER_LENGTH);

    if (event_name_length < filter_length)
    {
      // does not match this filter, try the next event handler
      continue;
    }

    const int cmp = memcmp(event_handlers[i].filter, event_name, filter_length);
    if (0 == cmp)
    {
        // don't call the handler directly, use a callback for it.
        if (!this->descriptor.call_event_handler)
        {
            if(event_handlers[i].handler_data)
            {
                EventHandlerWithData handler = (EventHandlerWithData) event_handlers[i].handler;
                handler(event_handlers[i].handler_data, (char *)event_name, (char *)data);
            }
            else
            {
                event_handlers[i].handler((char *)event_name, (char *)data);
            }
        }
        else
        {
            descriptor.call_event_handler(sizeof(FilteringEventHandler), &event_handlers[i], (const char*)event_name, (const char*)data, NULL);
        }
    }
    // else continue the for loop to try the next handler
  }
}

ProtocolError CoreProtocol::post_description(int desc_flags)
{
    ProtocolError result;

    int desc_len = build_post_description(queue + 2, desc_flags);
    queue[0] = ((desc_len >> 8) & 0xff);
    queue[1] = (desc_len & 0xff);
    LOG(INFO,"Sending %s%s%s describe message", desc_flags & DESCRIBE_SYSTEM ? "S" : "",
                                                desc_flags & DESCRIBE_APPLICATION ? "A" : "",
                                                desc_flags & DESCRIBE_METRICS ? "M" : "");
    // Generate a valid return code
    if ( 0 >= blocking_send(queue, desc_len + 2) ) {
        result = particle::protocol::IO_ERROR_GENERIC_SEND;
    } else {
        result = particle::protocol::NO_ERROR;
    }

    return result;
}

bool CoreProtocol::send_description(int description_flags, msg& message)
{
    int desc_len = description(queue + 2, message.token, queue[2], queue[3], description_flags);
    queue[0] = (desc_len >> 8) & 0xff;
    queue[1] = desc_len & 0xff;
    LOG(INFO,"Sending %s%s%s describe message", description_flags & DESCRIBE_SYSTEM ? "S" : "",
                                                description_flags & DESCRIBE_APPLICATION ? "A" : "",
                                                description_flags & DESCRIBE_METRICS ? "M" : "");
    return blocking_send(queue, desc_len + 2)>=0;
}

bool CoreProtocol::handle_message(msg& message, token_t token, CoAPMessageType::Enum message_type)
{
  switch (message_type)
  {
    case CoAPMessageType::DESCRIBE:
    {
        int desc_flags = DESCRIBE_DEFAULT;
        if (message_padding_strip(queue, message.len) > 8 && queue[8] <= DESCRIBE_MAX) {
            desc_flags = queue[8];
        }

        if (!send_description(desc_flags, message)) {
            return false;
        }
        break;
    }
    case CoAPMessageType::FUNCTION_CALL:
        if (!handle_function_call(message))
            return false;
        break;
    case CoAPMessageType::VARIABLE_REQUEST:
    {
      // copy the variable key
      int queue_offset = 8;
      size_t variable_key_length = queue[7] & 0x0F;
      if (variable_key_length == MAX_OPTION_DELTA_LENGTH+1) {
          variable_key_length = MAX_OPTION_DELTA_LENGTH+1 + queue[8];
          queue_offset++;
      }
      if (variable_key_length > MAX_VARIABLE_KEY_LENGTH) {
          variable_key_length = MAX_VARIABLE_KEY_LENGTH;
      }

      char variable_key[MAX_VARIABLE_KEY_LENGTH+1];
      memcpy(variable_key, queue + queue_offset, variable_key_length);
      memset(variable_key + variable_key_length, 0, MAX_VARIABLE_KEY_LENGTH+1 - variable_key_length);

      queue[0] = 0;
      queue[1] = 16; // default buffer length

      // get variable value according to type using the descriptor
      SparkReturnType::Enum var_type = descriptor.variable_type(variable_key);
      if(SparkReturnType::BOOLEAN == var_type)
      {
        bool *bool_val = (bool *)descriptor.get_variable(variable_key);
        variable_value(queue + 2, token, queue[2], queue[3], *bool_val);
      }
      else if(SparkReturnType::INT == var_type)
      {
        int *int_val = (int *)descriptor.get_variable(variable_key);
        variable_value(queue + 2, token, queue[2], queue[3], *int_val);
      }
      else if(SparkReturnType::STRING == var_type)
      {
        char *str_val = (char *)descriptor.get_variable(variable_key);

        // 2-byte leading length, 16 potential padding bytes
        int max_length = QUEUE_SIZE - 2 - 16;
        int str_length = strlen(str_val);
        if (str_length > max_length) {
          str_length = max_length;
        }

        int buf_size = variable_value(queue + 2, token, queue[2], queue[3], str_val, str_length);
        queue[1] = buf_size & 0xff;
        queue[0] = (buf_size >> 8) & 0xff;
      }
      else if(SparkReturnType::DOUBLE == var_type)
      {
        double *double_val = (double *)descriptor.get_variable(variable_key);
        variable_value(queue + 2, token, queue[2], queue[3], *double_val);
      }

      // buffer length may have changed if variable is a long string
      if (0 > blocking_send(queue, (queue[0] << 8) + queue[1] + 2))
      {
        // error
        return false;
      }
      break;
    }

    case CoAPMessageType::SAVE_BEGIN:
      // fall through
    case CoAPMessageType::UPDATE_BEGIN:
        return handle_update_begin(message);

    case CoAPMessageType::CHUNK:
        return handle_chunk(message);

    case CoAPMessageType::UPDATE_DONE:
        return handle_update_done(message);

    case CoAPMessageType::EVENT:
        handle_event(message);
          break;
    case CoAPMessageType::KEY_CHANGE:
      // TODO
      break;
    case CoAPMessageType::SIGNAL_START:
      queue[0] = 0;
      queue[1] = 16;
      coded_ack(queue + 2, token, ChunkReceivedCode::OK, queue[2], queue[3]);
      if (0 > blocking_send(queue, 18))
      {
        // error
        return false;
      }

      callbacks.signal(true, 0, NULL);
      break;
    case CoAPMessageType::SIGNAL_STOP:
      queue[0] = 0;
      queue[1] = 16;
      coded_ack(queue + 2, token, ChunkReceivedCode::OK, queue[2], queue[3]);
      if (0 > blocking_send(queue, 18))
      {
        // error
        return false;
      }

      callbacks.signal(false, 0, NULL);
      break;

    case CoAPMessageType::HELLO:
      descriptor.ota_upgrade_status_sent();
      break;

    case CoAPMessageType::TIME:
      handle_time_response(queue[6] << 24 | queue[7] << 16 | queue[8] << 8 | queue[9]);
      break;

    case CoAPMessageType::PING:
      queue[0] = 0;
      queue[1] = 16;
      empty_ack(queue + 2, queue[2], queue[3]);
      if (0 > blocking_send(queue, 18))
      {
        // error
        return false;
      }
      break;

    case CoAPMessageType::EMPTY_ACK:
      LOG_DEBUG_C(TRACE, "comm.coap", "received ACK for message: %x", (unsigned)message.id);
      ack_handlers.setResult(message.id);
      break;

    case CoAPMessageType::ERROR:
    default:
      ; // drop it on the floor
  }

  // all's well
  return true;
}

CoAPMessageType::Enum CoreProtocol::handle_received_message(void)
{
  last_message_millis = callbacks.millis();
  expecting_ping_ack = false;
  size_t len = queue[0] << 8 | queue[1];
  if (len > QUEUE_SIZE) { // TODO add sanity check on data, e.g. CRC
      return CoAPMessageType::ERROR;
  }
  if (0 > blocking_receive(queue, len))
  {
    // error
    return CoAPMessageType::ERROR;;
  }
  CoAPMessageType::Enum message_type = received_message(queue, len);

  unsigned char token = queue[4];
  unsigned char *msg_to_send = queue + len;

  const uint16_t msg_id = (uint16_t)queue[2] << 8 | (uint16_t)queue[3];

  msg message;
  message.len = len;
  message.token = queue[4];
  message.response = msg_to_send;
  message.response_len = QUEUE_SIZE-len;
  message.id = msg_id;

  return handle_message(message, token, message_type)
          ? message_type : CoAPMessageType::ERROR;
}

void CoreProtocol::handle_time_response(uint32_t time)
{
    // deduct latency
    uint32_t latency = last_chunk_millis ? (callbacks.millis()-last_chunk_millis)/2000 : 0;
    last_chunk_millis = 0;
    timesync_.handle_time_response(time - latency, callbacks.millis(), callbacks.set_time);
}

unsigned short CoreProtocol::next_message_id()
{
  return ++_message_id;
}

unsigned char CoreProtocol::next_token()
{
  return ++_token;
}

void CoreProtocol::encrypt(unsigned char *buf, int length)
{
#ifdef USE_MBEDTLS
  mbedtls_aes_setkey_enc(&aes, key, 128);
  mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT, round_to_16(length), iv_send, buf, buf);
#else
  aes_setkey_enc(&aes, key, 128);
  aes_crypt_cbc(&aes, AES_ENCRYPT, length, iv_send, buf, buf);
#endif
  memcpy(iv_send, buf, 16);
}

void CoreProtocol::separate_response(unsigned char *buf,
                                      unsigned char token,
                                      unsigned char code)
{
    separate_response_with_payload(buf, token, code, NULL, 0);
}

void CoreProtocol::separate_response_with_payload(unsigned char *buf,
                                      unsigned char token,
                                      unsigned char code,
                                      unsigned char* payload,
                                      unsigned payload_len)
{
  unsigned short message_id = next_message_id();

  buf[0] = 0x51; // non-confirmable, one-byte token
  buf[1] = code;
  buf[2] = message_id >> 8;
  buf[3] = message_id & 0xff;
  buf[4] = token;

  unsigned len = 5;
  // for now, assume the payload is less than 9
  if (payload && payload_len) {
      buf[5] = 0xFF;
      memcpy(buf+6, payload, payload_len);
      len += 1 + payload_len;
  }

  memset(buf + len, 16-len, 16-len); // PKCS #7 padding

  encrypt(buf, 16);
}

int CoreProtocol::set_key(const unsigned char *signed_encrypted_credentials)
{
  unsigned char credentials[128];
  unsigned char hmac[20];

  if (0 != decipher_aes_credentials(core_private_key,
                                    signed_encrypted_credentials,
                                    credentials))
    return DECRYPTION_ERROR;

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
    if (handlers.random_seed_from_cloud)
        handlers.random_seed_from_cloud(seed);
    else
        default_random_seed_from_cloud(seed);

    return 0;
  }
  else return AUTHENTICATION_ERROR;
}

inline void CoreProtocol::coded_ack(unsigned char *buf,
                                     unsigned char code,
                                     unsigned char message_id_msb,
                                     unsigned char message_id_lsb
                                     )
{
  buf[0] = 0x60; // acknowledgment, no token
  buf[1] = code;
  buf[2] = message_id_msb;
  buf[3] = message_id_lsb;

  memset(buf + 4, 12, 12); // PKCS #7 padding

  encrypt(buf, 16);
}

inline void CoreProtocol::coded_ack(unsigned char *buf,
                                     unsigned char token,
                                     unsigned char code,
                                     unsigned char message_id_msb,
                                     unsigned char message_id_lsb)
{
  buf[0] = 0x61; // acknowledgment, one-byte token
  buf[1] = code;
  buf[2] = message_id_msb;
  buf[3] = message_id_lsb;
  buf[4] = token;

  memset(buf + 5, 11, 11); // PKCS #7 padding

  encrypt(buf, 16);
}

int CoreProtocol::command(ProtocolCommands::Enum command, uint32_t data)
{
  int result = UNKNOWN;
  switch (command) {
  case ProtocolCommands::SLEEP:
  case ProtocolCommands::DISCONNECT:
    result = !this->wait_confirmable() ? NO_ERROR : UNKNOWN;
    break;
  case ProtocolCommands::TERMINATE:
    ack_handlers.clear();
    result = NO_ERROR;
    break;
  }
  return result;
}

int CoreProtocol::wait_confirmable(uint32_t timeout)
{
  bool st = true;

  if (ack_handlers.size() != 0) {
    system_tick_t start = callbacks.millis();
    LOG(INFO, "Waiting for Confirmed messages to be ACKed.");

    while (((ack_handlers.size() != 0) && (callbacks.millis()-start)<timeout))
    {
      CoAPMessageType::Enum message;
      st = event_loop(message);
      if (!st)
      {
        LOG(WARN, "error receiving acknowledgements");
        break;
      }
    }
    LOG(INFO, "All Confirmed messages sent: %s",
        (ack_handlers.size() != 0) ? "no" : "yes");

    ack_handlers.clear();
  }

  return (int)(!st);
}

#endif // !PARTICLE_PROTOCOL
