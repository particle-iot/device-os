#include "UnitTest++.h"
#include "particle_protocol.h"

SUITE(StateMachine)
{
  TEST(InitialStateIs_READ_NONCE)
  {
    ParticleProtocol particle_protocol;
    ProtocolState::Enum state = particle_protocol.state();
    CHECK_EQUAL(ProtocolState::READ_NONCE, state);
  }
}
