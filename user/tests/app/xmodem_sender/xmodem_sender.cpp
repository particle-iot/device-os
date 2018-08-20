#include "application.h"

#include "exflash_hal.h"

#include "xmodem_sender.h"
#include "check.h"
#include "../../../../services/inc/stream.h"

#include <cstdarg>

SYSTEM_MODE(SEMI_AUTOMATIC)

namespace {

// Input stream generating text data
class TextStream: public InputStream {
public:
    explicit TextStream(size_t size) :
            size_(size),
            c_('A') {
    }

    int read(char* data, size_t size) override {
        // XmodemSender shouldn't try to read more bytes than necessary
        if (size > size_) {
            return SYSTEM_ERROR_OUT_OF_RANGE;
        }
        for (size_t i = 0; i < size; ++i) {
            data[i] = c_++;
            if (c_ > 'Z') {
                c_ = 'A';
            }
        }
        size_ -= size;
        return size;
    }

private:
    size_t size_;
    char c_;
};

// Input stream reading data from the external flash
class ExtFlashStream: public InputStream {
public:
    explicit ExtFlashStream(size_t offs, size_t size) :
            size_(size),
            offs_(offs) {
    }

    int read(char* data, size_t size) override {
        if (size > size_) {
            return SYSTEM_ERROR_OUT_OF_RANGE;
        }
        const int ret = hal_exflash_read(offs_, (uint8_t*)data, size);
        if (ret != 0) {
            return SYSTEM_ERROR_UNKNOWN;
        }
        size_ -= size;
        offs_ += size;
        return size;
    }

private:
    size_t size_;
    size_t offs_;
};

class SerialStream: public particle::Stream {
public:
    SerialStream() {
        Serial1.begin(115200);
    }

    ~SerialStream() {
        Serial1.end();
    }

    int read(char* data, size_t size) override {
        return Serial1.readBytes(data, size);
    }

    int write(const char* data, size_t size) override {
        return Serial1.write((const uint8_t*)data, size);
    }
};

class DummyModem {
public:
    explicit DummyModem(particle::Stream* strm) :
            strm_(strm) {
    }

    int writeCommand(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        char buf[256];
        size_t n = CHECK(vsnprintf(buf, sizeof(buf) - 2, fmt, args));
        buf[n++] = '\r';
        buf[n++] = '\n';
        CHECK(strm_->write(buf, n));
        Log.write(buf, n);
        return n;
    }

    int readResponse(unsigned timeout = 1000) {
        char buf[256];
        size_t size = 0;
        system_tick_t t = millis();
        do {
            char c = 0;
            const size_t n = CHECK(strm_->read(&c, 1));
            if (n > 0) {
                buf[size++] = c;
            }
        } while (millis() - t < timeout);
        Log.write(buf, size);
        return size;
    }

private:
    particle::Stream* strm_;
};

std::unique_ptr<XmodemSender> sender;
std::unique_ptr<SerialStream> serialStrm;
std::unique_ptr<DummyModem> modem;
std::unique_ptr<ExtFlashStream> extFlashStrm;
std::unique_ptr<TextStream> textStrm;

const RttLogHandler logHandler(LOG_LEVEL_ALL);

void initTextStream() {
    const size_t SIZE = 3500;
    textStrm.reset(new TextStream(SIZE));
    sender->init(serialStrm.get(), textStrm.get(), SIZE);
}

void initExtFlashStream() {
    const size_t SIZE = 825168;
    const size_t OFFSET = 0x00200000;
    extFlashStrm.reset(new ExtFlashStream(OFFSET, SIZE));
    sender->init(serialStrm.get(), extFlashStrm.get(), SIZE);
    modem->writeCommand("AT+FWUPD=%u", SIZE);
    modem->readResponse();
}

} // unnamed

void setup() {
    serialStrm.reset(new SerialStream);
    sender.reset(new XmodemSender);
    modem.reset(new DummyModem(serialStrm.get()));
    initTextStream();
    // initExtFlashStream();
}

void loop() {
    if (sender) {
        const int ret = sender->run();
        if (ret != XmodemSender::RUNNING) {
            if (ret == XmodemSender::DONE) {
                Log.info("XMODEM transfer finished");
            } else {
                Log.error("XMODEM transfer failed: %d", ret);
            }
            sender.reset();
        }
    }
}
