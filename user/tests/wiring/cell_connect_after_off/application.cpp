#include "application.h"
#include "unit-test/unit-test.h"

// make clean all TEST=wiring/no_fixture PLATFORM=electron -s COMPILE_LTO=n program-dfu DEBUG_BUILD=y
// make clean all TEST=wiring/no_fixture PLATFORM=electron -s COMPILE_LTO=n program-dfu DEBUG_BUILD=y USE_THREADING=y
//
 // Serial1LogHandler logHandler(115200, LOG_LEVEL_ALL, {
  // { "comm", LOG_LEVEL_NONE }, // filter out comm messages
  // { "system", LOG_LEVEL_INFO } // only info level for system messages
 // });

UNIT_TEST_APP();

SYSTEM_THREAD(ENABLED);