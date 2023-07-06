#ifndef PARTICLE_TEST_RUNNER

#include "application.h"
#include "unit-test/unit-test.h"

SYSTEM_MODE(SEMI_AUTOMATIC);

#if USE_THREADING
SYSTEM_THREAD(ENABLED);
#endif

UNIT_TEST_APP();

#endif // !defined(PARTICLE_TEST_RUNNER)
