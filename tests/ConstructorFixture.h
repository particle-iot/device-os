/**
  ******************************************************************************
  * @file    ConstructorFixture.h
  * @authors  Zachary Crockett
  * @version V1.0.0
  * @date    10-Jan-2014
  * @brief   Fixture for testing SparkProtocol
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

#ifndef __CONSTRUCTOR_FIXTURE_H
#define __CONSTRUCTOR_FIXTURE_H

#include <stdint.h>
#include "spark_protocol.h"

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
  static int mock_send(const unsigned char *buf, int buflen);
  static int mock_receive(unsigned char *buf, int buflen);
  static uint8_t message_to_receive[34];
  static bool function_called;
  static int mock_num_functions(void);
  static void mock_copy_function_key(char *destination, int function_index);
  static int mock_call_function(const char *function_key, const char *arg);
  static int mock_num_variables(void);
  static void mock_copy_variable_key(char *destination, int variable_index);
  static void *mock_get_variable(const char *variable_key);
  static void mock_signal(bool on);
  static bool signal_called_with;
  static int variable_to_get;
  static unsigned int mock_millis(void);
  static bool mock_ota_status_check(void);
  static SparkReturnType::Enum mock_variable_type(const char *variable_key);

  ConstructorFixture();
  SparkKeys keys;
  SparkCallbacks callbacks;
  SparkDescriptor descriptor;
  SparkProtocol spark_protocol;
};

#endif // __CONSTRUCTOR_FIXTURE_H
