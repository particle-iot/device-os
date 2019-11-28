#include "test_suite.h"

#include "spark_wiring_system.h"
#include "spark_wiring_startup.h"

#include "unit-test/unit-test.h"

using namespace particle;

STARTUP({
    System.enableFeature(FEATURE_RETAINED_MEMORY);
    TestSuite::instance()->init();
})

UNIT_TEST_APP()
