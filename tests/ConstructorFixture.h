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
