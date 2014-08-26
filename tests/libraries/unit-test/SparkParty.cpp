#include "application.h"
#include "unit-test.h"

// custom spark changes begin

SparkTestRunner _runner;

bool requestStart = false;
bool _enterDFU = false;


/**
 * A convenience method to setup serial.
 */
void unit_test_setup()
{
    Serial.begin(9600);
    _runner.begin();
}


bool isStartRequested(bool runImmediately) {
    if (runImmediately || requestStart)
        return true;
    if (Serial.available()) {
        char c = Serial.read();
        if (c=='t') {
            return true;
        }
    }
    
    return false;
}

/*
 * A convenience method to run tests as part of the main loop after a character
 * is received over serial.
 **/
void unit_test_loop(bool runImmediately)
{       
    if (_enterDFU)
        System.bootloader();
    
    if (!_runner.isStarted() && isStartRequested(runImmediately)) {
        Serial.println("Running tests");
        _runner.start();
    }
    
    if (_runner.isStarted()) {
        Test::run();
    }
}

int SparkTestRunner::testStatusColor() {
    if (Test::failed>0)
        return RGB_COLOR_RED;
    else if (Test::skipped>0)
        return RGB_COLOR_ORANGE;
    else
        return RGB_COLOR_GREEN;
}

int testCmd(String arg) {
    int result = 0;
    if (arg.equals("start")) {
        requestStart = true;
    }
    else if (arg.startsWith("exclude=")) {
        String pattern = arg.substring(8);
        Test::exclude(pattern.c_str());
    }
    else if (arg.startsWith("include=")) {
        String pattern = arg.substring(8);
        Test::include(pattern.c_str());
    }
    else if (arg.equals("enterDFU")) {
        _enterDFU = true;
    }
    else 
        result = -1;
    return result;
}

void SparkTestRunner::begin() {    
    Spark.variable("passed", &Test::passed, INT);
    Spark.variable("failed", &Test::failed, INT);
    Spark.variable("skipped", &Test::skipped, INT);
    Spark.variable("count", &Test::count, INT);
    Spark.variable("state", &_state, INT);
    Spark.function("cmd", testCmd);
    setState(WAITING);
}

// custom spark changes end

