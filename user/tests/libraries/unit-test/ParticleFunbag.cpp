#include "application.h"
#include "unit-test.h"
#include "rgbled.h"

#ifdef FLASHEE_EEPROM
#include "flashee-eeprom.h"
#endif

namespace {

/**
 * An output stream that discards the data.
 */
class NullPrint: public Print {
public:
    size_t write(uint8_t b) override {
        return 1;
    }

    size_t write(const uint8_t* buffer, size_t size) override {
        return size;
    }
};

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

#ifdef FLASHEE_EEPROM

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

const size_t LOG_BUFFER_SIZE = 601;

std::unique_ptr<Flashee::CircularBuffer> g_flashBuf;
std::unique_ptr<CircularBufferPrint> g_flashPrint;
std::unique_ptr<PrintTee> g_flashTee;
std::unique_ptr<uint8_t[]> g_logBuf;

/**
 * Advances the log to the next block of data.
 * Returns the number of bytes of data available in the variable
 */
int advanceLog() {
    if (!g_logBuf) {
        return -1;
    }
    int read = g_flashBuf->read_soft(g_logBuf.get(), LOG_BUFFER_SIZE - 1);
    g_logBuf[read] = 0;  // terminate string
    if (!read && SparkTestRunner::instance()->isComplete()) {
        read = -1;  // end of stream.
    }
    return read;
}

#endif // defined(FLASHEE_EEPROM)

unsigned countTests()
{
	unsigned total = 0;
	auto count = [&total](Test& t) {
		total++;
	};
	Test::for_each(count);
	return total;
}

void printStatus(Print& out)
{
    out.printlnf("%d tests available. Press 't' to begin, or 'i'/'e' to include/exclude tests matching a pattern.", countTests());
}

String readLine(Stream& stream)
{
	char buf[80];
	serialReadLine(&stream, buf, sizeof(buf), 0);
	return String(buf);
}

NullPrint g_nullPrint;

} // namespace

SparkTestRunner::SparkTestRunner() :
        ledEnabled_(true),
        serialEnabled_(true),
        cloudEnabled_(true),
        usbEnabled_(false),
        startRequested_(false),
        dfuRequested_(false),
        state_(INIT) {
}

void SparkTestRunner::setup() {
    Test::out = &g_nullPrint;
    if (serialEnabled_) {
        Serial.begin();
        printStatus(Serial);
        Test::out = &Serial;
    }
    if (cloudEnabled_) {
#ifdef FLASHEE_EEPROM
        Flashee::FlashDevice& store = Flashee::Devices::userFlash();
        // 64k should be plenty for anyone.
        int pageSize = store.pageSize();
        int pages = 64*1024/pageSize;
        // store circular buffer at end of external flash.
        g_flashBuf.reset(Flashee::Devices::createCircularBuffer((store.pageCount() - pages) * pageSize, store.length()));
        // direct output to Serial and to the circular buffer
        g_flashPrint.reset(new CircularBufferPrint(*g_flashBuf, spark_process));
        g_flashTee.reset(new PrintTee(*Test::out, *g_flashPrint));
        Test::out = g_flashTee.get();
        g_logBuf.reset(new(std::nothrow) uint8_t[LOG_BUFFER_SIZE]);
        if (g_logBuf) {
            g_logBuf[0] = 0;
            Particle.variable("log", g_logBuf.get(), STRING);
        }
#endif // defined(FLASHEE_EEPROM)
        Particle.variable("passed", &Test::passed, INT);
        Particle.variable("failed", &Test::failed, INT);
        Particle.variable("skipped", &Test::skipped, INT);
        Particle.variable("count", &Test::count, INT);
        Particle.variable("state", &state_, INT);
        Particle.function("cmd", testCmd);
    }
    setState(WAITING);
}

void SparkTestRunner::loop() {
    if (dfuRequested_) {
        System.dfu();
    }

    const auto runner = SparkTestRunner::instance();
    if (!runner->isStarted()) {
        if (serialEnabled_) {
            runSerialConsole();
        }
        if (startRequested_) {
            Serial.println("Running tests");
            runner->start();
        }
    }

    if (runner->isStarted()) {
        Test::run();
    }
}

void SparkTestRunner::runSerialConsole() {
    static bool firstFilter = true;
    if (Serial.available()) {
        char c = Serial.read();
        if (c=='t') {
            startRequested_ = true;
        }
        else if (c=='i') {
            Serial.print("Glob me for tests to include: ");
            String s = readLine(Serial);
            if (s.length()) {
                if (firstFilter) {
                    firstFilter = false;
                    Test::exclude("*");
                }
                unsigned count = Test::include(s.c_str());
                Serial.println();
                Serial.printlnf("Included %d tests matching '%s'.", count, s.c_str());
            }
            else {
                Serial.println("no changes made.");
            }
        }
        else if (c=='e') {
            Serial.print("Glob me for tests to exclude:");
            String s = readLine(Serial);
            if (s.length()) {
                firstFilter = false;
                unsigned count = Test::exclude(s.c_str());
                Serial.println();
                Serial.printlnf("Excluded %d tests matching '%s'.", count, s.c_str());
            }
            else {
                Serial.println("no changes made.");
            }
        }
        else if (c=='E') {
            Serial.println("Excluding all tests.");
            Test::exclude("*");
        }
        else if (c=='I') {
            Serial.println("Including all tests.");
            Test::include("*");
        }
        else if (c=='l') {
            Serial.println("Current tests included:");
            unsigned count = 0;
            auto print_and_count = [&count](Test& t) {
                if (t.is_enabled()) {
                    t.get_name().printTo(Serial);
                    Serial.println();
                    count++;
                }
            };
            Test::for_each(print_and_count);
            Serial.printlnf("%d test(s) included.", count);
        }
        else if (c=='c') {
            printStatus(Serial);
        }
        else if (c=='h') {
            Serial.println("Commands:");
            Serial.println(" i <glob>: include tests matching the glob");
            Serial.println(" e <glob>: exclude tests matching the glob");
            Serial.println(" I: include all tests");
            Serial.println(" E: exclude all tests");
            Serial.println(" c: count the tests");
            Serial.println(" l: list the tests");
            Serial.println(" t: start the tests");
            Serial.println(" h: get help. It looks a lot like what you're seeing now.");
            Serial.println("So...what'll it be, friend?");
        }
        else {
            Serial.print(c);
            Serial.println(": Not sure what you mean. Press 'h' for help.");
            printStatus(Serial);
        }
    }
}

int SparkTestRunner::testCmd(String arg) {
    int result = 0;
    if (arg.equals("start")) {
        SparkTestRunner::instance()->startRequested_ = true;
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
        SparkTestRunner::instance()->dfuRequested_ = true;
    }
#ifdef FLASHEE_EEPROM
    else if (arg.equals("log")) {
        result = advanceLog();
    }
#endif
    else {
        result = -1;
    }
    return result;
}

void SparkTestRunner::updateLEDStatus() {
    if (!ledEnabled_) {
        return;
    }
    int color = 0;
    if (Test::failed > 0) {
        color = RGB_COLOR_RED;
    } else if (Test::skipped > 0) {
        color = RGB_COLOR_ORANGE;
    } else {
        color = RGB_COLOR_GREEN;
    }
    RGB.control(true);
    RGB.color(color);
}

SparkTestRunner* SparkTestRunner::instance() {
    static SparkTestRunner runner;
    return &runner;
}
