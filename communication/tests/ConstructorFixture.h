/**
  ******************************************************************************
  * @file    ConstructorFixture.h
  * @authors  Zachary Crockett
  * @version V1.0.0
  * @date    10-Jan-2014
  * @brief   Fixture for testing CoreProtocol
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

#ifndef __CONSTRUCTOR_FIXTURE_H
#define __CONSTRUCTOR_FIXTURE_H

#include <stdint.h>
#include "core_protocol.h"

struct EventHandlerCalledWith
{
  char event_name[64];
  char data[64];
};

struct ConstructorFixture
{
  static const uint8_t nonce[41];
  static const char id[13];
  static uint8_t pubkey[295];
  static uint8_t private_key[613];
  static const uint8_t signed_encrypted_credentials[385];
  static int bytes_sent[2];
  static int bytes_received[2];
  static uint8_t sent_buf_0[256];
  static uint8_t sent_buf_1[256];
  static int mock_send(const unsigned char *buf, uint32_t buflen);
  static int mock_receive(unsigned char *buf, uint32_t buflen);
  static uint8_t message_to_receive[98];
  static int mock_prepare_for_firmware_update(FileTransfer::Descriptor& data, uint32_t flags, void*);
  static uint32_t mock_crc;
  static uint32_t mock_calculate_crc(const unsigned char *buf, uint32_t buflen);
  static unsigned short next_chunk_index;
  static int mock_save_firmware_chunk(FileTransfer::Descriptor& desc, const unsigned char *buf, void*);
  static bool did_prepare_for_update;
  static bool did_finish_update;
  static bool nothing_to_receive;
  static uint8_t saved_firmware_chunk[72];
  static int mock_finish_firmware_update(FileTransfer::Descriptor& data, uint32_t flags, void*);
  static bool function_called;
  static int mock_num_functions(void);
  static void mock_copy_function_key(char *destination, int function_index);
  static int mock_call_function(const char *function_key, const char *arg, SparkDescriptor::FunctionResultCallback callback, void* reserved);
  static int mock_num_variables(void);
  static void mock_copy_variable_key(char *destination, int variable_index);
  static const void *mock_get_variable(const char *variable_key);
  static void mock_signal(bool on, unsigned int param, void* reserved);
  static bool signal_called_with;
  static int variable_to_get;
  static system_tick_t next_millis;
  static system_tick_t mock_millis(void);
  static bool mock_ota_status_check(void);
  static SparkReturnType::Enum mock_variable_type(const char *variable_key);
  static void mock_set_time(time_t t, unsigned int param, void* reserved);
  static time_t set_time_called_with;
  static EventHandlerCalledWith event_handlers_called_with[2];
  static void mock_event_handler_0(const char *event_name, const char *data);
  static void mock_event_handler_1(const char *event_name, const char *data);

  ConstructorFixture();
  SparkKeys keys;
  SparkCallbacks callbacks;
  SparkDescriptor descriptor;
  CoreProtocol core_protocol;
};

#endif // __CONSTRUCTOR_FIXTURE_H
