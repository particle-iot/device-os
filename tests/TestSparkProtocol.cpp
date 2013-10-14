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
  static uint8_t sent_buf_1[18];
  static int mock_send(const unsigned char *buf, int buflen);
  static int mock_receive(unsigned char *buf, int buflen);
  static uint8_t message_to_receive[18];
  static bool function_called;
  static int mock_call_function(const char *function_key, const char *arg);

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
  "\x3A\x47\xFA\x78\xC6\x69\xC4\x9C\xAB\xA9\x28\x9E\x75\x33\x78\xE6"
  "\x46\x5A\xBB\x3E\x9D\xC6\x5A\x11\xE2\xEB\xDA\x0A\x01\x46\xD0\x38"
  "\x85\x83\x3B\x4A\x69\x71\xD8\xBC\x3D\x57\x01\xBD\xF0\xA5\xA4\xF4"
  "\x4D\x8E\x25\x28\xE7\x38\x3B\x6A\x02\x88\xCE\x3B\x1B\x5C\xF8\x63"
  "\xFF\xB1\x1D\x80\x61\x43\xAE\xA0\xFD\xCE\xD7\x1E\x1A\x95\xD1\x84"
  "\xB4\xA1\x25\x67\x29\x43\x66\xDC\x48\xCC\x88\x1A\xC9\xF6\xF4\x00"
  "\xF6\x15\xB2\x55\xF0\xDC\xD5\x62\x35\x68\x59\x45\x11\x74\x14\x12"
  "\xD0\xC7\x30\x67\x5E\x66\x95\x1F\x77\xD3\x82\xEB\x40\xA1\x28\xB0"
  "\x27\x38\x4A\x28\x44\x64\xDD\xC8\xCA\x05\xA2\x38\x51\x1A\x28\x19"
  "\xC8\x3B\x76\xEA\x3C\x17\x49\x9E\xED\x04\xFC\xDB\x60\xB1\xB1\x8F"
  "\xE5\x99\x46\x3F\xCB\xD4\x94\x4C\x87\xE4\xA2\x6A\xFD\x75\x82\xAC"
  "\x3C\x16\x3B\x12\x1B\x47\x7E\x2A\x95\x58\x55\x48\x78\x8D\x82\xD2"
  "\x3A\xB3\x5C\xBC\xCF\xE5\x48\x52\xFE\x8E\x46\x35\xEA\xC4\xD9\xF1"
  "\xFD\x7D\xE6\xA4\xEC\x49\x9F\x26\x53\xB9\xFC\x39\xE9\x14\x9C\x46"
  "\x38\x15\x88\x7E\xE1\xDD\x38\x02\xB2\xEF\x11\x48\x7F\x57\xA2\x14"
  "\x0C\xBB\xB6\x87\xF0\xBA\x79\xF6\x64\xFC\xDC\x09\xDF\xCA\x2B\x56";

int ConstructorFixture::bytes_sent[2] = { 0, 0 };
int ConstructorFixture::bytes_received[2] = { 0, 0 };
uint8_t ConstructorFixture::sent_buf_0[256];
uint8_t ConstructorFixture::sent_buf_1[18];

ConstructorFixture::ConstructorFixture()
{
  bytes_sent[0] = bytes_sent[1] = 0;
  bytes_received[0] = bytes_received[1] = 0;
  keys.core_private = private_key;
  keys.server_public = pubkey;
  callbacks.send = mock_send;
  callbacks.receive = mock_receive;
  descriptor.call_function = mock_call_function;
  function_called = false;
  spark_protocol.init(id, keys, callbacks, &descriptor);
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

uint8_t ConstructorFixture::message_to_receive[18];
bool ConstructorFixture::function_called = false;

int ConstructorFixture::mock_call_function(const char *function_key,
                                           const char *arg)
{
  const char *prevent_warning;
  prevent_warning = function_key;
  prevent_warning = arg;
  function_called = true;
  return 456;
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
/*
  TEST_FIXTURE(ConstructorFixture, HandshakeSendsExpectedCiphertext)
  {
    const uint8_t expected[257] =
      "\x0C\x93\x27\x7B\x1A\x8E\x47\x34\x35\x82\xDD\xA7\x47\xDE\x9C\x58"
      "\x5B\xD7\x5F\x1C\x24\x8B\x3B\xC4\x07\x37\x6D\xB4\xDD\x02\x65\xEF"
      "\xE3\x83\xAE\xB0\x09\x46\xC0\x88\x08\xDF\x14\x9D\xA6\x68\x6E\x9D"
      "\x52\xFC\x72\xAB\x30\xAA\x9B\xDF\x5A\x1A\xFA\xE2\xA5\xC4\xE2\x50"
      "\x44\xC0\xD5\x56\xF8\x80\xA8\xD1\xEE\x35\xAF\x99\x6B\x18\x2A\xD8"
      "\x2C\x80\x07\xAD\x40\x88\x8A\xCE\x04\x90\x67\x46\x8B\x62\x92\xE8"
      "\xF9\x5E\x50\x1C\xB6\xA8\x4C\x6F\xD2\x00\xA8\x75\x47\xDA\x21\x1A"
      "\x81\xB3\xEB\x66\x05\xAB\xA9\x11\x6E\x6D\xA7\xB3\xCE\xD2\x43\xEA"
      "\x37\xBF\x1D\x78\x37\x9F\xDB\x44\xD2\xA0\x93\xA9\x91\x87\x85\x58"
      "\x6F\xB2\x42\xA2\xD9\x28\x3F\xBF\xC6\x50\x72\x1E\x38\x83\x7F\x3B"
      "\x47\x1B\x48\x81\x40\x03\x76\x42\x55\xD2\x81\x52\xAD\x95\xC9\x49"
      "\xBA\x71\x99\x8F\x13\xC1\xF8\x64\xDF\x5E\x4A\x2B\x04\xFD\xF5\x80"
      "\xB5\xFD\x4C\xB9\x1D\xBF\x16\x1F\x56\xF6\x94\xF1\x8E\x97\xFC\x9C"
      "\x58\xFB\x7C\x4C\x9B\x3B\xD6\xB7\x2E\x3D\x5D\xE6\xCB\x52\xA9\x6E"
      "\xCA\x04\x8D\xF8\x65\x76\x17\x83\x8B\xAF\xA1\xF5\xFB\x08\xC3\xCB"
      "\x44\xFB\xD5\xE9\xFC\xB8\xBD\x05\x6B\xA3\x92\x32\x03\xC1\xC5\x54";
    spark_protocol.handshake();
    CHECK_ARRAY_EQUAL(expected, sent_buf_0, 256);
  }
  */

  TEST_FIXTURE(ConstructorFixture, HandshakeLaterReceives512Bytes)
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
      0x89, 0x5f, 0xeb, 0x18, 0x07, 0xba, 0x1d, 0xbf,
      0x11, 0x26, 0x55, 0x04, 0x75, 0x28, 0x2a, 0xef };
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
      0xd5, 0xb7, 0xf7, 0xfe, 0x9f, 0x2d, 0xca, 0xac,
      0xda, 0x15, 0x10, 0xa3, 0x27, 0x8b, 0xa7, 0xa9 };
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
      0xd5, 0xb7, 0xf7, 0xfe, 0x9f, 0x2d, 0xca, 0xac,
      0xda, 0x15, 0x10, 0xa3, 0x27, 0x8b, 0xa7, 0xa9 };
    memcpy(message_to_receive, describe, 18);
    const uint8_t expected[34] = {
      0x00, 0x20,
      0x02, 0xC4, 0x63, 0x34, 0x71, 0x75, 0x8F, 0x2C,
      0xDB, 0x17, 0xA0, 0xB2, 0xFD, 0x11, 0x8B, 0xE0,
      0x9E, 0x16, 0x47, 0x44, 0x28, 0x3E, 0x37, 0xA8,
      0x11, 0x41, 0x55, 0x69, 0x5B, 0x50, 0x2F, 0x9E };
    spark_protocol.handshake();
    bytes_received[0] = bytes_sent[0] = 0;
    spark_protocol.event_loop();
    CHECK_ARRAY_EQUAL(expected, sent_buf_0, 34);
  }

  TEST_FIXTURE(ConstructorFixture, EventLoopRespondsToFunctionCallWithACK)
  {
    uint8_t function_call[34] = {
      0x00, 0x20,
      0xd7, 0xd1, 0x1c, 0x1c, 0x2d, 0xde, 0x7d, 0x09,
      0x42, 0x49, 0x0d, 0x4b, 0x4f, 0x44, 0x19, 0xc3,
      0x17, 0x00, 0x0f, 0xcf, 0xb2, 0x58, 0x85, 0x2d,
      0xdb, 0x2d, 0xf7, 0xf8, 0x15, 0x61, 0x25, 0x9b };
    memcpy(message_to_receive, function_call, 34);
    const uint8_t expected[18] = {
      0x00, 0x10,
      0x11, 0xea, 0xc7, 0xe9, 0x5f, 0xe0, 0x90, 0x83,
      0xf7, 0xa0, 0xa0, 0x75, 0xf3, 0xc4, 0x4a, 0x7f };
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
      0xd7, 0xd1, 0x1c, 0x1c, 0x2d, 0xde, 0x7d, 0x09,
      0x42, 0x49, 0x0d, 0x4b, 0x4f, 0x44, 0x19, 0xc3,
      0x17, 0x00, 0x0f, 0xcf, 0xb2, 0x58, 0x85, 0x2d,
      0xdb, 0x2d, 0xf7, 0xf8, 0x15, 0x61, 0x25, 0x9b };
    memcpy(message_to_receive, function_call, 34);
    const uint8_t expected[18] = {
      0x00, 0x10,
      0x4a, 0x49, 0x79, 0xb4, 0x99, 0xce, 0xd4, 0x3e,
      0x84, 0xc2, 0xdf, 0x84, 0x90, 0x51, 0x9b, 0x59 };
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
      0xf1, 0x86, 0x58, 0xe3, 0x80, 0x80, 0xe6, 0xc3,
      0x72, 0xf9, 0xb9, 0x36, 0x1a, 0xed, 0x51, 0x93,
      0x79, 0x8b, 0xbb, 0x53, 0xfe, 0x5c, 0xdb, 0xb5,
      0x86, 0x9f, 0x35, 0xb3, 0x19, 0xb5, 0xc4, 0x41 };
    memcpy(message_to_receive, variable_request, 34);
    const uint8_t expected[18] = {
      0x00, 0x10,
      0xc8, 0x2b, 0xfb, 0xdf, 0x77, 0x5c, 0x40, 0x87,
      0xc8, 0x05, 0x48, 0x8b, 0x77, 0x9d, 0x89, 0x5d };
    spark_protocol.handshake();
    bytes_received[0] = bytes_sent[0] = 0;
    spark_protocol.event_loop();
    CHECK_ARRAY_EQUAL(expected, sent_buf_0, 18);
  }

  TEST_FIXTURE(ConstructorFixture, EventLoopCallsFunctionViaDescriptor)
  {
    uint8_t function_call[34] = {
      0x00, 0x20,
      0xd7, 0xd1, 0x1c, 0x1c, 0x2d, 0xde, 0x7d, 0x09,
      0x42, 0x49, 0x0d, 0x4b, 0x4f, 0x44, 0x19, 0xc3,
      0x17, 0x00, 0x0f, 0xcf, 0xb2, 0x58, 0x85, 0x2d,
      0xdb, 0x2d, 0xf7, 0xf8, 0x15, 0x61, 0x25, 0x9b };
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
}
