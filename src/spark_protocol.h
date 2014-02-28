/**
  ******************************************************************************
  * @file    spark_protocol.h
  * @authors  Zachary Crockett
  * @version V1.0.0
  * @date    15-Nov-2013
  * @brief   SPARK PROTOCOL
  ******************************************************************************
  Copyright (c) 2013 Spark Labs, Inc.  All rights reserved.

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

#ifndef __SPARK_PROTOCOL_H
#define __SPARK_PROTOCOL_H

#include "spark_descriptor.h"
#include "coap.h"
#include "events.h"
#include "tropicssl/rsa.h"
#include "tropicssl/aes.h"

#if !defined(arraySize)
#   define arraySize(a)            (sizeof((a))/sizeof((a[0])))
#endif

typedef unsigned long system_tick_t; // This needs to match the definition of millis

namespace ProtocolState {
  enum Enum {
    READ_NONCE
  };
}

namespace ChunkReceivedCode {
  enum Enum {
    OK = 0x44,
    BAD = 0x80
  };
}

struct SparkKeys
{
  unsigned char *core_private;
  unsigned char *server_public;
};

struct SparkCallbacks
{
  int (*send)(const unsigned char *buf, int buflen);
  int (*receive)(unsigned char *buf, int buflen);
  void (*prepare_for_firmware_update)(void);
  void (*finish_firmware_update)(void);
  long unsigned int (*calculate_crc)(unsigned char *buf, long unsigned int buflen);
  void (*save_firmware_chunk)(unsigned char *buf, long unsigned int buflen);
  void (*signal)(bool on);
  system_tick_t (*millis)();
};

class SparkProtocol
{
  public:
    static const int MAX_FUNCTION_ARG_LENGTH = 64;
    static const int MAX_FUNCTION_KEY_LENGTH = 12;
    static const int MAX_VARIABLE_KEY_LENGTH = 12;
    static const int MAX_EVENT_NAME_LENGTH = 64;
    static const int MAX_EVENT_DATA_LENGTH = 64;
    static const int MAX_EVENT_TTL_SECONDS = 16777215;
    static int presence_announcement(unsigned char *buf, const char *id);

    SparkProtocol();

    void init(const char *id,
              const SparkKeys &keys,
              const SparkCallbacks &callbacks,
              const SparkDescriptor &descriptor);
    int handshake(void);
    bool event_loop(void);
    bool is_initialized(void);
    void reset_updating(void);

    int set_key(const unsigned char *signed_encrypted_credentials);
    int blocking_send(const unsigned char *buf, int length);
    int blocking_receive(unsigned char *buf, int length);

    CoAPMessageType::Enum received_message(unsigned char *buf, int length);
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
                    int ttl, EventType::Enum event_type);
    void chunk_received(unsigned char *buf, unsigned char token,
                        ChunkReceivedCode::Enum code);
    void chunk_missed(unsigned char *buf, unsigned short chunk_index);
    void update_ready(unsigned char *buf, unsigned char token);
    int description(unsigned char *buf, unsigned char token,
                    unsigned char message_id_msb, unsigned char message_id_lsb);
    void ping(unsigned char *buf);

    /********** Queue **********/
    const int QUEUE_SIZE;
    int queue_bytes_available();
    int queue_push(const char *src, int length);
    int queue_pop(char *dst, int length);

    /********** State Machine **********/
    ProtocolState::Enum state();

  private:
    char device_id[12];
    unsigned char server_public_key[294];
    unsigned char core_private_key[612];
    aes_context aes;

    int (*callback_send)(const unsigned char *buf, int buflen);
    int (*callback_receive)(unsigned char *buf, int buflen);
    void (*callback_prepare_for_firmware_update)(void);
    void (*callback_finish_firmware_update)(void);
    long unsigned int (*callback_calculate_crc)(unsigned char *buf, long unsigned int buflen);
    void (*callback_save_firmware_chunk)(unsigned char *buf, long unsigned int buflen);
    void (*callback_signal)(bool on);
    system_tick_t (*callback_millis)();

    SparkDescriptor descriptor;

    unsigned char key[16];
    unsigned char iv_send[16];
    unsigned char iv_receive[16];
    unsigned char salt[8];
    unsigned short _message_id;
    system_tick_t last_message_millis;
    system_tick_t last_chunk_millis;
    unsigned short chunk_index;
    bool expecting_ping_ack;
    bool initialized;
    bool updating;
    char function_arg[MAX_FUNCTION_ARG_LENGTH];

    bool handle_received_message(void);
    unsigned short next_message_id();
    void encrypt(unsigned char *buf, int length);
    void separate_response(unsigned char *buf, unsigned char token, unsigned char code);
    inline void empty_ack(unsigned char *buf,
                          unsigned char message_id_msb,
                          unsigned char message_id_lsb);
    inline void coded_ack(unsigned char *buf,
                          unsigned char token,
                          unsigned char code,
                          unsigned char message_id_msb,
                          unsigned char message_id_lsb);

    /********** Queue **********/
    unsigned char queue[640];
    const unsigned char *queue_mem_boundary;
    unsigned char *queue_front;
    unsigned char *queue_back;
    void queue_init(void);
};

#endif // __SPARK_PROTOCOL_H
