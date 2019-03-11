#include "application.h"
#include "unit-test/unit-test.h"

// make clean all TEST=hal/ble_peripheral PLATFORM=xenon DEBUG_BUILD=y
//
// Serial1LogHandler logHandler(115200, LOG_LEVEL_ALL, {
    // { "comm", LOG_LEVEL_NONE }, // filter out comm messages
    // { "system", LOG_LEVEL_INFO } // only info level for system messages
// });

UNIT_TEST_APP();

// Enable threading if compiled with "USE_THREADING=y"
#if PLATFORM_THREADING == 1 && USE_THREADING == 1
SYSTEM_THREAD(ENABLED);
#endif

SYSTEM_MODE(MANUAL);
