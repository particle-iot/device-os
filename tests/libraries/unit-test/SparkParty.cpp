#include "application.h"
#include "unit-test.h"
#include "flashee-eeprom.h"


/**
 * A tee - allows print output to be directed to two places at once.
 */
class PrintTee : public Print {
    Print& p1;
    Print& p2;

public:

    PrintTee(Print& _p1, Print& _p2) :
        p1(_p1), p2(_p2) {}

    virtual size_t write(uint8_t w) {
        p1.write(w);
        p2.write(w);
        return 1;
    }
    
    virtual size_t write(const uint8_t *buffer, size_t size) {
        p1.write(buffer, size);
        p2.write(buffer, size);
        return size;
    }    
};

typedef void (*BufferFullCallback)();

class CircularBufferPrint : public Print {
    Flashee::CircularBuffer& buffer;
    BufferFullCallback callback;
public:
    
    CircularBufferPrint(Flashee::CircularBuffer& _buffer, BufferFullCallback _callback) 
    : buffer(_buffer), callback(_callback) {}
    
    virtual size_t write(uint8_t w) {
        return write(&w, 1);
    }
    
    virtual size_t write(const uint8_t *data, size_t size) {
        // all or nothing write. Here we are assuming the buffer is much larger than
        // individual writes, and that the callback will help flush the buffer.
        while (!buffer.write(data, size)) {
            callback();
        }
        return size;
    }    
    
};



SparkTestRunner _runner;

bool requestStart = false;
bool _enterDFU = false;

Flashee::CircularBuffer* log;
PrintTee* tee;
uint8_t buf[601];

/**
 * A convenience method to setup serial.
 */
void unit_test_setup()
{
    Serial.begin(9600);    
    Flashee::FlashDevice& store = Flashee::Devices::userFlash();
    // 64k should be plenty for anyone.
    int pageSize = store.pageSize();
    int pages = 64*1024/pageSize;
    // store circular buffer at end of external flash.
    log = Flashee::Devices::createCircularBuffer((store.pageCount()-pages)*pageSize, store.length());
    // direct output to Serial and to the circular buffer
    tee = new PrintTee(*Test::out, *new CircularBufferPrint(*log, SPARK_WLAN_Loop));
    Test::out = tee;
    Spark.variable("log", buf, STRING);
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

/**
 * Advances the log to the next block of data.
 * Returns the number of bytes of data available in the variable
 */
int advanceLog() {
    int end = sizeof(buf)-1;
    int read = log->read_soft(buf, end);
    buf[read] = 0;  // terminate string
    if (!read && _runner.isComplete())
        read = -1;  // end of stream.
    return read;
}

int testCmd(String arg) {
    int result = 0;
    if (arg.equals("log")) {
        result = advanceLog();
    }
    else if (arg.equals("start")) {
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

