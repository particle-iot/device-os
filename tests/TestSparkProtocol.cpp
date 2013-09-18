#include "UnitTest++.h"
#include "spark_protocol.h"

struct ConstructorFixture
{
  static const uint8_t id[12];
  static const uint8_t pubkey[294];
  static const uint8_t private_key[1192];
  static bool send_called;
  static bool receive_called;
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
bool ConstructorFixture::send_called = false;
bool ConstructorFixture::receive_called = false;

ConstructorFixture::ConstructorFixture()
{
  send_called = false;
  receive_called = false;
  keys.core_private = private_key;
  keys.server_public = pubkey;
  callbacks.send = mock_send;
  callbacks.receive = mock_receive;
}

int ConstructorFixture::mock_send(unsigned char *buf, int buflen)
{
  if (0 < buflen)
    buf[0] = 0;
  send_called = true;
  return 1;
}

int ConstructorFixture::mock_receive(unsigned char *buf, int buflen)
{
  if (0 < buflen)
    buf[0] = 0;
  receive_called = true;
  return 1;
}

SUITE(SparkProtocolConstruction)
{
  TEST_FIXTURE(ConstructorFixture, ConstructorInjectsDependencies)
  {
    SparkProtocol spark_protocol(id, keys, callbacks, &descriptor);
    CHECK(&spark_protocol);
  }

  TEST_FIXTURE(ConstructorFixture, HandshakeCallsReceive)
  {
    SparkProtocol spark_protocol(id, keys, callbacks, &descriptor);
    spark_protocol.handshake();
    CHECK(receive_called);
  }
}
