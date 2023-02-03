#include "application.h"
#include "unit-test.h"
#include "rgbled.h"

#ifdef HAS_SERIAL_FLASH
#include "flashee-eeprom.h"
#endif

#include "check.h"

namespace particle {

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

    static NullPrint* instance() {
        static NullPrint print;
        return &print;
    }
};

/**
 * A circular buffer for logging output.
 */
class LogBuffer: public Print {
public:
    static const size_t DEFAULT_BUFFER_SIZE = 1024;

    explicit LogBuffer(size_t size = DEFAULT_BUFFER_SIZE) :
            bufSize_(0),
            dataSize_(0) {
        resetBuffer(size);
    }

    size_t write(uint8_t b) override {
        return write(&b, 1);
    }

    size_t write(const uint8_t* data, size_t size) override {
        size_t n = size;
        if (n > bufSize_ - dataSize_) {
            size_t offs = 0;
            if (n > bufSize_) {
                data += n - bufSize_;
                n = bufSize_;
            } else {
                offs = bufSize_ - n;
                memmove(buf_.get(), buf_.get() + offs, dataSize_ - offs);
            }
            memcpy(buf_.get() + offs, data, n);
            if (bufSize_) {
                buf_[0] = '~';
            }
            dataSize_ = bufSize_;
        } else {
            memcpy(buf_.get() + dataSize_, data, n);
            dataSize_ += n;
        }
        return size;
    }

    bool resetBuffer(size_t size) {
        std::unique_ptr<char[]> buf;
        if (size > 0) {
            buf.reset(new(std::nothrow) char[size]);
            if (!buf) {
                return false;
            }
        }
        std::swap(buf_, buf);
        bufSize_ = size;
        dataSize_ = 0;
        return true;
    }

    size_t bufferSize() const {
        return bufSize_;
    }

    const char* data() const {
        return buf_.get();
    }

    size_t dataSize() const {
        return dataSize_;
    }

    void clear() {
        dataSize_ = 0;
    }

private:
    std::unique_ptr<char[]> buf_;
    size_t bufSize_;
    size_t dataSize_;
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

#ifdef HAS_SERIAL_FLASH

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

/**
 * Advances the log to the next block of data.
 * Returns the number of bytes of data available in the variable
 */
int advanceLog(Flashee::CircularBuffer* cblog, uint8_t* buf, size_t size) {
    int read = cblog->read_soft(buf, size - 1);
    buf[read] = 0;  // terminate string
    if (!read && TestRunner::instance()->isComplete()) {
        read = -1;  // end of stream.
    }
    return read;
}

#endif // defined(HAS_SERIAL_FLASH)

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

} // namespace

struct TestRunner::LogBufferData {
    LogBuffer stream;
    PrintTee tee;

    explicit LogBufferData(Print& otherStream) :
            tee(stream, otherStream) {
    }
};

struct TestRunner::CloudLogData {
#ifdef HAS_SERIAL_FLASH
    std::unique_ptr<Flashee::CircularBuffer> cblog;
    CircularBufferPrint stream;
    PrintTee tee;
    uint8_t buf[601];

    CloudLogData(Flashee::CircularBuffer* flashBuf, BufferFullCallback callback, Print& otherStream) :
            cblog(flashBuf),
            stream(*cblog, callback),
            tee(stream, otherStream),
            buf() {
    }
#endif // defined(HAS_SERIAL_FLASH)
};

TestRunner::TestRunner() :
        firstTest_(nullptr),
        state_(INIT),
        ledEnabled_(true),
        serialEnabled_(true),
        cloudEnabled_(true),
        logEnabled_(false),
        startRequested_(false),
        dfuRequested_(false) {
}

TestRunner::~TestRunner() {
    Test::out = NullPrint::instance();
}

void TestRunner::setup() {
    if (serialEnabled_) {
        Serial.begin();
        printStatus(Serial);
        Test::out = &Serial;
    } else {
        Test::out = NullPrint::instance();
    }
    if (cloudEnabled_) {
#ifdef HAS_SERIAL_FLASH
        Flashee::FlashDevice& store = Flashee::Devices::userFlash();
        // 64k should be plenty for anyone.
        int pageSize = store.pageSize();
        int pages = 64*1024/pageSize;
        // store circular buffer at end of external flash.
        std::unique_ptr<Flashee::CircularBuffer> buf;
        buf.reset(Flashee::Devices::createCircularBuffer((store.pageCount() - pages) * pageSize, store.length()));
        if (buf) {
            // direct output to the circular buffer
            cloudLog_.reset(new(std::nothrow) CloudLogData(buf.get(), spark_process, *Test::out));
            if (cloudLog_) {
                buf.release(); // Transfer ownership
                Particle.variable("log", cloudLog_->buf, STRING);
                Test::out = &cloudLog_->tee;
            }
        }
#endif // defined(HAS_SERIAL_FLASH)
        Particle.variable("passed", &Test::passed, INT);
        Particle.variable("failed", &Test::failed, INT);
        Particle.variable("skipped", &Test::skipped, INT);
        Particle.variable("count", &Test::count, INT);
        Particle.variable("state", &state_, INT);
        Particle.function("cmd", testCmd);
    }
    if (logEnabled_) {
        logBuf_.reset(new(std::nothrow) LogBufferData(*Test::out));
        if (logBuf_) {
            Test::out = &logBuf_->tee;
        }
    }
    firstTest_ = Test::root;
    setState(WAITING);
}

void TestRunner::loop() {
    if (dfuRequested_) {
        System.dfu();
    }

    if (!isStarted()) {
        if (serialEnabled_) {
            runSerialConsole();
        }
        if (startRequested_) {
            if (serialEnabled_) {
                Serial.println("Running tests");
            }
            start();
        }
    } else {
        Test::run();
    }
}

void TestRunner::reset() {
    if (state_ <= WAITING) {
        return;
    }
    Test::root = firstTest_;
    for (auto t = Test::root; t; t = t->next) {
        t->state = Test::UNSETUP;
    }
    Test::passed = 0;
    Test::failed = 0;
    Test::skipped = 0;
    if (logBuf_) {
        logBuf_->stream.clear();
    }
    flushMailbox();
    setState(WAITING);
}

const char* TestRunner::logBuffer() const {
    if (logBuf_) {
        return logBuf_->stream.data();
    }
    return nullptr;
}

size_t TestRunner::logSize() const {
    if (logBuf_) {
        return logBuf_->stream.dataSize();
    }
    return 0;
}

TestRunner& TestRunner::logSize(size_t size) {
    if (logBuf_) {
        logBuf_->stream.resetBuffer(size);
    }
    return *this;
}

void TestRunner::runSerialConsole() {
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

int TestRunner::testCmd(String arg) {
    int result = 0;
    if (arg.equals("start")) {
        TestRunner::instance()->startRequested_ = true;
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
        TestRunner::instance()->dfuRequested_ = true;
    }
#ifdef HAS_SERIAL_FLASH
    else if (arg.equals("log")) {
        const auto d = TestRunner::instance()->cloudLog_.get();
        if (d) {
            result = advanceLog(d->cblog.get(), d->buf, sizeof(d->buf));
        }
    }
#endif // HAS_SERIAL_FLASH
    else {
        result = -1;
    }
    return result;
}

int TestRunner::pushMailbox(MailboxEntry entry, system_tick_t wait) {
    auto m = new MailboxEntry(std::move(entry));
    CHECK_TRUE(m, SYSTEM_ERROR_NO_MEMORY);
    outMailbox_.pushBack(m);
    if (wait) {
        SCOPE_GUARD({
            if (m->isCompleted()) {
                popOutboundMailbox();
            }
        });
        return m->wait(wait) ? 0 : SYSTEM_ERROR_TIMEOUT;
    }
    return 0;
}

int TestRunner::pushMailboxMsg(const char* data, system_tick_t wait) {
    return pushMailbox(MailboxEntry().type(MailboxEntry::Type::DATA).data(data, strlen(data)), wait);
}

int TestRunner::pushMailboxBuffer(const char* data, size_t size, system_tick_t wait) {
    return pushMailbox(MailboxEntry().type(MailboxEntry::Type::DATA).data(data, size), wait);
}

void TestRunner::flushMailbox() {
    MailboxEntry* m = nullptr;
    while ((m = outMailbox_.popFront())) {
        delete m;
    }
}

TestRunner::MailboxEntry* TestRunner::peekOutboundMailbox() {
    return outMailbox_.front();
}

int TestRunner::popOutboundMailbox() {
    auto m = outMailbox_.popFront();
    if (m) {
        delete m;
        return 0;
    }
    return SYSTEM_ERROR_NOT_FOUND;
}

void TestRunner::updateLEDStatus() {
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

TestRunner* TestRunner::instance() {
    static TestRunner runner;
    return &runner;
}

} // namespace particle
