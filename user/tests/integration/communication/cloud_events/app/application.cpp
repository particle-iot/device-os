#include "request_handler.h"
#include "suite.h"

#include "spark_wiring_startup.h"

#include "unit-test/unit-test.h"

using namespace particle;

STARTUP({
	test::Suite::instance()->init();
})

UNIT_TEST_APP()
