#include "UnitTest++.h"
#include "spark_protocol.h"

struct ConstructorFixture
{
  static const uint8_t id[12];
  static const uint8_t pubkey[294];
  static const uint8_t private_key[1192];
  int (*mock_send)(unsigned char *buf, int buflen);
  int (*mock_receive)(unsigned char *buf, int buflen);
};

const uint8_t ConstructorFixture::id[12] = {};
const uint8_t ConstructorFixture::pubkey[294] = {};
const uint8_t ConstructorFixture::private_key[1192] = {};

SUITE(SparkProtocolConstruction)
{
  TEST_FIXTURE(ConstructorFixture, ConstructorInjectsDependencies)
  {
    SparkKeys keys;
    keys.core_private = private_key;
    keys.server_public = pubkey;
    
    SparkCallbacks callbacks;
    callbacks.send = mock_send;
    callbacks.receive = mock_receive;

    SparkDescriptor descriptor;

    SparkProtocol spark_protocol(id, keys, callbacks, &descriptor);
    CHECK(&spark_protocol);
  }
}
