#include "UnitTest++.h"
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

const uint8_t ConstructorFixture::nonce[41] =
  "\x31\xE8\x30\x24\x6F\x2D\x7D\x98\x7C\x42\x47\x7E\xF0\x33\xF4\x24"
  "\xFF\x62\xD3\x82\xB1\x7A\x09\x31\x13\x0B\x23\x63\x98\xDE\x90\x84"
  "\x71\x41\xF5\x83\x04\x84\x17\x7B";

const char ConstructorFixture::id[13] =
  "\x54\xE1\xC8\x88\xF6\xD9\x49\x2B\xEB\xEE\x1E\xE9";

uint8_t ConstructorFixture::pubkey[295] =
  "\x30\x82\x01\x22\x30\x0D\x06\x09\x2A\x86\x48\x86\xF7\x0D\x01\x01"
  "\x01\x05\x00\x03\x82\x01\x0F\x00\x30\x82\x01\x0A\x02\x82\x01\x01"
  "\x00\xA4\x4B\x8F\x50\xBF\xD7\x94\x77\xF6\xC9\xBC\xEB\x1A\x00\xF3"
  "\x1D\x31\x51\xA8\xE0\xB0\xD4\x0F\x3C\xFF\x49\x85\x71\xBA\xFA\x54"
  "\x80\x9C\x91\x3D\x24\xD8\x9A\x4F\x99\x64\x30\xFC\xB5\x96\x44\xB1"
  "\x24\x8A\xA8\xD2\xC1\xBE\xEA\x3D\x95\x9B\x2F\xB2\x0F\x1C\x9D\xF7"
  "\x26\x51\xE9\x74\x7B\x8E\x7B\x3A\xEF\xF5\x47\x83\xC9\x71\x85\xEF"
  "\x3C\x51\x10\x35\x40\xA5\x79\x61\xFB\x21\x60\x1E\xDB\xCC\xA3\xE7"
  "\x98\x18\xA5\x61\x4E\x7C\xB2\x91\xB9\x92\xA7\x81\x5C\x49\x35\xF2"
  "\x0B\x23\x71\xCA\xFE\x10\x4D\x9D\x50\x04\xD8\xF1\x0F\x19\xD8\xC3"
  "\x7A\x63\x9D\xF5\x22\x23\x67\x09\x12\xDC\x8D\xC9\x0F\x7F\xCC\xD4"
  "\x52\x64\x96\xCF\x7A\x2C\x76\x32\x38\xCA\x9B\x7A\xC7\xD4\x27\x0F"
  "\x3F\xD1\xFB\x8A\x62\x04\x8B\xB7\x03\x25\x18\xCB\xF4\x3B\x0A\x90"
  "\x50\x2A\x5E\xBE\x1F\xC8\x36\x3E\x8F\x79\xD2\xB3\xDA\xE1\x44\xE3"
  "\x09\xF7\x12\x17\x49\x00\xC9\x38\x8C\xA3\xFF\xDD\x6A\xD1\x43\xB8"
  "\x05\xF8\x6A\x4A\xB6\xE0\x19\x2A\x02\x45\x92\x6F\xF9\x61\xB7\xE8"
  "\x39\x17\x05\x19\x14\x28\xB3\x8E\x4F\x63\xA5\x7F\x87\x7A\xA7\x62"
  "\x6B\x7A\x8C\xFD\xD3\x10\xED\x9E\xAB\x8B\xC5\xA1\x28\xB6\x17\x8E"
  "\x3D\x02\x03\x01\x00\x01";

uint8_t ConstructorFixture::private_key[613] =
  "\x30\x82\x02\x5E\x02\x01\x00\x02\x81\x81\x00\xC4\xC8\xEB\xFA\x99"
  "\xA5\xD1\xE5\xF9\x9D\x33\xEA\x1C\x93\xF2\x4A\x71\xC7\x1E\xA0\x1E"
  "\xE6\x71\x87\x39\x5E\x5F\x69\x56\x4F\x76\xC1\x83\x61\x10\xEA\x78"
  "\x69\x6E\x5A\xA2\x4D\x5E\x83\x4E\x41\xD0\xE5\x44\xBC\x48\x5F\x7D"
  "\x85\x65\x24\xB0\x9C\x9C\x3C\xD0\x0F\x42\x6A\x6D\x46\x51\x9C\x3E"
  "\xDC\x88\x33\x84\xC5\xF4\x6D\xAD\x89\xFD\x01\xDC\x2B\x3F\xB0\x6F"
  "\x12\x80\xEC\xE2\xD9\x53\x00\x66\x93\x58\x3C\x0B\x15\x66\xEA\x47"
  "\xD9\xDD\x8F\x49\xEE\xD7\x1A\x81\xBA\xE6\x58\x5C\x63\x7A\xDD\xC5"
  "\x11\xF1\xD2\xCE\x8C\x01\x60\xAD\xF3\xB4\x5F\x02\x03\x01\x00\x01"
  "\x02\x81\x81\x00\xBB\xC5\x58\xDE\xF4\x13\xB4\xF8\xB3\xB9\x5C\x5B"
  "\x2C\xCF\xC3\x27\x63\xEF\xF3\x7A\x28\x62\x0D\xBC\x51\x72\x8A\xAA"
  "\x51\xD0\x5B\x6A\x05\x79\xEE\x91\x3D\x3A\xA5\x31\x58\xA3\x68\xE6"
  "\xF4\x1A\x7B\x40\xF9\xD8\x8B\x5A\x8A\xC4\x69\xA1\x9B\xE0\xA4\x78"
  "\xA6\xB3\x98\xD3\x96\x20\x7B\xE4\x93\x9A\x0E\xFE\xD9\x44\x54\x6C"
  "\xCF\x2D\x5E\x9B\x91\x94\x58\x90\x30\xAC\x08\xA5\xE1\x8E\x5F\x84"
  "\xC3\x36\xD0\xCD\x0F\x10\xBF\x05\x6E\x29\x27\x8A\x16\x7A\xC6\xC2"
  "\x78\xCF\x2C\xBC\x5E\x5B\x00\x38\x3E\x66\xBF\x12\x2B\x20\x17\x8E"
  "\xE7\xE2\x7A\x09\x02\x41\x00\xEA\x0B\x61\xD6\x8D\x8C\xAD\x1C\xFC"
  "\x0A\x6F\x37\x69\x3A\xD7\x9F\x3C\x4E\xFF\xFA\x97\x72\x5C\x31\x36"
  "\x1F\x12\x23\x4B\x00\x29\x70\x82\x5F\x3F\xBF\x98\xE3\x35\x24\xD2"
  "\x3F\xE6\x88\x9D\xA6\x72\xE3\x4A\x09\xEA\xCA\xF2\x42\xCE\x8B\xB6"
  "\x18\x04\xAC\x01\x73\x4C\xCD\x02\x41\x00\xD7\x3E\xBE\x61\xA3\xF0"
  "\x75\x2B\xE4\xDD\x60\x67\xA6\x9E\x6A\xDF\x41\xB1\x71\xC9\x54\xDA"
  "\xF1\xB6\xAC\xEB\x3E\x12\x3C\xA8\x6C\xCB\x75\xFC\xDA\xE5\x69\xBF"
  "\xB1\x61\x4F\x4F\xD0\x32\x21\xF8\x52\x27\x1C\x59\x69\xBE\x3E\xB3"
  "\xF3\x16\x41\xBC\xAF\x3A\x6F\x15\x05\xDB\x02\x40\x5A\x18\xE9\xA0"
  "\x1B\xBB\xB5\x04\xBC\x6E\x13\xE4\x63\xE9\x18\x0A\x9F\xBF\xD5\xC1"
  "\x15\x3E\x1C\x09\x81\xC9\x32\x45\x4D\xE1\x11\x12\xD3\xCD\x71\x10"
  "\x03\xFE\x2B\x7E\x32\x46\x11\x2C\x34\x6C\x58\x3B\xF1\x4B\xA2\x0C"
  "\x60\x78\xA1\x64\x9D\x43\xDF\xC0\x8B\x8A\x64\x5D\x02\x41\x00\xD3"
  "\x42\xDE\x11\x6F\x9A\xDF\x26\x49\xE7\x8E\x6B\xAD\x79\xE7\x63\x61"
  "\x53\x0C\x5F\x93\x4D\xA1\xD8\xAE\x37\xE6\x20\x78\x30\xC7\x37\x9B"
  "\x82\xA6\x46\x6D\x58\x9C\x7C\xEA\x1F\x68\x35\x0C\x6A\x72\x17\xB9"
  "\x17\x79\x56\x24\xAC\xF2\x76\x71\xE7\x04\x05\xD2\x69\x4B\xE9\x02"
  "\x41\x00\x83\x7B\x91\x02\x66\xA0\x81\xA9\xBD\xBA\xA2\x86\x3D\x2B"
  "\x03\x61\xE8\x8F\x05\x12\xE3\x33\xAF\x6C\x9D\xFD\x22\xBF\xC5\xC0"
  "\x3B\xD3\x69\x45\x37\xAF\x3D\xC5\xFE\x91\x14\x73\x5C\x8E\xEC\xEE"
  "\xA3\x1B\x90\xB7\x23\x43\xC3\x5D\x7E\xF8\xBE\xDB\x3E\x62\x1A\x17"
  "\xE9\xBF\x00\x00";

const uint8_t ConstructorFixture::signed_encrypted_credentials[385] =
  "\x0B\xAD\x19\x22\xB6\x60\xF4\xC7\xB4\xEA\x34\xD9\xBF\xBB\x31\xDC"
  "\x1A\x60\x99\xD8\x57\xF5\x4A\x88\xC7\x5C\x61\x2F\x91\x59\xE9\xE6"
  "\x9E\x6B\x1F\x86\xCF\x83\xE3\xE5\xE7\x8F\x7B\x89\x12\x63\xEC\xA2"
  "\x85\xA7\x87\x11\x41\xC2\xE0\xA1\x5C\x4F\xD3\x1D\x23\xDD\x19\xF5"
  "\x38\xB0\x6C\x4B\x70\xE4\x26\x31\xE6\x16\x81\x2A\x82\x80\xA6\xE0"
  "\x78\x3E\xE3\xDE\xB2\x29\x0F\x81\x72\x48\x27\x6E\x48\x01\x13\xED"
  "\xFF\x09\x8D\xFC\xBB\xA2\x46\x5C\xB2\x07\xDC\x8D\x58\x36\x8B\xA8"
  "\x21\xA6\x5D\xAC\x6E\x6E\xF0\x8E\x39\x5A\xD3\x71\x65\x92\x6B\xF0"
  "\x9E\x27\x75\x13\x79\xA7\xCD\xAD\x74\xF8\xAF\xA4\x4D\xDA\x11\x1D"
  "\x0A\x8F\xE7\x7B\xFC\xB7\x1A\x45\x45\x88\x01\x7E\x86\x03\x3D\x75"
  "\xE2\x37\x9C\x3D\x26\x51\x59\x3F\x73\xF7\x36\x44\xD1\xB7\x6C\x59"
  "\x72\x0E\xE9\x42\x10\x48\xE0\xA0\xB5\x3F\x11\x35\xD2\x5C\x6F\x55"
  "\x98\x13\xAF\xEF\x0C\xFE\x2D\x36\xB9\x63\x20\xD7\x69\x81\xE8\xAB"
  "\x2E\x78\x0A\xFD\x27\x5B\x4E\xC9\x1F\x1A\xC1\xFB\x06\x86\x8E\x63"
  "\xA3\xE5\xDC\x97\x05\x09\x16\x5A\xD2\x54\x1C\xA0\x16\x67\x53\x4C"
  "\xFB\x30\x6A\xB6\x85\x4E\x96\x11\xCF\xA1\xC4\x85\x4F\x1B\xB5\xD6"
  "\x8A\x91\xEA\x26\xD1\xA7\xD0\x25\x58\x93\x05\x93\x2B\xEC\x93\xD2"
  "\xCD\x96\x3D\x03\xF4\xEB\x80\x9E\x17\x3E\x64\xB2\xA1\xA8\x95\xB8"
  "\xE5\xB0\xC9\xD8\x39\xDA\x18\x32\xC1\xC7\xF8\x85\x62\xBA\x8F\x2E"
  "\x45\xA2\x41\x31\x2F\x26\x44\x5B\xA6\xA4\x4D\x70\xCD\xC0\xFB\x8B"
  "\x68\x6A\xBA\x0E\xE0\xD5\xF4\x28\x31\x6C\xC7\xE5\x40\x30\x6A\xC1"
  "\xEE\x2F\x3F\xA6\x74\x81\x3D\xA1\xDC\xBA\x34\xE4\xC7\x44\x58\x3F"
  "\x0C\x99\xD0\xCD\xC0\xC0\x4A\x9F\x10\x95\x50\x49\x09\xCE\x09\xE2"
  "\xF7\xBD\x88\x2E\xAE\x86\xCD\x19\x1E\xAC\x3A\x0E\xB2\x29\x1B\x6B";

int ConstructorFixture::bytes_sent[2] = { 0, 0 };
int ConstructorFixture::bytes_received[2] = { 0, 0 };
uint8_t ConstructorFixture::sent_buf_0[256];
uint8_t ConstructorFixture::sent_buf_1[256];

ConstructorFixture::ConstructorFixture()
{
  bytes_sent[0] = bytes_sent[1] = 0;
  bytes_received[0] = bytes_received[1] = 0;
  keys.core_private = private_key;
  keys.server_public = pubkey;
  callbacks.send = mock_send;
  callbacks.receive = mock_receive;
  callbacks.signal = mock_signal;
  callbacks.millis = mock_millis;
  descriptor.num_functions = mock_num_functions;
  descriptor.copy_function_key = mock_copy_function_key;
  descriptor.call_function = mock_call_function;
  descriptor.get_variable = mock_get_variable;
  descriptor.was_ota_upgrade_successful = mock_ota_status_check;
  descriptor.variable_type = mock_variable_type;
  function_called = false;
  variable_to_get = -98765;
  spark_protocol.init(id, keys, callbacks, descriptor);
}

int ConstructorFixture::mock_send(const unsigned char *buf, int buflen)
{
  if (0 < buflen)
  {
    if (0 == bytes_sent[0] || 1 == bytes_sent[0])
    {
      uint8_t *dst = sent_buf_0;
      if (20 == buflen)
      {
        // blocking send test, part 1
        buflen = 1;
      }
      else if (19 == buflen)
      {
        // blocking send test, part 2
        dst = sent_buf_0 + 1;
      }
      // event loop tests send 16 + 2 or 32 + 2
      else if (34 != buflen && 18 != buflen)
      {
        // handshake, send first several bytes
        buflen = 11;
      }
      memcpy(dst, buf, buflen);
      bytes_sent[0] += buflen;
    }
    else if (11 == bytes_sent[0])
    {
      // handshake, send remaining bytes
      memcpy(sent_buf_0 + 11, buf, buflen);
      bytes_sent[0] += buflen;
    }
    else
    {
      memcpy(sent_buf_1, buf, buflen);
      bytes_sent[1] += buflen;
    }
  }
  else buflen = 0;
  return buflen;
}

int ConstructorFixture::mock_receive(unsigned char *buf, int buflen)
{
  if (0 < buflen)
  {
    if (0 == bytes_received[0] || 7 == bytes_received[0])
    {
      if (40 == buflen)
      {
        // handshake, receive first several bytes
        buflen = 7;
        memcpy(buf, nonce, buflen);
      }
      else if (33 == buflen)
      {
        // handshake, receive remainint bytes
        memcpy(buf, nonce + 7, buflen);
      }
      else
      {
        // event_loop
        memcpy(buf, message_to_receive, buflen);
      }
      bytes_received[0] += buflen;
    }
    else
    {
      if (384 == buflen)
      {
        // handshake
        memcpy(buf, signed_encrypted_credentials, buflen);
      }
      else if (20 == buflen)
      {
        // blocking receive test, part 1
        buflen = 1;
        memcpy(buf, message_to_receive, buflen);
      }
      else if (19 == buflen)
      {
        // blocking receive test, part 2
        memcpy(buf, message_to_receive, buflen);
      }
      else
      {
        // event_loop
        memcpy(buf, message_to_receive + 2, buflen);
      }
      bytes_received[1] += buflen;
    }
  }
  else buflen = 0;
  return buflen;
}

uint8_t ConstructorFixture::message_to_receive[34];
bool ConstructorFixture::function_called = false;
int ConstructorFixture::variable_to_get = -98765;
bool ConstructorFixture::signal_called_with = false;

int ConstructorFixture::mock_num_functions(void)
{
  return 1;
}

void ConstructorFixture::mock_copy_function_key(char *dst, int i)
{
  const char *funcs[1] = { "brew\0\0\0\0\0\0\0\0" };
  memcpy(dst, funcs[i], SparkProtocol::MAX_FUNCTION_KEY_LENGTH);
}

int ConstructorFixture::mock_call_function(const char *function_key,
                                           const char *arg)
{
  const char *prevent_warning;
  prevent_warning = function_key;
  prevent_warning = arg;
  function_called = true;
  return 456;
}

void *ConstructorFixture::mock_get_variable(const char *variable_key)
{
  const char *prevent_warning;
  prevent_warning = variable_key;
  return &variable_to_get;
}

void ConstructorFixture::mock_signal(bool on)
{
  signal_called_with = on;
}

unsigned int ConstructorFixture::mock_millis(void)
{
  return 0;
}

bool ConstructorFixture::mock_ota_status_check(void)
{
  return false;
}

SparkReturnType::Enum ConstructorFixture::mock_variable_type(const char *variable_key)
{
  const char *prevent_warning;
  prevent_warning = variable_key;
  return SparkReturnType::INT;
}

SUITE(SparkProtocolConstruction)
{
  TEST_FIXTURE(ConstructorFixture, NoErrorReturnedFromHandshake)
  {
    int err = spark_protocol.handshake();
    CHECK_EQUAL(0, err);
  }

  TEST_FIXTURE(ConstructorFixture, HandshakeReceives40Bytes)
  {
    spark_protocol.handshake();
    CHECK_EQUAL(40, bytes_received[0]);
  }

  TEST_FIXTURE(ConstructorFixture, HandshakeSends256Bytes)
  {
    spark_protocol.handshake();
    CHECK_EQUAL(256, bytes_sent[0]);
  }

  TEST_FIXTURE(ConstructorFixture, HandshakeLaterReceives384Bytes)
  {
    spark_protocol.handshake();
    CHECK_EQUAL(384, bytes_received[1]);
  }

  TEST_FIXTURE(ConstructorFixture, HandshakeLaterSends18Bytes)
  {
    spark_protocol.handshake();
    CHECK_EQUAL(18, bytes_sent[1]);
  }

  TEST_FIXTURE(ConstructorFixture, HandshakeSendsExpectedHello)
  {
    const uint8_t expected[18] = {
      0x00, 0x10,
      0xde, 0x89, 0x60, 0x27, 0xba, 0x11, 0x1a, 0x95,
      0xbe, 0x39, 0x6f, 0x4d, 0xfc, 0x30, 0x9b, 0x6b };
    spark_protocol.handshake();
    CHECK_ARRAY_EQUAL(expected, sent_buf_1, 18);
  }


  /********* event loop *********/

  TEST_FIXTURE(ConstructorFixture, EventLoopReceives2Bytes)
  {
    spark_protocol.handshake();
    bytes_received[0] = 0;
    spark_protocol.event_loop();
    CHECK_EQUAL(2, bytes_received[0]);
  }

  TEST_FIXTURE(ConstructorFixture, EventLoopLaterReceives16Bytes)
  {
    uint8_t describe[18] = {
      0x00, 0x10,
      0xd5, 0xb7, 0xf7, 0xfe, 0x9f, 0x2d, 0xca, 0xac,
      0xda, 0x15, 0x10, 0xa3, 0x27, 0x8b, 0xa7, 0xa9 };
    memcpy(message_to_receive, describe, 18);
    spark_protocol.handshake();
    bytes_received[0] = bytes_received[1] = 0;
    spark_protocol.event_loop();
    CHECK_EQUAL(16, bytes_received[1]);
  }

  TEST_FIXTURE(ConstructorFixture, EventLoopRespondsToDescribeWith34Bytes)
  {
    uint8_t describe[18] = {
      0x00, 0x10,
      0x4d, 0x2b, 0x01, 0x6f, 0x13, 0xee, 0xde, 0xdc,
      0xaf, 0x79, 0x23, 0xfb, 0x76, 0x81, 0xb3, 0x7a };
    memcpy(message_to_receive, describe, 18);
    spark_protocol.handshake();
    bytes_received[0] = bytes_sent[0] = 0;
    spark_protocol.event_loop();
    CHECK_EQUAL(34, bytes_sent[0]);
  }

  TEST_FIXTURE(ConstructorFixture, EventLoopRespondsToDescribeWithDescription)
  {
    uint8_t describe[18] = {
      0x00, 0x10,
      0x4d, 0x2b, 0x01, 0x6f, 0x13, 0xee, 0xde, 0xdc,
      0xaf, 0x79, 0x23, 0xfb, 0x76, 0x81, 0xb3, 0x7a };
    memcpy(message_to_receive, describe, 18);
    const uint8_t expected[34] = {
      0x00, 0x20,
      0x8c, 0x75, 0x7e, 0x24, 0xf7, 0x56, 0xde, 0x78,
      0xd8, 0x4f, 0x19, 0x80, 0xa8, 0xe0, 0xd1, 0x84,
      0xa3, 0x36, 0xb2, 0x8c, 0xb8, 0xfb, 0x3c, 0xfa,
      0xdf, 0x88, 0xe3, 0x66, 0x18, 0xe3, 0x18, 0x24 };
    spark_protocol.handshake();
    bytes_received[0] = bytes_sent[0] = 0;
    spark_protocol.event_loop();
    CHECK_ARRAY_EQUAL(expected, sent_buf_0, 34);
  }

  TEST_FIXTURE(ConstructorFixture, EventLoopRespondsToFunctionCallWithACK)
  {
    uint8_t function_call[34] = {
      0x00, 0x20,
      0xc6, 0xf4, 0xcb, 0x28, 0x85, 0xd8, 0x1e, 0xdd,
      0x23, 0x1c, 0x56, 0xb8, 0xd8, 0x03, 0x28, 0x94,
      0xce, 0x89, 0xab, 0x42, 0x48, 0x86, 0x78, 0x76,
      0xa0, 0x14, 0x07, 0x23, 0xb5, 0xa9, 0xfe, 0x75 };
    memcpy(message_to_receive, function_call, 34);
    const uint8_t expected[18] = {
      0x00, 0x10,
      0x80, 0xef, 0x08, 0x49, 0xd1, 0xbe, 0xe3, 0x20,
      0xf5, 0x06, 0xf9, 0x7d, 0xf1, 0xce, 0x25, 0x36 };
    spark_protocol.handshake();
    bytes_received[0] = bytes_sent[0] = 0;
    spark_protocol.event_loop();
    CHECK_ARRAY_EQUAL(expected, sent_buf_0, 18);
  }

  TEST_FIXTURE(ConstructorFixture,
               EventLoopLaterRespondsToFunctionCallWithFunctionReturn)
  {
    uint8_t function_call[34] = {
      0x00, 0x20,
      0xc6, 0xf4, 0xcb, 0x28, 0x85, 0xd8, 0x1e, 0xdd,
      0x23, 0x1c, 0x56, 0xb8, 0xd8, 0x03, 0x28, 0x94,
      0xce, 0x89, 0xab, 0x42, 0x48, 0x86, 0x78, 0x76,
      0xa0, 0x14, 0x07, 0x23, 0xb5, 0xa9, 0xfe, 0x75 };
    memcpy(message_to_receive, function_call, 34);
    const uint8_t expected[18] = {
      0x00, 0x10,
      0xa5, 0x71, 0x32, 0x5f, 0x7a, 0xd0, 0xf1, 0x3a,
      0xb9, 0x1c, 0x09, 0x2d, 0xe2, 0x8b, 0x4f, 0xe0 };
    spark_protocol.handshake();
    bytes_received[0] = bytes_sent[0] = 0;
    spark_protocol.event_loop();
    CHECK_ARRAY_EQUAL(expected, sent_buf_1, 18);
  }

  TEST_FIXTURE(ConstructorFixture,
               EventLoopRespondsToVariableRequestWithVariableValue)
  {
    uint8_t variable_request[34] = {
      0x00, 0x20,
      0xbc, 0x99, 0x43, 0x1b, 0x9a, 0x74, 0x44, 0x93,
      0x8f, 0x91, 0xa8, 0x4b, 0xc2, 0x09, 0x2b, 0xb5,
      0x75, 0x28, 0x57, 0xa6, 0xc6, 0xf4, 0x44, 0x07,
      0xdd, 0xd2, 0x0a, 0x72, 0x32, 0xb0, 0x1c, 0xbf };
    memcpy(message_to_receive, variable_request, 34);
    const uint8_t expected[18] = {
      0x00, 0x10,
      0x59, 0x28, 0xfa, 0x3d, 0x00, 0xea, 0xb2, 0x85,
      0x15, 0xb5, 0x06, 0x6d, 0x44, 0x65, 0x83, 0xa8 };
    spark_protocol.handshake();
    bytes_received[0] = bytes_sent[0] = 0;
    spark_protocol.event_loop();
    CHECK_ARRAY_EQUAL(expected, sent_buf_0, 18);
  }

  TEST_FIXTURE(ConstructorFixture, EventLoopCallsFunctionViaDescriptor)
  {
    uint8_t function_call[34] = {
      0x00, 0x20,
      0xc6, 0xf4, 0xcb, 0x28, 0x85, 0xd8, 0x1e, 0xdd,
      0x23, 0x1c, 0x56, 0xb8, 0xd8, 0x03, 0x28, 0x94,
      0xce, 0x89, 0xab, 0x42, 0x48, 0x86, 0x78, 0x76,
      0xa0, 0x14, 0x07, 0x23, 0xb5, 0xa9, 0xfe, 0x75 };
    memcpy(message_to_receive, function_call, 34);
    spark_protocol.handshake();
    bytes_received[0] = bytes_sent[0] = 0;
    spark_protocol.event_loop();
    CHECK(function_called);
  }

  TEST_FIXTURE(ConstructorFixture, BlockingSendAccumulatesOverMultipleCalls)
  {
    uint8_t expected[20] = {
      1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20 };
    spark_protocol.blocking_send(expected, 20);
    CHECK_ARRAY_EQUAL(expected, sent_buf_0, 20);
  }

  TEST_FIXTURE(ConstructorFixture, BlockingReceiveAccumulatesOverMultipleCalls)
  {
    uint8_t expected[20] = {
      1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20 };
    uint8_t receive_buf[20];
    memcpy(message_to_receive, expected, 20);
    spark_protocol.blocking_receive(receive_buf, 20);
    CHECK_ARRAY_EQUAL(expected, receive_buf, 20);
  }



  /*********************************
   * Over-the-air Firmware Updates *
   *********************************/

  TEST_FIXTURE(ConstructorFixture, EventLoopRespondsToUpdateBeginWithACK)
  {
    CHECK(false);
  }

  TEST_FIXTURE(ConstructorFixture, EventLoopPreparesForUpdateUponUpdateBegin)
  {
    // callbacks.prepare_for_firmware_update
    CHECK(false);
  }

  TEST_FIXTURE(ConstructorFixture, EventLoopRespondsToUpdateBeginWithUpdateReady)
  {
    CHECK(false);
  }

  TEST_FIXTURE(ConstructorFixture, EventLoopRespondsToChunkWithACK)
  {
    CHECK(false);
  }

  TEST_FIXTURE(ConstructorFixture, EventLoopRespondsToChunkWithChunkReceivedOKIfCRCMatches)
  {
    CHECK(false);
  }

  TEST_FIXTURE(ConstructorFixture, EventLoopRespondsToChunkWithChunkReceivedBADOnCRCMismatch)
  {
    CHECK(false);
  }

  TEST_FIXTURE(ConstructorFixture, EventLoopSavesReceivedChunk)
  {
    // callbacks.save_firmware_chunk
    CHECK(false);
  }

  TEST_FIXTURE(ConstructorFixture, EventLoopRespondsToUpdateDoneWithACK)
  {
    CHECK(false);
  }

  TEST_FIXTURE(ConstructorFixture, EventLoopFinishesFirmwareUpdateOnUpdateDone)
  {
    // callbacks.finish_firmware_update
    CHECK(false);
  }

  TEST_FIXTURE(ConstructorFixture, EventLoopSendsChunkMissedOnTimeout)
  {
    CHECK(false);
  }

  TEST_FIXTURE(ConstructorFixture, EventLoopCallsSignalWithTrueOnSignalStart)
  {
    uint8_t signal_start[18] = {
      0x00, 0x10,
      0x0B, 0xD9, 0x31, 0x11, 0xF2, 0x1D, 0xD2, 0x48,
      0x2B, 0x78, 0x26, 0x43, 0x38, 0x05, 0x9B, 0xB2 };
    memcpy(message_to_receive, signal_start, 18);
    spark_protocol.handshake();
    bytes_received[0] = bytes_sent[0] = 0;
    spark_protocol.event_loop();
    CHECK_EQUAL(true, signal_called_with);
  }

  TEST_FIXTURE(ConstructorFixture, EventLoopCallsSignalWithFalseOnSignalStop)
  {
    uint8_t signal_stop[18] = {
      0x00, 0x10,
      0xc0, 0x2f, 0x1e, 0x98, 0x92, 0x53, 0xc6, 0x12,
      0x67, 0x2a, 0xe0, 0x10, 0xeb, 0x20, 0xcc, 0x9f };
    memcpy(message_to_receive, signal_stop, 18);
    spark_protocol.handshake();
    bytes_received[0] = bytes_sent[0] = 0;
    spark_protocol.event_loop();
    CHECK_EQUAL(false, signal_called_with);
  }

  TEST(IsInitializedIsFalse)
  {
    SparkProtocol spark_protocol;
    CHECK_EQUAL(false, spark_protocol.is_initialized());
  }

  TEST_FIXTURE(ConstructorFixture, AfterInitIsInitializedIsTrue)
  {
    CHECK_EQUAL(true, spark_protocol.is_initialized());
  }
}
