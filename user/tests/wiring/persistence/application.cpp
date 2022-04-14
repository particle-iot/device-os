#include "application.h"
#include "unit-test/unit-test.h"

SYSTEM_MODE(SEMI_AUTOMATIC)

#if USE_THREADING == 1
SYSTEM_THREAD(ENABLED)
#endif

UNIT_TEST_APP()
