/**
  ******************************************************************************
  * @file    core_protocol.h
  * @authors  Zachary Crockett, Matthew McGowan
  * @version V1.0.0
  * @date    15-Nov-2013
  * @brief   CORE PROTOCOL
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

#ifndef __CORE_PROTOCOL_H
#define __CORE_PROTOCOL_H

#include "protocol_selector.h"

#if !PARTICLE_PROTOCOL

#include "protocol_defs.h"
#include "spark_descriptor.h"
#include "coap.h"
#include "events.h"
#ifdef USE_MBEDTLS
#include "mbedtls/rsa.h"
#include "mbedtls/aes.h"
#define aes_context mbedtls_aes_context
#else
# if PLATFORM_ID == 6 || PLATFORM_ID == 8
#  include "wiced_security.h"
#  include "crypto_open/bignum.h"
#  define aes_context aes_context_t
# else
#  include "tropicssl/rsa.h"
#  include "tropicssl/aes.h"
# endif
#endif
#include "device_keys.h"
#include "file_transfer.h"
#include "spark_protocol_functions.h"
#include "timesyncmanager.h"
#include <stdint.h>

using namespace particle::protocol;
using particle::CompletionHandler;
using particle::CompletionHandlerMap;

#if !defined(arraySize)
#   define arraySize(a)            (sizeof((a))/sizeof((a[0])))
#endif


namespace ProtocolState {
  enum Enum {
    READ_NONCE
  };
}

/*
 * @brief Protocol implementation for the Spark Core
 *
 * @warning This protocol is deprecated
 * @note This protocol shall not be extended. All new APIs
 * requiring parity shall return a ProtocolError of NOT_IMPLEMENTED
 */
class CoreProtocol
{
  public:
    static int presence_announcement(unsigned char *buf, const unsigned char *id);

    CoreProtocol();

    void init(const char *id,
              const SparkKeys &keys,
              const SparkCallbacks &callbacks,
              const SparkDescriptor &descriptor);
    int handshake(void);

    /**
     * Wait for a given message type until the timeout.
     * @return true if the message was processed within the timeout.
     */
    bool event_loop(CoAPMessageType::Enum msg, system_tick_t timeout);
    bool event_loop(CoAPMessageType::Enum& msg);
    bool is_initialized(void);
    void reset_updating(void);

    void set_product_id(product_id_t product_id) { if (product_id!=UNDEFINED_PRODUCT_ID) this->product_id=product_id; }
    void set_product_firmware_version(product_firmware_version_t version) { if (version!=UNDEFINED_PRODUCT_VERSION) this->product_firmware_version = version; }
    void get_product_details(product_details_t& details) {
        if (details.size>=4) {
            details.product_id = this->product_id;
            details.product_version = this->product_firmware_version;
        }
    }
    int set_key(const unsigned char *signed_encrypted_credentials);
    int blocking_send(const unsigned char *buf, int length);
    int blocking_receive(unsigned char *buf, int length);

    CoAPMessageType::Enum received_message(unsigned char *buf, size_t length);
    void hello(unsigned char *buf, bool newly_upgraded);
    void key_changed(unsigned char *buf, unsigned char token);
    void function_return(unsigned char *buf, unsigned char token,
                         int return_value);
    void variable_value(unsigned char *buf, unsigned char token,
                        unsigned char message_id_msb, unsigned char message_id_lsb,
                        bool return_value);
    void variable_value(unsigned char *buf, unsigned char token,
                        unsigned char message_id_msb, unsigned char message_id_lsb,
                        int return_value);
    void variable_value(unsigned char *buf, unsigned char token,
                        unsigned char message_id_msb, unsigned char message_id_lsb,
                        double return_value);
    int variable_value(unsigned char *buf, unsigned char token,
                       unsigned char message_id_msb, unsigned char message_id_lsb,
                       const void *return_value, int length);
    bool send_event(const char *event_name, const char *data,
                    int ttl, EventType::Enum event_type, int flags, CompletionHandler handler);

    bool add_event_handler(const char *event_name, EventHandler handler) {
        return add_event_handler(event_name, handler, NULL, SubscriptionScope::FIREHOSE, NULL);
    }
    bool add_event_handler(const char *event_name, EventHandler handler,
                        void *handler_data, SubscriptionScope::Enum scope,
                        const char* device_id);
    bool event_handler_exists(const char *event_name, EventHandler handler,
        void *handler_data, SubscriptionScope::Enum scope, const char* id);
    void remove_event_handlers(const char* event_name);
    void send_subscriptions();
    bool send_subscription(const char *event_name, const char *device_id);
    bool send_subscription(const char *event_name, SubscriptionScope::Enum scope);
    size_t time_request(unsigned char *buf);
    bool send_time_request(void);
    bool time_request_pending() const { return timesync_.is_request_pending(); }
    system_tick_t time_last_synced(time_t* tm) const { return timesync_.last_sync(*tm); }
    void chunk_received(unsigned char *buf, unsigned char token,
                        ChunkReceivedCode::Enum code);
    void chunk_missed(unsigned char *buf, unsigned short chunk_index);
    void update_ready(unsigned char *buf, unsigned char token);
    void update_ready(unsigned char *buf, unsigned char token, uint8_t flags);

    /**
     * @brief Constructs the body of a describe message
     *
     * @param buf The buffer for the message
     * @param offset The offset to start the body of the message
     *               (typically, beyond the header)
     * @param desc_flags The information description flags
     * @arg \p DESCRIBE_APPLICATION
     * @arg \p DESCRIBE_METRICS
     * @arg \p DESCRIBE_SYSTEM
     *
     * @returns The buffer length of the message
     * @retval \p -1 - failure
     */
    int build_describe_message(unsigned char *buf, unsigned char offset, int desc_flags);

    /**
     * @brief Constructs a describe message (POST request)
     *
     * @param buf The buffer for the message
     * @param desc_flags The information description flags
     * @arg \p DESCRIBE_APPLICATION
     * @arg \p DESCRIBE_METRICS
     * @arg \p DESCRIBE_SYSTEM
     *
     * @returns The buffer length of the message
     * @retval \p -1 - failure
     */
    int build_post_description(unsigned char *buf, int desc_flags);
    int description(unsigned char *buf, unsigned char token,
                    unsigned char message_id_msb, unsigned char message_id_lsb, int desc_flags);

    /**
     * @brief Produces and transmits a describe message (POST request)
     *
     * @param desc_flags Flags describing the information to provide
     * @arg \p DESCRIBE_APPLICATION
     * @arg \p DESCRIBE_METRICS
     * @arg \p DESCRIBE_SYSTEM
     *
     * @returns \s ProtocolError result value
     * @retval \p particle::protocol::NO_ERROR
     * @retval \p particle::protocol::IO_ERROR_GENERIC_SEND
     *
     * @sa particle::protocol::ProtocolError
     */
    ProtocolError post_description(int desc_flags);

    void ping(unsigned char *buf);
    bool function_result(const void* result, SparkReturnType::Enum resultType, uint8_t token);

    /********** Queue **********/
    const size_t QUEUE_SIZE;
#if 0
    int queue_bytes_available();
    int queue_push(const char *src, int length);
    int queue_pop(char *dst, int length);
#endif
    void set_handlers(CommunicationsHandlers& handlers) {
        this->handlers = handlers;
    }

    int command(ProtocolCommands::Enum command, uint32_t data);
    int wait_confirmable(uint32_t timeout=5000);

    /********** State Machine **********/
    ProtocolState::Enum state();

  private:
    struct msg {
        uint8_t token;
        size_t len;
        uint8_t* response;
        size_t response_len;
        uint16_t id;
    };

    CommunicationsHandlers handlers;   // application callbacks
    char device_id[12];
    unsigned char server_public_key[MAX_SERVER_PUBLIC_KEY_LENGTH];
    unsigned char core_private_key[MAX_DEVICE_PRIVATE_KEY_LENGTH];
    aes_context aes;

    FilteringEventHandler event_handlers[MAX_SUBSCRIPTIONS];
    SparkCallbacks callbacks;
    SparkDescriptor descriptor;

    CompletionHandlerMap<uint16_t> ack_handlers;
    system_tick_t last_ack_handlers_update;

    static const unsigned SEND_EVENT_ACK_TIMEOUT = 20000;

    unsigned char key[16];
    unsigned char iv_send[16];
    unsigned char iv_receive[16];
    unsigned char salt[8];
    unsigned short _message_id;
    unsigned char _token;
    system_tick_t last_message_millis;
    system_tick_t last_chunk_millis;    // NB: also used to synchronize time
    unsigned short chunk_index;
    unsigned short chunk_size;
    bool expecting_ping_ack;
    bool initialized;
    uint8_t updating;
    char function_arg[MAX_FUNCTION_ARG_LENGTH+1]; // add one for null terminator

    size_t wrap(unsigned char *buf, size_t msglen);
    CoAPMessageType::Enum handle_received_message(void);
    bool handle_message(msg& message, token_t token, CoAPMessageType::Enum message_type);

    bool handle_function_call(msg& message);
    void handle_event(msg& message);
    unsigned short next_message_id();
    unsigned char next_token();
    void encrypt(unsigned char *buf, int length);
    void separate_response(unsigned char *buf, unsigned char token, unsigned char code);
    void separate_response_with_payload(unsigned char *buf, unsigned char token,
        unsigned char code, unsigned char* payload, unsigned payload_len);

    void coded_ack(unsigned char *buf,
                      unsigned char code,
                      unsigned char message_id_msb,
                      unsigned char message_id_lsb);

    void empty_ack(unsigned char *buf,
                          unsigned char message_id_msb,
                          unsigned char message_id_lsb) {
        coded_ack(buf, 0, message_id_msb, message_id_lsb);
    };


    void coded_ack(unsigned char *buf,
                          unsigned char token,
                          unsigned char code,
                          unsigned char message_id_msb,
                          unsigned char message_id_lsb);

    bool handle_update_begin(msg& m);
    bool handle_chunk(msg& m);
    bool handle_update_done(msg& m);
    void handle_time_response(uint32_t time);

    /********** Queue **********/
    unsigned char queue[PROTOCOL_BUFFER_SIZE];
    product_id_t product_id;
    product_firmware_version_t product_firmware_version;
    FileTransfer::Descriptor file;
    inline void queue_init()
    {
    }

    unsigned chunk_bitmap_size()
    {
        return (file.chunk_count(chunk_size)+7)/8;
    }

    uint8_t* chunk_bitmap()
    {
        return &queue[QUEUE_SIZE-chunk_bitmap_size()];
    }

    void set_chunks_received(uint8_t value);
    bool is_chunk_received(chunk_index_t idx);
    void flag_chunk_received(chunk_index_t index);
    chunk_index_t next_chunk_missing(chunk_index_t index);
    int send_missing_chunks(int count);
    size_t notify_update_done(uint8_t* msg, token_t token, uint8_t code);

    /**
     * Send a particular type of describe message.
     * @param description_flags A combination of DescriptionType enum values.
     * @param message
     * @return true on success
     */
    bool send_description(int description_flags, msg& message);

    /**
     * Marks the indices of missed chunks not yet requested.
     */
    chunk_index_t missed_chunk_index;

    TimeSyncManager timesync_;
};

#ifdef USE_MBEDTLS
#undef aes_context
#endif

#endif // !PARTICLE_PROTOCOL

#endif // __CORE_PROTOCOL_H
