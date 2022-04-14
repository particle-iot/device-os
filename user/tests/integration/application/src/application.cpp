#include "test.h"
#include "test_suite.h"

#include "spark_wiring_startup.h"

#include "unit-test/unit-test.h"

#ifndef NO_TEST_APP_INIT
STARTUP({
    particle::testAppInit();
});
#endif

#ifndef NO_TEST_APP_SETUP_AND_LOOP
void setup() {
    particle::testAppSetup();
}

void loop() {
    particle::testAppLoop();
}
#endif

namespace particle {

void testAppInit() {
    SPARK_ASSERT(TestSuite::instance()->init() == 0);
}

void testAppSetup() {
    TestRunner::instance()->setup();
}

void testAppLoop() {
    TestRunner::instance()->loop();
}

} // namespace particle
