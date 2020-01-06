#include "application.h"
#include "unit-test/unit-test.h"

#include "io_expander.h"

UNIT_TEST_APP();

// Enable threading if compiled with "USE_THREADING=y"
#if PLATFORM_THREADING == 1 && USE_THREADING == 1
SYSTEM_THREAD(ENABLED);
#endif

SYSTEM_MODE(MANUAL);
