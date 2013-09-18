#include "UnitTest++.h"
#include "spark_protocol.h"

struct ConstructorFixture
{
  static const uint8_t id[12];
  static const uint8_t pubkey[294];
  static const uint8_t private_key[1192];
  static int bytes_sent;
  static int bytes_received;
  static int mock_send(unsigned char *buf, int buflen);
  static int mock_receive(unsigned char *buf, int buflen);

  ConstructorFixture();
  SparkKeys keys;
  SparkCallbacks callbacks;
  SparkDescriptor descriptor;
};

const uint8_t ConstructorFixture::id[12] = {};
const uint8_t ConstructorFixture::pubkey[294] = {};
const uint8_t ConstructorFixture::private_key[1192] = {};
int ConstructorFixture::bytes_sent = 0;
int ConstructorFixture::bytes_received = 0;

ConstructorFixture::ConstructorFixture()
{
  bytes_sent = 0;
  bytes_received = 0;
  keys.core_private = private_key;
  keys.server_public = pubkey;
  callbacks.send = mock_send;
  callbacks.receive = mock_receive;
}

int ConstructorFixture::mock_send(unsigned char *buf, int buflen)
{
  if (0 < buflen)
    buf[0] = 0;
  bytes_sent += buflen;
  return buflen;
}

int ConstructorFixture::mock_receive(unsigned char *buf, int buflen)
{
  if (0 < buflen)
    buf[0] = 0;
  bytes_received += buflen;
  return buflen;
}

SUITE(SparkProtocolConstruction)
{
  TEST_FIXTURE(ConstructorFixture, ConstructorInjectsDependencies)
  {
    SparkProtocol spark_protocol(id, keys, callbacks, &descriptor);
    CHECK(&spark_protocol);
  }

  TEST_FIXTURE(ConstructorFixture, HandshakeReceives40Bytes)
  {
    SparkProtocol spark_protocol(id, keys, callbacks, &descriptor);
    spark_protocol.handshake();
    CHECK_EQUAL(40, bytes_received);
  }

  TEST_FIXTURE(ConstructorFixture, HandshakeSends256Bytes)
  {
    SparkProtocol spark_protocol(id, keys, callbacks, &descriptor);
    spark_protocol.handshake();
    CHECK_EQUAL(256, bytes_sent);
  }
}
