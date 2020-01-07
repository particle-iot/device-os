#include "application.h"
#include "unit-test/unit-test.h"

#include "io_expander.h"

UNIT_TEST_APP();

// Enable threading if compiled with "USE_THREADING=y"
#if PLATFORM_THREADING == 1 && USE_THREADING == 1
SYSTEM_THREAD(ENABLED);
#endif

SYSTEM_MODE(MANUAL);

SerialLogHandler log(LOG_LEVEL_INFO);

test(IOExpander_01_init_should_succeed) {
    assertEqual(IOExpander.init(0x20, D23, D22), (int)SYSTEM_ERROR_NONE);
}

test(IOExpander_02_hard_reset_should_succeed) {
    assertEqual(IOExpander.reset(), (int)SYSTEM_ERROR_NONE);
}

test(IOExpander_03_configure_pin_should_succeed) {
    IoExpanderPinConfig config = {};

    assertEqual(IOExpander.configure(config), (int)SYSTEM_ERROR_NONE);
}
