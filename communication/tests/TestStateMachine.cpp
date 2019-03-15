#include "UnitTest++.h"
#include "core_protocol.h"

SUITE(StateMachine)
{
  TEST(InitialStateIs_READ_NONCE)
  {
    SparkProtocol spark_protocol;
    ProtocolState::Enum state = spark_protocol.state();
    CHECK_EQUAL(ProtocolState::READ_NONCE, state);
  }
}
