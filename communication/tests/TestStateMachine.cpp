#include "UnitTest++.h"
#include "core_protocol.h"

SUITE(StateMachine)
{
  TEST(InitialStateIs_READ_NONCE)
  {
    CoreProtocol core_protocol;
    ProtocolState::Enum state = core_protocol.state();
    CHECK_EQUAL(ProtocolState::READ_NONCE, state);
  }
}
